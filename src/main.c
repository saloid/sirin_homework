#include "main.h"
#include "daemonize/daemonize.h"
#include "storage/storage.h"
#include "sniffer/sniffer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h> //non blocking socket
#include <errno.h>

bool thisIsDaemon;

int main(int argc, char **argv)
{
	thisIsDaemon = createDaemon();
	if (thisIsDaemon)
	{
		doSomeDaemonStuff();
	}
	else
	{
		passCommandToDaemon(argc, argv);
		printf("Command sent, terminating main process\n");
	}

	return (0);
}

int guard(int n, char *err)
{
	if (n == -1)
	{
		if (thisIsDaemon)
		{
			syslog(LOG_ERR, "socket error: %s (%s)\n", err, strerror(errno));
		}
		else
		{
			perror(err);
		}
		exit(EXIT_FAILURE);
	}
	return (n);
}

//main proc part
int clientSocket;

void passCommandToDaemon(int argc, char **argv)
{
	if (argc > 1)
	{
		connectToSocket();
		if (startsWith(argv[1], "start") || startsWith(argv[1], ""))
		{
			//start sniffer
		}
		else if (startsWith(argv[1], "stop"))
		{
		}
		else if (startsWith(argv[1], "show") && argc == 4) //show [ip] count
		{
		}
		else if (startsWith(argv[1], "select") && argc == 4) //select iface [iface]
		{
		}
		else if (startsWith(argv[1], "stat")) //stat [iface]
		{
		}
		else
		{
			printHelp();
		}
		close(clientSocket);
	}
	else
	{
		printHelp();
	}
}

void connectToSocket()
{
	struct sockaddr_un name;
	char buffer[BUFFER_SIZE];

	printf("Connecting to daemon\n");

	clientSocket = guard(socket(AF_UNIX, SOCK_SEQPACKET, 0), "socket");

	memset(&name, 0, sizeof(struct sockaddr_un)); //compatibility

	name.sun_family = AF_UNIX; //config
	strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);
	while (connect(clientSocket, (const struct sockaddr *)&name, sizeof(name)) < 0)
	{
		printf("Server offline, trying again...\n");
		sleep(1);
	}
	printf("connected to server\n");

	buffer[0] = 42;
	strcpy(&buffer[1], "hello");
	guard(write(clientSocket, buffer, BUFFER_SIZE), "write");

	printf("data writen, waiting answer\n");

	guard(read(clientSocket, buffer, BUFFER_SIZE), "read");

	buffer[BUFFER_SIZE - 1] = '\0';
	printf("Answer = %s\n", buffer);
}

void printHelp()
{
	printf("---------------------------------------------------------------------------------------------------------------------\n");
	printf("Sniffer available commands:\n\n");
	printf("start                - packets are being sniffed from now on from default iface\n");
	printf("stop                 - packets are not sniffed\n");
	printf("show [ip] count      - print number of packets received from ip address\n");
	printf("select iface [iface] - select interface for sniffing eth0, wlan0, ethN, wlanN...\n");
	printf("stat [iface]         - show all collected statistics for particular interface, if iface omitted - for all interfaces.\n");
	printf("--help               - show this text\n");
	printf("---------------------------------------------------------------------------------------------------------------------\n");
}

bool startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre),
		   lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

//daemon part

volatile bool needStop = false;
int daemonSocket;

void doSomeDaemonStuff()
{
	syslog(LOG_NOTICE, "daemon init...\n");

	signal(SIGHUP, signalHandlerCallback);  // catch hangup signal
	signal(SIGTERM, signalHandlerCallback); // catch kill signal
	setPacketCallback(newPacketCallback);   // set callback

	if (!initStorage()) //critical file or buffer error - no reason to stay alive
	{
		exit(EXIT_FAILURE);
	}

	startSniffer(NULL);
	createSocket();

	syslog(LOG_NOTICE, "daemon inited\n");

	while (!needStop)
	{
		processSocket();
		snifferLoop();
		sleep(1);
	}

	close(daemonSocket);
	unlink(SOCKET_NAME);

	stopSniffer();
	deinitStorage();

	syslog(LOG_NOTICE, "daemon stoped\n");
	closelog();
}

void createSocket()
{
	struct sockaddr_un name;

	unlink(SOCKET_NAME);												//delete socket from last time
	daemonSocket = guard(socket(AF_UNIX, SOCK_SEQPACKET, 0), "socket"); //creating socket

	//create non-blocking socket
	int flags = guard(fcntl(daemonSocket, F_GETFL), "could not get flags on TCP listening socket");
	guard(fcntl(daemonSocket, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");

	memset(&name, 0, sizeof(struct sockaddr_un)); //for portability

	name.sun_family = AF_UNIX; //config
	strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

	guard(bind(daemonSocket, (const struct sockaddr *)&name, sizeof(name)), "bind"); //bind

	guard(listen(daemonSocket, 20), "listen"); //listen
	syslog(LOG_NOTICE, "Socket created at addr %s\n", SOCKET_NAME);
}

void processSocket()
{
	static int dataSocket = -1;
	if (dataSocket < 0)
	{
		dataSocket = accept(daemonSocket, NULL, NULL);
	}
	if (dataSocket >= 0) //new input connection detected
	{
		syslog(LOG_NOTICE, "New input connection");
		char buffer[BUFFER_SIZE];

		if (read(dataSocket, buffer, BUFFER_SIZE) < 0) //new input packet
		{
			if (errno == EWOULDBLOCK)
			{
				return; //ok, try in next iteration
			}
			else
			{
				dataSocket = -1;
				perror("read");
			}
		}
		else
		{
			buffer[BUFFER_SIZE - 1] = '\0';
			syslog(LOG_NOTICE, "New input data: %d, %s", buffer[0], &buffer[1]);

			sprintf(buffer, "%lu", getTotalPacketsNum());

			if (write(dataSocket, buffer, BUFFER_SIZE) < 0)
			{
				perror("write");
			}

			close(dataSocket);
			dataSocket = -1;
		}
	}
}

void newPacketCallback(uint32_t ipAddr, char *ifaceName)
{
	syslog(LOG_DEBUG, "IP: %s, iface: %s\n", ipToString(ipAddr), ifaceName);
	if (addIpAddr(ipAddr, ifaceName) == false)
	{
		syslog(LOG_ERR, "add ip addr failed\n");
		needStop = true;
	}
}

//callback
void signalHandlerCallback(int sig)
{
	switch (sig)
	{
	case SIGHUP:
		syslog(LOG_NOTICE, "hangup signal catched");
		break;
	case SIGTERM:
		syslog(LOG_NOTICE, "terminate signal catched");
		needStop = true;
		break;
	}
}