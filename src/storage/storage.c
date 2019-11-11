#include "storage.h"
#include <stdio.h>
#include <stdlib.h>

recordsBufferStruct ipBuffer = {.bufferPtr = NULL,
                                .elementSize = sizeof(ipRecordStruct),
                                .bufferSize = 0,
                                .file = NULL};

recordsBufferStruct ifaceBuffer = {.bufferPtr = NULL,
                                   .elementSize = sizeof(ifaceRecordStruct),
                                   .bufferSize = 0,
                                   .file = NULL};

static bool safeReadFile(recordsBufferStruct *bufferStruct)
{
    long fileSize;

    fseek(bufferStruct->file, 0L, SEEK_END);
    fileSize = ftell(bufferStruct->file);
    rewind(bufferStruct->file);
    if (fileSize % (bufferStruct->elementSize) == 0)
    {
        bufferStruct->bufferPtr = malloc(fileSize);
        if (bufferStruct->bufferPtr == NULL)
        {
            printf("malloc error - critical");
            return (false);
        }
        bufferStruct->bufferSize = fileSize;
        if (fread(bufferStruct->bufferPtr, fileSize, 1, bufferStruct->file) != 1)
        {
            printf("file read error - file ignored\n");
        }
    }
    else
    {
        printf("Wrong file size (%ld) - file ignored\n", fileSize);
    }
}

static bool safeWriteFile(recordsBufferStruct *bufferStruct)
{
    size_t writtenSize;

    writtenSize = fwrite(bufferStruct->bufferPtr, bufferStruct->bufferSize, 1, bufferStruct->file);
    if (writtenSize != bufferStruct->bufferSize)
    {
        printf("File save error");
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
    ipBuffer.file = fopen(IP_LIST_FILENAME, "wb+");
    ifaceBuffer.file = fopen(IFACE_LIST_FILENAME, "wb+");

    if (ipBuffer.file == NULL || ifaceBuffer.file == NULL)
    {
        printf("file open/creation error");
        return (false);
    }

    if (safeReadFile(&ipBuffer) == false ||
        safeReadFile(&ifaceBuffer) == false)
    {
        printf("read file error");
        return (false);
    }

    return (true);
}

void deinitStorage()
{
    safeWriteFile(&ipBuffer);
    safeWriteFile(&ifaceBuffer);
}

bool clearData()
{
    ipBuffer.bufferPtr = realloc(ipBuffer.bufferPtr, 0);
    ipBuffer.bufferSize = 0;
    ifaceBuffer.bufferPtr = realloc(ifaceBuffer.bufferPtr, 0);
    ifaceBuffer.bufferSize = 0;
}

bool addIpAddr(uint32_t ipAddr, char *iface)
{

    /*
    ipBuffer.bufferPtr = realloc(ipBuffer.bufferPtr, (ipBuffer.bufferSize + sizeof(ipRecordStruct)));
    if (ipBuffer.bufferPtr == 0)
    {
        printf("New ip address add error - all data lost");
        return (false);
    }
    */
}

uint64_t getTotalPacketsNum()
{
    uint32_t ifacesNum = ifaceBuffer.bufferSize / sizeof(ifaceRecordStruct);
    ifaceRecordStruct *currentIfaceRecord = ifaceBuffer.bufferPtr;
    uint64_t totalPacketsNum = 0;
    
    for (uint32_t i = 0; i < ifacesNum; i++)
    {
        totalPacketsNum += currentIfaceRecord[i].recordsNum;
    }

    return (totalPacketsNum);
}
