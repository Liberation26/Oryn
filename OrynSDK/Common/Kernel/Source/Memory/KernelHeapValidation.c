#include "KernelHeapInternal.h"

static int ValidateRawBlocks(
    unsigned long long* freeBytes,
    unsigned long long* allocatedBytes,
    unsigned long long* requestedBytes,
    unsigned long long* activeCount,
    unsigned long long* activeRawCount)
{
    OrynKernelHeapBlock* cursor = gOrynHeapHead;
    unsigned int guard = 0U;
    while (cursor != 0)
    {
        if (cursor->Magic != ORYN_KERNEL_HEAP_MAGIC)
        {
            return 0;
        }
        if (cursor->Next != 0 && cursor->Next->Previous != cursor)
        {
            return 0;
        }
        if ((cursor->Flags & ORYN_KERNEL_HEAP_FLAG_SLAB) != 0U)
        {
            return 0;
        }
        if ((cursor->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) != 0U)
        {
            *freeBytes += cursor->Size;
        }
        else
        {
            *allocatedBytes += cursor->Size;
            *requestedBytes += cursor->RequestedSize;
            *activeCount += 1ULL;
            *activeRawCount += 1ULL;
        }
        cursor = cursor->Next;
        guard += 1U;
        if (guard > 65535U)
        {
            return 0;
        }
    }
    return 1;
}

static int ValidateSlabCaches(
    unsigned long long* freeBytes,
    unsigned long long* allocatedBytes,
    unsigned long long* activeCount,
    unsigned long long* activeSlabCount)
{
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT; ++index)
    {
        OrynKernelSlabCache* cache = &gOrynSlabCaches[index];
        if (cache->ObjectSize == 0ULL || cache->Stats.ObjectSize != cache->ObjectSize)
        {
            return 0;
        }
        unsigned long long totalObjects = cache->Stats.FreeObjects + cache->Stats.ActiveObjects;
        unsigned long long capacity = cache->Stats.PagesAllocated * cache->Stats.ObjectsPerPage;
        if (totalObjects > capacity)
        {
            return 0;
        }
        *freeBytes += cache->Stats.FreeObjects * cache->ObjectSize;
        *allocatedBytes += cache->Stats.ActiveObjects * cache->ObjectSize;
        *activeCount += cache->Stats.ActiveObjects;
        *activeSlabCount += cache->Stats.ActiveObjects;
    }
    return 1;
}

int OrynKernelHeapValidate(void)
{
    unsigned long long freeBytes = 0ULL;
    unsigned long long allocatedBytes = 0ULL;
    unsigned long long requestedBytes = 0ULL;
    unsigned long long activeCount = 0ULL;
    unsigned long long activeRawCount = 0ULL;
    unsigned long long activeSlabCount = 0ULL;
    int ok = 1;

    gOrynHeapStats.ValidationRuns += 1ULL;
    if (gOrynHeapStats.Initialized == 0U)
    {
        ok = 0;
    }
    if (!ValidateRawBlocks(&freeBytes, &allocatedBytes, &requestedBytes, &activeCount, &activeRawCount))
    {
        ok = 0;
    }
    if (!ValidateSlabCaches(&freeBytes, &allocatedBytes, &activeCount, &activeSlabCount))
    {
        ok = 0;
    }
    if (allocatedBytes != gOrynHeapStats.AllocatedBytes)
    {
        ok = 0;
    }
    if (activeCount != gOrynHeapStats.ActiveAllocations)
    {
        ok = 0;
    }
    if (activeRawCount != gOrynHeapStats.ActiveRawAllocations)
    {
        ok = 0;
    }
    if (activeSlabCount != gOrynHeapStats.ActiveSlabAllocations)
    {
        ok = 0;
    }
    if (requestedBytes > gOrynHeapStats.RequestedAllocatedBytes)
    {
        ok = 0;
    }
    if (gOrynHeapStats.LeakCounter != gOrynHeapStats.ActiveAllocations)
    {
        ok = 0;
    }
    if (gOrynHeapStats.LeakBytes != gOrynHeapStats.RequestedAllocatedBytes)
    {
        ok = 0;
    }
    if (!OrynHeapValidateObjectCaches())
    {
        ok = 0;
    }
    if (!OrynKernelHeapValidateGuards())
    {
        ok = 0;
    }
    if (!ok)
    {
        gOrynHeapStats.ValidationFailures += 1ULL;
    }
    (void)freeBytes;
    return ok;
}

int OrynKernelHeapRunSelfTest(void)
{
    unsigned long long beforeDouble = gOrynHeapStats.DoubleFreeCount;
    unsigned long long beforeStackGuards = gOrynHeapStats.StackGuardPages;
    unsigned long long beforeCriticalGuards = gOrynHeapStats.CriticalHeapGuardPages;
    void* small = kmalloc(24ULL);
    void* medium = kmalloc(128ULL);
    unsigned char* zeroed = (unsigned char*)kcalloc(8ULL, 8ULL);
    char* grown = (char*)kmalloc(8ULL);
    void* critical = OrynKernelHeapAllocCritical(64ULL);
    void* stack = kcalloc(1ULL, ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE * 2ULL);

    if (small == 0 || medium == 0 || zeroed == 0 || grown == 0 || critical == 0 || stack == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < 64U; ++index)
    {
        if (zeroed[index] != 0U)
        {
            return 0;
        }
    }

    grown[0] = 'O';
    grown[1] = 'K';
    grown = (char*)krealloc(grown, 64ULL);
    if (grown == 0 || grown[0] != 'O' || grown[1] != 'K')
    {
        return 0;
    }

    kfree(small);
    kfree(medium);
    kfree(zeroed);
    kfree(grown);
    OrynKernelHeapInstallStackGuard((unsigned long long)stack, ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE * 2ULL);
    kfree(critical);
    kfree(critical);
    kfree(stack);

    if (gOrynHeapStats.DoubleFreeCount <= beforeDouble)
    {
        return 0;
    }
    if (gOrynHeapStats.StackGuardPages <= beforeStackGuards ||
        gOrynHeapStats.CriticalHeapGuardPages <= beforeCriticalGuards ||
        !OrynKernelHeapValidateGuards())
    {
        return 0;
    }
    if (!OrynHeapRunObjectCacheSelfTest())
    {
        return 0;
    }
    return OrynKernelHeapValidate() && OrynKernelHeapCheckNoLeaks();
}

void OrynKernelHeapPrintProof(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Heap pages: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.HeapPages);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap active allocations: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ActiveAllocations);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap peak active allocations: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.PeakActiveAllocations);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap leak counter: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.LeakCounter);
    OrynKernelDiagnosticsLogText(" leak bytes: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.LeakBytes);
    OrynKernelDiagnosticsLogText(" peak leak bytes: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.PeakLeakBytes);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap active raw/slab: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ActiveRawAllocations);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ActiveSlabAllocations);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap slab caches: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.SlabCacheCount);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap object caches active/peak: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ActiveObjectCacheObjects);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.PeakActiveObjectCacheObjects);
    OrynKernelDiagnosticsLogText(" alloc/free: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ObjectCacheAllocations);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ObjectCacheFrees);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap guard pages stack/critical/unmapped: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.StackGuardPages);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.CriticalHeapGuardPages);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.GuardPagesUnmapped);
    OrynKernelDiagnosticsLogText(" total: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.GuardPages);
    OrynKernelDiagnosticsLogText(" install failures: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.GuardInstallFailures);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap guard validation runs: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.GuardValidationRuns);
    OrynKernelDiagnosticsLogText(" failures: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.GuardValidationFailures);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap validation runs: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ValidationRuns);
    OrynKernelDiagnosticsLogText(" failures: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.ValidationFailures);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap leak checks: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.LeakCheckRuns);
    OrynKernelDiagnosticsLogText(" failures: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.LeakCheckFailures);
    OrynKernelDiagnosticsLogText("\n[KERNEL] Heap invalid frees: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.InvalidFreeCount);
    OrynKernelDiagnosticsLogText(" double frees: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.DoubleFreeCount);
    OrynKernelDiagnosticsLogText(" coalesces: ");
    OrynKernelDiagnosticsLogDec64(gOrynHeapStats.CoalesceCount);
    OrynKernelDiagnosticsLogText("\n");
}
