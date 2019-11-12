#include "storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>   //for strncmp, strncpy
#include <sys/stat.h> //for mkdir
#include <errno.h>	//strerror
#include <syslog.h>
//#include <sys/types.h>	//modes for mkdir

//static objects
/*
 * False if buffer fill error
 */
static bool safeReadFile(recordsBufferStruct *bufferStruct);

/*
 * False if file not written
 */
static bool safeWriteFile(recordsBufferStruct *bufferStruct);

static ipRecordStruct *getIpStructPtr(uint32_t ipAddr);

static const ipRecordStruct *lastSearchIpPointer = NULL;

/*
 *	For bsearch
 */
static int compareIp(const void *searchkey, const void *cmpelem);
static bool insertIpToBuffer(uint32_t ipAddr);

static recordsBufferStruct ipList = {.bufferPtr = NULL,
									 .elementSize = sizeof(ipRecordStruct),
									 .bufferSize = 0,
									 .file = NULL};

static recordsBufferStruct ifaceList = {.bufferPtr = NULL,
										.elementSize = sizeof(ifaceRecordStruct),
										.bufferSize = 0,
										.file = NULL};

static bool safeReadFile(recordsBufferStruct *bufferStruct)
{
	fseek(bufferStruct->file, 0L, SEEK_END);
	long fileSize = ftell(bufferStruct->file);
	rewind(bufferStruct->file);
	if (fileSize % (bufferStruct->elementSize) == 0)
	{
		bufferStruct->bufferPtr = malloc(fileSize);
		if (bufferStruct->bufferPtr == NULL)
		{
			syslog(LOG_ERR, "malloc error\n");
			return (false);
		}
		bufferStruct->bufferSize = fileSize;
		if (fread(bufferStruct->bufferPtr, fileSize, 1, bufferStruct->file) != 1)
		{
			syslog(LOG_WARNING, "file read error (maybe empty) - file ignored\n");
		}
	}
	else
	{
		syslog(LOG_WARNING, "Wrong file size (%ld) - file ignored\n", fileSize);
	}

	return (true);
}

static bool safeWriteFile(recordsBufferStruct *bufferStruct)
{
	size_t writtenSize;

	writtenSize = fwrite(bufferStruct->bufferPtr, bufferStruct->bufferSize, 1, bufferStruct->file);
	if (writtenSize != bufferStruct->bufferSize || fflush(bufferStruct->file) != 0)
	{
		syslog(LOG_ERR, "File save error: %s\n", strerror(errno));
		return (false);
	}
	//todo: write buffer to file

	fclose(bufferStruct->file);
	free(bufferStruct->bufferPtr);
	bufferStruct->bufferSize = 0;

	return (true);
}

bool initStorage()
{
	mkdir(FILES_PATH, 0777);
	ipList.file = fopen(IP_LIST_FILENAME, "wb+");
	ifaceList.file = fopen(IFACE_LIST_FILENAME, "wb+");

	if (ipList.file == NULL || ifaceList.file == NULL)
	{
		syslog(LOG_ERR, "file open/creation error: %s\n", strerror(errno));
		return (false);
	}

	if (safeReadFile(&ipList) == false ||
		safeReadFile(&ifaceList) == false)
	{
		syslog(LOG_ERR, "read file error\n");
		return (false);
	}

	return (true);
}

void deinitStorage()
{
	safeWriteFile(&ipList);
	safeWriteFile(&ifaceList);
}

void clearData()
{
	ipList.bufferPtr = realloc(ipList.bufferPtr, 0);
	ipList.bufferSize = 0;
	ifaceList.bufferPtr = realloc(ifaceList.bufferPtr, 0);
	ifaceList.bufferSize = 0;
}

uint64_t getIfaceStat(char *iface)
{
	uint32_t ifacesNum = ifaceList.bufferSize / sizeof(ifaceRecordStruct);
	ifaceRecordStruct *currentIfaceRecord = ifaceList.bufferPtr;

	for (uint32_t i = 0; i < ifacesNum; i++)
	{
		if (strncmp(currentIfaceRecord[i].ifaceName, iface, MAX_IFACE_LENGTH) == 0)
		{
			return (currentIfaceRecord[i].recordsNum);
		}
	}

	return (0); //not found
}

bool addIpAddr(uint32_t ipAddr, char *iface)
{
	//add ip addr
	ipRecordStruct *foundIpStruct;

	foundIpStruct = getIpStructPtr(ipAddr);
	if (foundIpStruct == NULL)
	{
		if (insertIpToBuffer(ipAddr) == false)
		{
			syslog(LOG_ERR, "adding new ip addr failed - all data lost\n");

			return (false);
		}
	}
	else
	{
		foundIpStruct->recordsNum++;
	}

	//add iface
	uint32_t ifacesNum = ifaceList.bufferSize / sizeof(ifaceRecordStruct);
	ifaceRecordStruct *currentIfaceRecord = ifaceList.bufferPtr;
	for (uint32_t i = 0; i < ifacesNum; i++)
	{
		if (strncmp(currentIfaceRecord[i].ifaceName, iface, MAX_IFACE_LENGTH) == 0)
		{
			currentIfaceRecord[i].recordsNum++;
			return (true);
		}
	}

	ifaceList.bufferPtr = realloc(ifaceList.bufferPtr, (ifaceList.bufferSize + sizeof(ifaceRecordStruct)));

	if (ifaceList.bufferPtr == NULL)
	{
		syslog(LOG_ERR, "New iface add error - all data lost\n");
		return (false);
	}
	currentIfaceRecord = ifaceList.bufferPtr + ifaceList.bufferSize;
	strncpy(currentIfaceRecord->ifaceName, iface, MAX_IFACE_LENGTH);
	currentIfaceRecord->recordsNum = 1;
	ifaceList.bufferSize += sizeof(ifaceRecordStruct);

	return (true);
}

uint64_t getPacketsNum(uint32_t ipAddr)
{
	ipRecordStruct *foundIpStruct = getIpStructPtr(ipAddr);
	if (foundIpStruct == NULL)
	{
		return (0);
	}
	else
	{
		return (foundIpStruct->recordsNum);
	}
}

static ipRecordStruct *getIpStructPtr(uint32_t ipAddr)
{
	if (ipList.bufferPtr == NULL)
	{
		return (NULL);
	}
	ipRecordStruct *foundPointer;

	foundPointer = bsearch(&ipAddr, ipList.bufferPtr, ipList.bufferSize / sizeof(ipRecordStruct), sizeof(ipRecordStruct), compareIp);

	return (foundPointer);
}

uint64_t getTotalPacketsNum()
{
	uint32_t ifacesNum = ifaceList.bufferSize / sizeof(ifaceRecordStruct);
	ifaceRecordStruct *currentIfaceRecord = ifaceList.bufferPtr;
	uint64_t totalPacketsNum = 0;

	for (uint32_t i = 0; i < ifacesNum; i++)
	{
		totalPacketsNum += currentIfaceRecord[i].recordsNum;
	}

	return (totalPacketsNum);
}

static int compareIp(const void *searchkey, const void *cmpelem)
{
	lastSearchIpPointer = cmpelem;
	return (*(uint32_t *)searchkey - *(uint32_t *)cmpelem);
}

static bool insertIpToBuffer(uint32_t ipAddr)
{
	uint32_t elementNum = 0;
	if (ipList.bufferPtr != NULL && lastSearchIpPointer != NULL)
	{
		elementNum = ((void *)lastSearchIpPointer - ipList.bufferPtr) / sizeof(ipRecordStruct);
		if (compareIp(&ipAddr, lastSearchIpPointer) > 0)
		{
			elementNum++;
		}
	}
	ipList.bufferPtr = realloc(ipList.bufferPtr, (ipList.bufferSize + sizeof(ipRecordStruct)));
	if (ipList.bufferPtr == NULL)
	{
		syslog(LOG_ERR, "New ip address add error - all data lost\n");
		return (false);
	}
	memmove(&((ipRecordStruct *)ipList.bufferPtr)[elementNum + 1], //should be safe, no need NULL check
			&((ipRecordStruct *)ipList.bufferPtr)[elementNum],
			ipList.bufferSize - elementNum * sizeof(ipRecordStruct));
	((ipRecordStruct *)ipList.bufferPtr)[elementNum].ipAddr = ipAddr;
	((ipRecordStruct *)ipList.bufferPtr)[elementNum].recordsNum = 1;
	ipList.bufferSize += sizeof(ipRecordStruct);

	return (true);
}