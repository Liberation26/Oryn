#ifndef ORYN_KERNEL_HEAP_H
#define ORYN_KERNEL_HEAP_H

#include "KernelPhysicalMemory.h"
#include "KernelVirtualMemory.h"

#define ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT 4U
#define ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE 4096ULL

typedef struct OrynKernelHeapStats
{
    unsigned int Initialized;
    unsigned long long PageSize;
    unsigned long long HeapPages;
    unsigned long long GuardPages;
    unsigned long long TotalBytes;
    unsigned long long FreeBytes;
    unsigned long long AllocatedBytes;
    unsigned long long PeakAllocatedBytes;
    unsigned long long AllocationCount;
    unsigned long long FreeCount;
    unsigned long long ReallocationCount;
    unsigned long long CallocCount;
    unsigned long long ActiveAllocations;
    unsigned long long LeakCounter;
    unsigned long long FailedAllocations;
    unsigned long long SlabAllocations;
    unsigned long long SlabFrees;
    unsigned long long SlabCacheCount;
    unsigned long long StackGuardPages;
    unsigned long long CriticalHeapGuardPages;
} OrynKernelHeapStats;

typedef struct OrynKernelSlabCacheStats
{
    unsigned long long ObjectSize;
    unsigned long long ObjectsPerPage;
    unsigned long long PagesAllocated;
    unsigned long long ActiveObjects;
    unsigned long long FreeObjects;
    unsigned long long AllocationCount;
    unsigned long long FreeCount;
} OrynKernelSlabCacheStats;

int OrynKernelHeapInit(OrynKernelPhysicalMemory* physicalMemory);
void OrynKernelHeapAttachVirtualMemory(OrynKernelVirtualMemory* virtualMemory);
void* kmalloc(unsigned long long size);
void kfree(void* pointer);
void* krealloc(void* pointer, unsigned long long size);
void* kcalloc(unsigned long long count, unsigned long long size);
void* OrynKernelHeapAllocCritical(unsigned long long size);
void OrynKernelHeapInstallStackGuard(unsigned long long stackBase, unsigned long long stackBytes);
const OrynKernelHeapStats* OrynKernelHeapGetStats(void);
int OrynKernelHeapGetSlabCacheStats(unsigned int index, OrynKernelSlabCacheStats* stats);
int OrynKernelHeapRunSelfTest(void);
void OrynKernelHeapPrintProof(void);

#endif
