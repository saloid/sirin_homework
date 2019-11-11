#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define FILES_PATH "/etc/" DAEMON_NAME "/"
#define IP_LIST_FILENAME FILES_PATH "ip_list"
#define IFACE_LIST_FILENAME FILES_PATH "iface_list"

typedef struct __attribute__((__packed__)) ipRecordStruct
{
    uint32_t ipAddr;
    uint64_t recordsNum;
} ipRecordStruct;

typedef struct __attribute__((__packed__)) ifaceRecordStruct
{
    char ifaceName[MAX_IFACE_LENGTH];
    uint64_t recordsNum;
} ifaceRecordStruct;

typedef struct recordsBufferStruct
{
    void *bufferPtr;
    const size_t elementSize;
    size_t bufferSize;
    FILE *file;
} recordsBufferStruct;

/*
 *  Returns true if file creation/read succesfull. False - if not.
 */
bool initStorage();

/*
 *  Increment packets num from this IP and iface 
 */
bool addIpAddr(uint32_t ipAddr, char * iface);

/*
 *  Returns packets num from this iface.
 */
uint64_t getIfaceStat(char * iface);

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
 *  Returns false if delete failed
 */
bool clearData();

/*
 * False if buffer fill error
 */
static bool safeReadFile(recordsBufferStruct *bufferStruct);

/*
 * False if file not written
 */
static bool safeWriteFile(recordsBufferStruct *bufferStruct);

#endif //_STORAGE_H_