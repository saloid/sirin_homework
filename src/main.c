#include "main.h"
#include "daemonize/daemonize.h"
#include "storage/storage.h"
#include "sniffer/sniffer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

volatile bool errorDetected = false;

int main(int argc, char **argv)
{
	(void)argc; //unused vars, supress warnings
	(void)argv;

	skeleton_daemon();

	syslog(LOG_NOTICE, "daemon started\n");
	if (!initStorage())
	{
		exit(EXIT_FAILURE);
	}

	setPacketCallback(newPacketCallback);
	startSniffer(NULL);
	syslog(LOG_NOTICE, "init finished\n");

	while (!errorDetected)
	{
		snifferLoop();
		sleep(1);
	}

	stopSniffer();
	deinitStorage();

	syslog(LOG_NOTICE, "init finished\n");

	return 0;
}

void newPacketCallback(uint32_t ipAddr, char *ifaceName)
{
	syslog(LOG_DEBUG, "IP: %s, iface: %s\n", ipToString(ipAddr), ifaceName);
	if (addIpAddr(ipAddr, ifaceName) == false)
	{
		syslog(LOG_ERR, "add ip addr failed\n");
		errorDetected = true;
	}
}