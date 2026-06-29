#ifndef ORYN_KERNEL_MEMORY_MAP_H
#define ORYN_KERNEL_MEMORY_MAP_H

#include "OrynBootInfo.h"

#define ORYN_KERNEL_MEMORY_MAX_ENTRIES 256U
#define ORYN_KERNEL_PAGE_SIZE 4096ULL

typedef enum OrynKernelMemoryType
{
    OrynKernelMemoryReserved = 0,
    OrynKernelMemoryUsable = 1,
    OrynKernelMemoryBootloaderReclaim = 2,
    OrynKernelMemoryAcpiReclaim = 3,
    OrynKernelMemoryAcpiNvs = 4,
    OrynKernelMemoryMmio = 5,
    OrynKernelMemoryBad = 6,
    OrynKernelMemoryKernelReserved = 7,
    OrynKernelMemoryRuntime = 8
} OrynKernelMemoryType;

typedef struct OrynKernelMemoryEntry
{
    OrynKernelMemoryType Type;
    unsigned int SourceType;
    unsigned int Reserved;
    unsigned long long PhysicalStart;
    unsigned long long PageCount;
    unsigned long long Attribute;
} OrynKernelMemoryEntry;

typedef struct OrynKernelMemoryMap
{
    OrynKernelMemoryEntry Entries[ORYN_KERNEL_MEMORY_MAX_ENTRIES];
    unsigned int EntryCount;
    unsigned int SourceEntryCount;
    unsigned int Truncated;
    unsigned long long TotalBytes;
    unsigned long long UsableBytes;
    unsigned long long ReservedBytes;
    unsigned long long BootloaderReclaimBytes;
    unsigned long long AcpiReclaimBytes;
    unsigned long long AcpiNvsBytes;
    unsigned long long MmioBytes;
    unsigned long long BadBytes;
    unsigned long long KernelReservedBytes;
    unsigned long long RuntimeBytes;
} OrynKernelMemoryMap;

int OrynMemoryMapBuildFromBootInfo(const OrynBootInfo* bootInfo, OrynKernelMemoryMap* memoryMap);
void OrynMemoryMapPrintSummary(const OrynKernelMemoryMap* memoryMap);
const char* OrynMemoryMapTypeName(OrynKernelMemoryType type);

#endif
