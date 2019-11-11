#ifndef _SNIFFER_H_
#define _SNIFFER_H_

#include <ctype.h>
#include <sys/types.h>  //u_char, u_short, etc.
#include <netinet/in.h> //in_addr struct
#include <pcap.h>       // pcap_pkthdr struct

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* IP header */
struct sniff_ip
{
    u_char ip_vhl;                 /* version << 4 | header length >> 2 */
    u_char ip_tos;                 /* type of service */
    u_short ip_len;                /* total length */
    u_short ip_id;                 /* identification */
    u_short ip_off;                /* fragment offset field */
#define IP_RF 0x8000               /* reserved fragment flag */
#define IP_DF 0x4000               /* dont fragment flag */
#define IP_MF 0x2000               /* more fragments flag */
#define IP_OFFMASK 0x1fff          /* mask for fragmenting bits */
    u_char ip_ttl;                 /* time to live */
    u_char ip_p;                   /* protocol */
    u_short ip_sum;                /* checksum */
    struct in_addr ip_src, ip_dst; /* source and dest address */
};
#define IP_HL(ip) (((ip)->ip_vhl) & 0x0f)

void setPacketCallback(void * callback);

/*
 * Begin sniffer service.
 * Pass NULL if need to listen default iface
 */
void startSniffer(char * ifaceName);

/*
 *Stop sniffer service. Call on end or reconfigure
 */
void stopSniffer();

char *ipToString(uint32_t ipAddr);

void snifferLoop();

/*
 * Packet handler (callback)
 */
void gotPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);

#endif  //_SNIFFER_H_F