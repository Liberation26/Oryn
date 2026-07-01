#ifndef ORYN_KERNEL_PHYSICAL_MEMORY_H
#define ORYN_KERNEL_PHYSICAL_MEMORY_H

#include "KernelMemoryMap.h"

#define ORYN_PHYSICAL_PAGE_SIZE 4096ULL
#define ORYN_PHYSICAL_MAX_FREE_PAGES 1048576U
#define ORYN_PHYSICAL_MIN_ALLOC_ADDRESS 0x100000ULL
#define ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT 0x1000000000ULL

#define ORYN_PHYSICAL_ALLOC_FAIL 0ULL
#define ORYN_PHYSICAL_MAX_OWNERSHIP_RECORDS 8192U

typedef enum OrynPhysicalPageOwner
{
    OrynPhysicalPageOwnerFree = 0,
    OrynPhysicalPageOwnerGeneric = 1,
    OrynPhysicalPageOwnerPageTable = 2,
    OrynPhysicalPageOwnerKernelHeap = 3,
    OrynPhysicalPageOwnerUserPage = 4,
    OrynPhysicalPageOwnerReserved = 5
} OrynPhysicalPageOwner;

typedef struct OrynPhysicalPageRecord
{
    unsigned long long PhysicalAddress;
    unsigned int Owner;
    unsigned int ReferenceCount;
    unsigned long long Tag;
} OrynPhysicalPageRecord;

typedef struct OrynPhysicalPageOwnershipStats
{
    unsigned int RecordsUsed;
    unsigned int RecordsCapacity;
    unsigned long long PagesWithReferences;
    unsigned long long TotalReferences;
    unsigned long long GenericPages;
    unsigned long long PageTablePages;
    unsigned long long KernelHeapPages;
    unsigned long long UserPages;
    unsigned long long ReservedPages;
    unsigned long long OwnershipRecordOverflows;
    unsigned long long OwnershipMismatches;
} OrynPhysicalPageOwnershipStats;

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
    OrynPhysicalPageRecord PageRecords[ORYN_PHYSICAL_MAX_OWNERSHIP_RECORDS];
    unsigned int PageRecordCount;
    unsigned long long OwnershipRecordOverflows;
    unsigned long long OwnershipMismatches;
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
int OrynPhysicalMemorySetPageOwner(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress,
    unsigned int owner,
    unsigned long long tag);
int OrynPhysicalMemoryAddPageReference(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress);
int OrynPhysicalMemoryReleasePageReference(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress);
int OrynPhysicalMemoryGetOwnershipStats(
    const OrynKernelPhysicalMemory* allocator,
    OrynPhysicalPageOwnershipStats* stats);
void OrynPhysicalMemoryPrintOwnershipDiagnostics(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryPrintSummary(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryPrintFinalState(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryRunSelfTest(OrynKernelPhysicalMemory* allocator);

#endif
