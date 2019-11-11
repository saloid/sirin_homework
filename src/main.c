#include "main.h"
#include "storage/storage.h"
#include "sniffer/sniffer.h"
#include <stdbool.h>



int main(int argc, char **argv)
{
	(void)argc;	//unused vars, supress warnings
	(void)argv;

	setPacketCallback(newPacketCallback);
	startSniffer(NULL);

	while (true)
	{
		snifferLoop();
	}

	stopSniffer();

	printf("\nProg end.\n");

	return 0;
}

void newPacketCallback (uint32_t ipAddr, char * ifaceName)
{
	printf("IP: %s, iface: %s\n", ipToString(ipAddr), ifaceName);
}