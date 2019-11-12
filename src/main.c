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
		//printf("Command sent, terminating main process\n");
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
		char buffer[BUFFER_SIZE];
		buffer[0] = NOPE;

		connectToSocket();
		if (startsWith(argv[1], "start") || startsWith(argv[1], ""))
		{
			buffer[0] = START;
		}
		else if (startsWith(argv[1], "stop"))
		{
			buffer[0] = STOP;
		}
		else if (startsWith(argv[1], "show") && argc == 4) //show [ip] count
		{
			printf("show cmd processing\n");
			if (strcmp("count", argv[3]) == 0)
			{
				buffer[0] = SHOW;
				uint32_t ipToShow = stringToIp(argv[2]);
				memcpy(&buffer[1], &ipToShow, sizeof(ipToShow));
			}
			else
			{
				printf("wrong command\n");
			}
		}
		else if (startsWith(argv[1], "select") && argc == 4 && startsWith(argv[2], "iface")) //select iface [iface]
		{
			buffer[0] = SELECT;
			strncpy(&buffer[1], argv[3], BUFFER_SIZE - 2);
		}
		else if (startsWith(argv[1], "stat")) //stat [iface]
		{
			buffer[0] = STAT;
			if (argc > 2)
			{
				strncpy(&buffer[1], argv[2], BUFFER_SIZE - 2);
			}
			else
			{
				buffer[1] = '\0';
			}
		}
		else if (startsWith(argv[1], "erase")) //erase
		{
			buffer[0] = ERASE;
		}
		else
		{
			printHelp();
		}

		guard(write(clientSocket, buffer, BUFFER_SIZE), "write");
		//printf("data writen, waiting answer\n");
		guard(read(clientSocket, buffer, BUFFER_SIZE), "read");
		buffer[BUFFER_SIZE - 1] = '\0'; //to be sure
		printf("%s\n", buffer);
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

	//printf("Connecting to daemon\n");

	clientSocket = guard(socket(AF_UNIX, SOCK_SEQPACKET, 0), "socket");

	memset(&name, 0, sizeof(struct sockaddr_un)); //compatibility

	name.sun_family = AF_UNIX; //config
	strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);
	while (connect(clientSocket, (const struct sockaddr *)&name, sizeof(name)) < 0)
	{
		//printf("Server offline, trying again...\n");
		usleep(50000);
	}
	//printf("connected to server\n");
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
	printf("erase                - clear all stores data\n");
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

	//startSniffer(NULL);
	createSocket();

	syslog(LOG_NOTICE, "daemon inited\n");

	while (!needStop)
	{
		snifferLoop();
		processSocket();
		usleep(50000);
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
				return; //ok, try next time
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

			processDaemonCommand(buffer);
			//sprintf(buffer, "%lu", getTotalPacketsNum());

			if (write(dataSocket, buffer, BUFFER_SIZE) < 0)
			{
				perror("write");
			}

			close(dataSocket);
			dataSocket = -1;
		}
	}
}

void processDaemonCommand(char *buffer)
{
	switch (buffer[0])
	{
	case START:
		startSniffer(NULL);
		sprintf(buffer, "daemon started");
		break;

	case STOP:
		sprintf(buffer, "daemon stoped");
		needStop = true;
		break;

	case SHOW:
	{
		uint32_t ipToShow;

		memcpy(&ipToShow, &buffer[1], sizeof(ipToShow));
		uint64_t packetsNum = getPacketsNum(ipToShow);
		sprintf(buffer, "num: %lu", packetsNum);
	}
	break;

	case SELECT:
		stopSniffer();
		startSniffer(&buffer[1]);
		sprintf(buffer, "iface changed");
		break;

	case STAT:
	{
		uint64_t packetsNum;

		if (buffer[1] == '\0')
		{
			packetsNum = getTotalPacketsNum();
		}
		else
		{
			packetsNum = getIfaceStat(&buffer[1]);
		}
		sprintf(buffer, "packets count: %lu", packetsNum);
	}
	break;

	case ERASE:
		clearData();
		needStop = true;
		sprintf(buffer, "data deleted, daemon stoped");
		break;

	case NOPE:
		syslog(LOG_NOTICE, "NOPE command received\n");
		break;

	default:
		syslog(LOG_WARNING, "Unknown command: %d\n", buffer[0]);
		sprintf(buffer, "unkknown command");
		break;
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