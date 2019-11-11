#include "sniffer.h"
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

pcap_t *handle; // sniffer obj
struct bpf_program fp; // filter obj
char currentIfaceName[MAX_IFACE_LENGTH];
void (*_packetCallback)(uint32_t ipAddr, char *iface) = NULL;

void startSniffer(char *ifaceName)
{
    char *dev = NULL;              /* capture device name */
    char errbuf[PCAP_ERRBUF_SIZE]; /* error buffer */

    if (ifaceName == NULL)
    {   //set OS defult iface
        dev = pcap_lookupdev(errbuf);
		if (dev == NULL)
			{
				fprintf(stderr, "Couldn't find default device: %s\n",
						errbuf);
				exit(EXIT_FAILURE);
			}
    }
    else
    {
        dev = ifaceName;
    }

	if (strlen(dev) >= sizeof(currentIfaceName))	//check iface length
	{
		fprintf(stderr, "Too long iface name (%s), max length = %d\n", dev, (MAX_IFACE_LENGTH-1));
		exit(EXIT_FAILURE);
	}
	strncpy(currentIfaceName, dev, sizeof(currentIfaceName));

	bpf_u_int32 mask;									// subnet mask
	bpf_u_int32 net;                                    // gateway ip?
    if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) // get network number and mask associated with capture device
    {
        fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
                dev, errbuf);
        net = 0;
        mask = 0;
    }

    // print capture info
    printf("Device: %s\n", dev);

    handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf); // open capture device
    if (handle == NULL)
    {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        exit(EXIT_FAILURE);
    }

    if (pcap_datalink(handle) != DLT_EN10MB) // make sure we're capturing on an Ethernet device [2]
    {
        fprintf(stderr, "%s is not an Ethernet\n", dev);
        exit(EXIT_FAILURE);
    }

    const char filter_exp[] = "ip";                    // filter expression - all IP packets
    if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) //compile the filter expression
    {
        fprintf(stderr, "Couldn't parse filter %s: %s\n",
                filter_exp, pcap_geterr(handle));
        exit(EXIT_FAILURE);
    }

    if (pcap_setfilter(handle, &fp) == -1) // apply the compiled filter
    {
        fprintf(stderr, "Couldn't install filter %s: %s\n",
                filter_exp, pcap_geterr(handle));
        exit(EXIT_FAILURE);
    }

    if (pcap_setdirection(handle, PCAP_D_IN)) //capture only input packets
    {
        fprintf(stderr, "Couldn't set direction: %s\n", pcap_geterr(handle));
        exit(EXIT_FAILURE);
    }

	if (pcap_setnonblock(handle, 1, errbuf))
	{
		fprintf(stderr, "Non-block set error: %s\n", errbuf);
	}    
}

void stopSniffer()
{
	pcap_freecode(&fp);
	pcap_close(handle);
}

void snifferLoop()
{
	pcap_dispatch(handle, -1, gotPacket, NULL);
}

void gotPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	(void)args;	//supress not used warning
	(void)header;
	const struct sniff_ip *ip; /* The IP header */
	int size_ip;

    /* define/compute ip header offset */
    ip = (struct sniff_ip *)(packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip) * 4;
    if (size_ip < 20)
    {
        printf("   * Invalid IP header length: %u bytes\n", size_ip);
        return;
    }
	if (_packetCallback != NULL)
	{
		_packetCallback((ip->ip_src).s_addr, currentIfaceName);
	}
}

void setPacketCallback(void *callback)
{
	_packetCallback = callback;
}

char *ipToString(uint32_t ipAddr)
{
	struct in_addr ipToConvert = {.s_addr = ipAddr};
	return (inet_ntoa(ipToConvert));
}