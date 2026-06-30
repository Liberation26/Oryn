#ifndef ORYN_KERNEL_PHYSICAL_MEMORY_H
#define ORYN_KERNEL_PHYSICAL_MEMORY_H

#include "KernelMemoryMap.h"

#define ORYN_PHYSICAL_PAGE_SIZE 4096ULL
#define ORYN_PHYSICAL_MAX_FREE_PAGES 262144U
#define ORYN_PHYSICAL_MIN_ALLOC_ADDRESS 0x100000ULL
#define ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT 0x1000000000ULL

#define ORYN_PHYSICAL_ALLOC_FAIL 0ULL

typedef struct OrynKernelPhysicalMemory
{
    unsigned long long FreePages[ORYN_PHYSICAL_MAX_FREE_PAGES];
    unsigned int Initialized;
    unsigned int CapacityPages;
    unsigned int FreePageCount;
    unsigned int UsedPageCount;
    unsigned int TotalUsablePages;
    unsigned int TrackedUsablePages;
    unsigned int UntrackedUsablePages;
    unsigned int ReservedLowPages;
    unsigned int ReservedPages;
    unsigned long long PageSize;
    unsigned long long LowestFreeAddress;
    unsigned long long HighestFreeAddress;
} OrynKernelPhysicalMemory;

int OrynPhysicalMemoryInit(const OrynKernelMemoryMap* memoryMap, OrynKernelPhysicalMemory* allocator);
unsigned long long OrynPhysicalMemoryAllocatePage(OrynKernelPhysicalMemory* allocator);
unsigned long long OrynPhysicalMemoryAllocatePageBelow(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long exclusiveLimit);
unsigned int OrynPhysicalMemoryReserveRange(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalStart,
    unsigned long long byteCount);
int OrynPhysicalMemoryFreePage(OrynKernelPhysicalMemory* allocator, unsigned long long physicalAddress);
void OrynPhysicalMemoryPrintSummary(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryPrintFinalState(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryRunSelfTest(OrynKernelPhysicalMemory* allocator);

#endif
