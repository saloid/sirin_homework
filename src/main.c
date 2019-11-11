#include "main.h"
#include "storage/storage.h"
#include "sniffer/sniffer.h"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

volatile bool errorDetected = false;

int main(int argc, char **argv)
{
	(void)argc; //unused vars, supress warnings
	(void)argv;

	if (!initStorage())
	{
		exit(EXIT_FAILURE);
	}

	setPacketCallback(newPacketCallback);
	startSniffer(NULL);
	printf("init finished\n");

	while (!errorDetected)
	{
		snifferLoop();
		sleep(1);
	}

	stopSniffer();
	deinitStorage();

	printf("\nProg end.\n");

	return 0;
}

void newPacketCallback(uint32_t ipAddr, char *ifaceName)
{
	printf("IP: %s, iface: %s\n", ipToString(ipAddr), ifaceName);
	if (addIpAddr(ipAddr, ifaceName) == false)
	{
		printf("add ip addr failed\n");
		errorDetected = true;
		//exit(EXIT_FAILURE);
	}
}