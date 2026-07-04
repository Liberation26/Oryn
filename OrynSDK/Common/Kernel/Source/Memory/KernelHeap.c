#include "KernelHeapInternal.h"

OrynKernelPhysicalMemory* gOrynHeapPhysicalMemory;
OrynKernelVirtualMemory* gOrynHeapVirtualMemory;
OrynKernelHeapBlock* gOrynHeapHead;
OrynKernelHeapStats gOrynHeapStats;
OrynKernelSlabCache gOrynSlabCaches[ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT];

unsigned long long OrynHeapAlignUp(unsigned long long value)
{
    return (value + ORYN_KERNEL_HEAP_ALIGN - 1ULL) & ~(ORYN_KERNEL_HEAP_ALIGN - 1ULL);
}

void OrynHeapClearMemory(void* pointer, unsigned long long size)
{
    if (pointer != 0 && size != 0ULL)
    {
        (void)memset(pointer, 0, (size_t)size);
    }
}

void OrynHeapCopyMemory(void* target, const void* source, unsigned long long size)
{
    if (target != 0 && source != 0 && size != 0ULL)
    {
        (void)memmove(target, source, (size_t)size);
    }
}

OrynKernelHeapBlock* OrynHeapPointerToBlock(void* pointer)
{
    if (pointer == 0)
    {
        return 0;
    }
    return ((OrynKernelHeapBlock*)pointer) - 1;
}

void* OrynHeapBlockToPointer(OrynKernelHeapBlock* block)
{
    if (block == 0)
    {
        return 0;
    }
    return (void*)(block + 1);
}

void OrynHeapAddFreeBytes(unsigned long long bytes)
{
    gOrynHeapStats.FreeBytes += bytes;
}

void OrynHeapRemoveFreeBytes(unsigned long long bytes)
{
    if (gOrynHeapStats.FreeBytes >= bytes)
    {
        gOrynHeapStats.FreeBytes -= bytes;
    }
    else
    {
        gOrynHeapStats.FreeBytes = 0ULL;
    }
}

void OrynHeapAddAllocatedBytes(unsigned long long bytes)
{
    gOrynHeapStats.AllocatedBytes += bytes;
    if (gOrynHeapStats.AllocatedBytes > gOrynHeapStats.PeakAllocatedBytes)
    {
        gOrynHeapStats.PeakAllocatedBytes = gOrynHeapStats.AllocatedBytes;
    }
}

void OrynHeapRemoveAllocatedBytes(unsigned long long bytes)
{
    if (gOrynHeapStats.AllocatedBytes >= bytes)
    {
        gOrynHeapStats.AllocatedBytes -= bytes;
    }
    else
    {
        gOrynHeapStats.AllocatedBytes = 0ULL;
    }
}

unsigned long long OrynHeapAllocatePage(void)
{
    if (gOrynHeapPhysicalMemory == 0)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }
    return OrynPhysicalMemoryAllocatePage(gOrynHeapPhysicalMemory);
}

int OrynKernelHeapInit(OrynKernelPhysicalMemory* physicalMemory)
{
    if (physicalMemory == 0 || physicalMemory->Initialized == 0U)
    {
        return 0;
    }

    (void)memset(&gOrynHeapStats, 0, sizeof(gOrynHeapStats));
    (void)memset(&gOrynSlabCaches, 0, sizeof(gOrynSlabCaches));
    gOrynHeapPhysicalMemory = physicalMemory;
    gOrynHeapVirtualMemory = 0;
    gOrynHeapHead = 0;
    gOrynHeapStats.Initialized = 1U;
    gOrynHeapStats.PageSize = ORYN_PHYSICAL_PAGE_SIZE;
    gOrynHeapStats.SlabCacheCount = ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT;

    const unsigned long long sizes[ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT] = { 32ULL, 64ULL, 128ULL, 256ULL };
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT; ++index)
    {
        gOrynSlabCaches[index].ObjectSize = sizes[index];
        gOrynSlabCaches[index].Stats.ObjectSize = sizes[index];
    }

    void* seed = OrynHeapAllocateRaw(ORYN_PHYSICAL_PAGE_SIZE / 2ULL, 0U);
    if (seed == 0)
    {
        return 0;
    }

    /*
     * Prime the raw heap with one physical page, then release the seed
     * allocation so the boot proof starts with an initialized heap and zero
     * active allocations.  Keeping the seed allocation live made the self-test
     * correctly report a leak and blocked VirtualMemory from starting.
     */
    OrynHeapFreeRaw(seed);
    return OrynKernelHeapValidate();
}

void OrynKernelHeapAttachVirtualMemory(OrynKernelVirtualMemory* virtualMemory)
{
    gOrynHeapVirtualMemory = virtualMemory;
}

void* kmalloc(unsigned long long size)
{
    unsigned int cacheIndex = OrynHeapFindSlabCache(size);
    if (cacheIndex < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT)
    {
        void* slab = OrynHeapSlabAllocate(cacheIndex, size);
        if (slab != 0)
        {
            return slab;
        }
    }
    return OrynHeapAllocateRaw(size, 0U);
}

void kfree(void* pointer)
{
    if (pointer == 0)
    {
        return;
    }
    if (OrynHeapSlabFree(pointer))
    {
        return;
    }
    OrynHeapFreeRaw(pointer);
}

void* krealloc(void* pointer, unsigned long long size)
{
    if (pointer == 0)
    {
        return kmalloc(size);
    }
    if (size == 0ULL)
    {
        kfree(pointer);
        return 0;
    }

    OrynKernelHeapBlock* block = OrynHeapPointerToBlock(pointer);
    unsigned long long oldSize = 0ULL;
    if (block != 0 && (block->Magic == ORYN_KERNEL_HEAP_MAGIC || block->Magic == ORYN_KERNEL_SLAB_MAGIC))
    {
        oldSize = block->RequestedSize;
        if (oldSize >= size)
        {
            block->RequestedSize = size;
            gOrynHeapStats.ReallocationCount += 1ULL;
            return pointer;
        }
    }

    void* replacement = kmalloc(size);
    if (replacement == 0)
    {
        return 0;
    }
    OrynHeapCopyMemory(replacement, pointer, oldSize);
    kfree(pointer);
    gOrynHeapStats.ReallocationCount += 1ULL;
    return replacement;
}

void* kcalloc(unsigned long long count, unsigned long long size)
{
    if (count != 0ULL && size > (~0ULL / count))
    {
        gOrynHeapStats.FailedAllocations += 1ULL;
        return 0;
    }
    unsigned long long total = count * size;
    void* pointer = kmalloc(total);
    if (pointer != 0)
    {
        OrynHeapClearMemory(pointer, total);
        gOrynHeapStats.CallocCount += 1ULL;
    }
    return pointer;
}

void* OrynKernelHeapAllocCritical(unsigned long long size)
{
    return OrynHeapAllocateRaw(size, ORYN_KERNEL_HEAP_FLAG_CRITICAL);
}

void OrynKernelHeapInstallStackGuard(unsigned long long stackBase, unsigned long long stackBytes)
{
    if (stackBase == 0ULL || stackBytes < ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE)
    {
        return;
    }
    if (gOrynHeapVirtualMemory != 0)
    {
        OrynVirtualMemoryUnmapGuardPage(stackBase);
    }
    gOrynHeapStats.GuardPages += 1ULL;
    gOrynHeapStats.StackGuardPages += 1ULL;
}

const OrynKernelHeapStats* OrynKernelHeapGetStats(void)
{
    return &gOrynHeapStats;
}

int OrynKernelHeapGetSlabCacheStats(unsigned int index, OrynKernelSlabCacheStats* stats)
{
    if (index >= ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT || stats == 0)
    {
        return 0;
    }
    *stats = gOrynSlabCaches[index].Stats;
    return 1;
}
