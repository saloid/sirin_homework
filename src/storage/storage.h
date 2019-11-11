#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define FILES_PATH "/etc/" DAEMON_NAME "/"
#define IP_LIST_FILENAME FILES_PATH "ip_list"
#define IFACE_LIST_FILENAME FILES_PATH "iface_list"

typedef struct __attribute__((__packed__))
{
	uint32_t ipAddr;
	uint64_t recordsNum;
} ipRecordStruct;

typedef struct __attribute__((__packed__))
{
	char ifaceName[MAX_IFACE_LENGTH];
	uint64_t recordsNum;
} ifaceRecordStruct;

typedef struct
{
	void *bufferPtr;
	const size_t elementSize;
	size_t bufferSize;
	FILE *file;
} recordsBufferStruct;

/*
 *  Returns true if file creation/read succesfull.
 */
bool initStorage();

/*
 *  Increment packets num from this IP and iface 
 */
bool addIpAddr(uint32_t ipAddr, char *iface);

/*
 *  Returns packets num from this iface.
 */
uint64_t getIfaceStat(char *iface);

/*
 *  Get received packets num from this IP
 */
uint64_t getPacketsNum(uint32_t ipAddr);

/*
 *  Returns total packets num, from all ifaces
 */
uint64_t getTotalPacketsNum();

/*
 * Save all data to file
 */
void deinitStorage();

/*
 *  Delete all stored data
 */
void clearData();



#endif //_STORAGE_H_