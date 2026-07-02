#ifndef ORYN_KERNEL_PHYSICAL_MEMORY_H
#define ORYN_KERNEL_PHYSICAL_MEMORY_H

#include "KernelMemoryMap.h"

#define ORYN_PHYSICAL_PAGE_SIZE 4096ULL
#define ORYN_PHYSICAL_MAX_FREE_PAGES 1048576U
#define ORYN_PHYSICAL_MIN_ALLOC_ADDRESS 0x100000ULL
#define ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT 0x1000000000ULL

#define ORYN_PHYSICAL_ALLOC_FAIL 0ULL
#define ORYN_PHYSICAL_MAX_OWNERSHIP_RECORDS 8192U
#define ORYN_PHYSICAL_DMA_32BIT_LIMIT 0x100000000ULL
#define ORYN_PHYSICAL_DMA_24BIT_LIMIT 0x01000000ULL
#define ORYN_PHYSICAL_PRESSURE_LOW_PERCENT 20U
#define ORYN_PHYSICAL_PRESSURE_CRITICAL_PERCENT 5U

typedef enum OrynPhysicalPageOwner
{
    OrynPhysicalPageOwnerFree = 0,
    OrynPhysicalPageOwnerGeneric = 1,
    OrynPhysicalPageOwnerPageTable = 2,
    OrynPhysicalPageOwnerKernelHeap = 3,
    OrynPhysicalPageOwnerUserPage = 4,
    OrynPhysicalPageOwnerReserved = 5,
    OrynPhysicalPageOwnerDma = 6
} OrynPhysicalPageOwner;

typedef struct OrynPhysicalAllocationConstraints
{
    unsigned long long MinAddress;
    unsigned long long MaxExclusiveAddress;
    unsigned long long Alignment;
    unsigned int PageCount;
    unsigned int RequireContiguous;
    unsigned int DmaSafe;
    unsigned int Owner;
    unsigned long long Tag;
} OrynPhysicalAllocationConstraints;

typedef struct OrynPhysicalPageRecord
{
    unsigned long long PhysicalAddress;
    unsigned int Owner;
    unsigned int ReferenceCount;
    unsigned long long Tag;
} OrynPhysicalPageRecord;

typedef enum OrynPhysicalMemoryPressureLevel
{
    OrynPhysicalMemoryPressureNormal = 0,
    OrynPhysicalMemoryPressureLow = 1,
    OrynPhysicalMemoryPressureCritical = 2,
    OrynPhysicalMemoryPressureOutOfMemory = 3
} OrynPhysicalMemoryPressureLevel;

typedef enum OrynPhysicalOutOfMemoryAction
{
    OrynPhysicalOutOfMemoryActionContinue = 0,
    OrynPhysicalOutOfMemoryActionReclaim = 1,
    OrynPhysicalOutOfMemoryActionKillProcess = 2,
    OrynPhysicalOutOfMemoryActionKernelPanic = 3
} OrynPhysicalOutOfMemoryAction;

typedef struct OrynPhysicalMemoryPressureState
{
    unsigned int Initialized;
    unsigned int Level;
    unsigned int LastAction;
    unsigned int LowWatermarkPages;
    unsigned int CriticalWatermarkPages;
    unsigned int FreePages;
    unsigned int UsedPages;
    unsigned int TotalTrackedPages;
    unsigned long long AllocationFailures;
    unsigned long long LowTransitions;
    unsigned long long CriticalTransitions;
    unsigned long long OutOfMemoryEvents;
} OrynPhysicalMemoryPressureState;

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
    unsigned long long DmaPages;
    unsigned long long OwnershipRecordOverflows;
    unsigned long long OwnershipMismatches;
    unsigned long long ConstrainedAllocations;
    unsigned long long ConstrainedAllocationFailures;
    unsigned long long DmaSafeAllocations;
    unsigned long long ContiguousAllocationPages;
    OrynPhysicalMemoryPressureState Pressure;
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
    unsigned long long ConstrainedAllocations;
    unsigned long long ConstrainedAllocationFailures;
    unsigned long long DmaSafeAllocations;
    unsigned long long ContiguousAllocationPages;
    OrynPhysicalMemoryPressureState Pressure;
} OrynKernelPhysicalMemory;

int OrynPhysicalMemoryInit(const OrynKernelMemoryMap* memoryMap, OrynKernelPhysicalMemory* allocator);
unsigned long long OrynPhysicalMemoryAllocatePage(OrynKernelPhysicalMemory* allocator);
unsigned long long OrynPhysicalMemoryAllocatePageBelow(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long exclusiveLimit);
unsigned long long OrynPhysicalMemoryAllocateConstrainedPage(
    OrynKernelPhysicalMemory* allocator,
    const OrynPhysicalAllocationConstraints* constraints);
unsigned long long OrynPhysicalMemoryAllocateDmaPage(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long maxExclusiveAddress,
    unsigned long long alignment);
unsigned long long OrynPhysicalMemoryAllocateContiguousPages(
    OrynKernelPhysicalMemory* allocator,
    const OrynPhysicalAllocationConstraints* constraints);
int OrynPhysicalMemoryFreeContiguousPages(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress,
    unsigned int pageCount);
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
void OrynPhysicalMemoryPressureConfigure(
    OrynKernelPhysicalMemory* allocator,
    unsigned int lowWatermarkPages,
    unsigned int criticalWatermarkPages);
void OrynPhysicalMemoryPressureRefresh(OrynKernelPhysicalMemory* allocator);
const OrynPhysicalMemoryPressureState* OrynPhysicalMemoryGetPressureState(
    const OrynKernelPhysicalMemory* allocator);
unsigned int OrynPhysicalMemoryOutOfMemoryAction(
    const OrynKernelPhysicalMemory* allocator,
    unsigned int kernelRequest);
int OrynPhysicalMemoryRunPressureSelfTest(OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryPrintOwnershipDiagnostics(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryPrintSummary(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryPrintFinalState(const OrynKernelPhysicalMemory* allocator);
void OrynPhysicalMemoryRunSelfTest(OrynKernelPhysicalMemory* allocator);

#endif
