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

int main(int argc, char **argv)
{
	bool thisIsDaemon = createDaemon();
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

void passCommandToDaemon(int argc, char **argv)
{
	if (argc > 1)
	{
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
	}
	else
	{
		//start sniffer
	}
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

int guard(int n, char *err)
{
	if (n == -1)
	{
		syslog(LOG_ERR, "socket error: %s\n", err);
		perror(err);
		exit(1);
	}
	return (n);
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
	syslog(LOG_ERR, "Socket created at addr %s\n", SOCKET_NAME);
}

void processSocket()
{
	static int dataSocket = 0;
	if (dataSocket <= 0)
	{
		dataSocket = accept(daemonSocket, NULL, NULL);
	}
	if (dataSocket > 0) //new input connection detected
	{
		char buffer[BUFFER_SIZE];

		if (read(dataSocket, buffer, BUFFER_SIZE) <= 0) //new input packet
		{
			if (errno == EWOULDBLOCK)
			{
				return; //ok, try in next iteration
			}
			else
			{
				dataSocket = 0;
				perror("read");
			}
		}
		else
		{
			buffer[BUFFER_SIZE - 1] = 0;
			syslog(LOG_ERR, "New input data: %d, %s", buffer[0], &buffer[1]);

			sprintf(buffer, "%lu", getTotalPacketsNum());

			if (write(dataSocket, buffer, BUFFER_SIZE) <= 0)
			{
				perror("write");
			}

			close(dataSocket);
			dataSocket = 0;
		}
	}
}

bool startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre),
		   lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
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