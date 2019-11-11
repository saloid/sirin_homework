#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <stdint.h>
#include <stdbool.h>

struct __attribute__((__packed__)) ipRecordStruct
{
    uint32_t ipAddr;
    uint64_t recordsNum;
};

struct __attribute__((__packed__)) ifaceRecordStruct
{
    char ifaceName[MAX_IFACE_LENGTH];
    uint64_t recordsNum;
};

/*
 *  Returns true if files open/creation succesfull. False - if not.
 */
bool initStorage();

/*
 *  Increment packets num from this IP and iface 
 */
void addIpAddr(uint32_t ipAddr, char * iface);

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
 * Close file
 */
void deinitStorage();

/*
 *  Delete all stored data
 *  Returns false if delete failed
 */
bool clearData();

#endif //_STORAGE_H_