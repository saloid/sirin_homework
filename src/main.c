#include "main.h"
#include "daemonize/daemonize.h"
#include "storage/storage.h"
#include "sniffer/sniffer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>

volatile bool needStop = false;

int main(int argc, char **argv)
{
	(void)argc; //unused vars, supress warnings
	(void)argv;

	skeleton_daemon();
	signal(SIGHUP, signal_handler);  /* catch hangup signal */
	signal(SIGTERM, signal_handler); /* catch kill signal */

	syslog(LOG_NOTICE, "daemon started\n");
	if (!initStorage())
	{
		exit(EXIT_FAILURE);
	}

	setPacketCallback(newPacketCallback);
	startSniffer(NULL);
	syslog(LOG_NOTICE, "init finished\n");

	while (!needStop)
	{
		snifferLoop();
		sleep(1);
	}

	stopSniffer();
	deinitStorage();

	syslog(LOG_NOTICE, "daemon stoped\n");
	closelog();

	return 0;
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

void signal_handler(int sig)
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