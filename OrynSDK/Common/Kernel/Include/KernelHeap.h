#ifndef ORYN_KERNEL_HEAP_H
#define ORYN_KERNEL_HEAP_H

#include "KernelPhysicalMemory.h"
#include "KernelVirtualMemory.h"

#define ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT 4U
#define ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT 8U
#define ORYN_KERNEL_HEAP_OBJECT_CACHE_NAME_BYTES 32U
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
    unsigned long long RequestedAllocatedBytes;
    unsigned long long PeakRequestedAllocatedBytes;
    unsigned long long AllocationCount;
    unsigned long long FreeCount;
    unsigned long long ReallocationCount;
    unsigned long long CallocCount;
    unsigned long long ActiveAllocations;
    unsigned long long PeakActiveAllocations;
    unsigned long long ActiveRawAllocations;
    unsigned long long ActiveSlabAllocations;
    unsigned long long RawAllocationCount;
    unsigned long long RawFreeCount;
    unsigned long long LeakCounter;
    unsigned long long LeakBytes;
    unsigned long long PeakLeakBytes;
    unsigned long long LeakCheckRuns;
    unsigned long long LeakCheckFailures;
    unsigned long long FailedAllocations;
    unsigned long long SlabAllocations;
    unsigned long long SlabFrees;
    unsigned long long SlabCacheCount;
    unsigned long long ObjectCacheCount;
    unsigned long long ObjectCacheAllocations;
    unsigned long long ObjectCacheFrees;
    unsigned long long ActiveObjectCacheObjects;
    unsigned long long PeakActiveObjectCacheObjects;
    unsigned long long ObjectCacheValidationRuns;
    unsigned long long ObjectCacheValidationFailures;
    unsigned long long StackGuardPages;
    unsigned long long CriticalHeapGuardPages;
    unsigned long long InvalidFreeCount;
    unsigned long long DoubleFreeCount;
    unsigned long long CoalesceCount;
    unsigned long long ValidationRuns;
    unsigned long long ValidationFailures;
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

typedef struct OrynKernelObjectCacheStats
{
    char Name[ORYN_KERNEL_HEAP_OBJECT_CACHE_NAME_BYTES];
    unsigned int Configured;
    unsigned long long ObjectSize;
    unsigned long long Alignment;
    unsigned long long BackingSlabSize;
    unsigned long long AllocationCount;
    unsigned long long FreeCount;
    unsigned long long ActiveObjects;
    unsigned long long PeakActiveObjects;
    unsigned long long FailedAllocations;
    unsigned long long ValidationRuns;
    unsigned long long ValidationFailures;
} OrynKernelObjectCacheStats;

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
int OrynKernelHeapCreateObjectCache(unsigned int index, const char* name, unsigned long long objectSize, unsigned long long alignment);
void* OrynKernelHeapObjectAlloc(unsigned int index);
void OrynKernelHeapObjectFree(unsigned int index, void* pointer);
int OrynKernelHeapGetObjectCacheStats(unsigned int index, OrynKernelObjectCacheStats* stats);
int OrynKernelHeapHasLeaks(void);
int OrynKernelHeapCheckNoLeaks(void);
int OrynKernelHeapValidate(void);
int OrynKernelHeapRunSelfTest(void);
void OrynKernelHeapPrintProof(void);

#endif
