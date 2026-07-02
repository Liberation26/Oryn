#ifndef ORYN_KERNEL_PARTITION_H
#define ORYN_KERNEL_PARTITION_H

#include "KernelBlockDevice.h"

#define ORYN_KERNEL_PARTITION_MAX_ENTRIES 32U

typedef enum OrynKernelPartitionTableType
{
    OrynKernelPartitionTableNone = 0,
    OrynKernelPartitionTableMbr = 1,
    OrynKernelPartitionTableGpt = 2
} OrynKernelPartitionTableType;

typedef struct OrynKernelPartitionEntry
{
    uint32_t Present;
    uint32_t Index;
    uint32_t MbrType;
    uint64_t FirstLba;
    uint64_t BlockCount;
    uint8_t TypeGuid[16];
    uint8_t UniqueGuid[16];
} OrynKernelPartitionEntry;

typedef struct OrynKernelPartitionTable
{
    OrynKernelPartitionTableType Type;
    uint32_t Valid;
    uint32_t EntryCount;
    uint32_t ParseAttempts;
    uint32_t ParseFailures;
    uint32_t MbrParsed;
    uint32_t GptParsed;
    OrynKernelPartitionEntry Entries[ORYN_KERNEL_PARTITION_MAX_ENTRIES];
} OrynKernelPartitionTable;

void OrynKernelPartitionTableInit(OrynKernelPartitionTable* table);
int OrynKernelPartitionParse(OrynKernelBlockDevice* device, OrynKernelPartitionTable* table);
int OrynKernelPartitionParseMbr(OrynKernelBlockDevice* device, OrynKernelPartitionTable* table);
int OrynKernelPartitionParseGpt(OrynKernelBlockDevice* device, OrynKernelPartitionTable* table);
const OrynKernelPartitionEntry* OrynKernelPartitionGetEntry(const OrynKernelPartitionTable* table, uint32_t index);

#endif
