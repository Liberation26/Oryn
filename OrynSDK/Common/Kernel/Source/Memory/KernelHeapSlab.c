#include "KernelHeapInternal.h"

unsigned int OrynHeapFindSlabCache(unsigned long long size)
{
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT; ++index)
    {
        if (size <= gOrynSlabCaches[index].ObjectSize)
        {
            return index;
        }
    }
    return ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT;
}

static int GrowSlabCache(unsigned int cacheIndex)
{
    OrynKernelSlabCache* cache = &gOrynSlabCaches[cacheIndex];
    unsigned long long page = OrynHeapAllocatePage();
    if (page == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        gOrynHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    (void)OrynPhysicalMemorySetPageOwner(
        gOrynHeapPhysicalMemory,
        page,
        OrynPhysicalPageOwnerKernelHeap,
        ORYN_KERNEL_HEAP_FLAG_SLAB);

    unsigned long long objectBytes = cache->ObjectSize + sizeof(OrynKernelHeapBlock);
    unsigned long long objects = ORYN_PHYSICAL_PAGE_SIZE / objectBytes;
    if (objects == 0ULL)
    {
        (void)OrynPhysicalMemoryFreePage(gOrynHeapPhysicalMemory, page);
        return 0;
    }

    unsigned char* base = (unsigned char*)(unsigned long long)page;
    for (unsigned long long index = 0ULL; index < objects; ++index)
    {
        OrynKernelHeapBlock* block = (OrynKernelHeapBlock*)(base + index * objectBytes);
        block->Magic = ORYN_KERNEL_SLAB_MAGIC;
        block->Size = cache->ObjectSize;
        block->RequestedSize = 0ULL;
        block->Flags = ORYN_KERNEL_HEAP_FLAG_FREE | ORYN_KERNEL_HEAP_FLAG_SLAB;
        block->SlabCacheIndex = cacheIndex;
        OrynKernelSlabFreeObject* object = (OrynKernelSlabFreeObject*)OrynHeapBlockToPointer(block);
        object->Next = cache->FreeList;
        cache->FreeList = object;
        cache->Stats.FreeObjects += 1ULL;
    }

    cache->Stats.PagesAllocated += 1ULL;
    cache->Stats.ObjectsPerPage = objects;
    gOrynHeapStats.HeapPages += 1ULL;
    gOrynHeapStats.TotalBytes += ORYN_PHYSICAL_PAGE_SIZE;
    gOrynHeapStats.FreeBytes += objects * cache->ObjectSize;
    return 1;
}

void* OrynHeapSlabAllocate(unsigned int cacheIndex, unsigned long long requested)
{
    OrynKernelSlabCache* cache = &gOrynSlabCaches[cacheIndex];
    if (cache->FreeList == 0 && !GrowSlabCache(cacheIndex))
    {
        return 0;
    }

    OrynKernelSlabFreeObject* object = cache->FreeList;
    cache->FreeList = object->Next;
    OrynKernelHeapBlock* block = OrynHeapPointerToBlock(object);
    block->Flags &= ~ORYN_KERNEL_HEAP_FLAG_FREE;
    block->RequestedSize = requested;
    OrynHeapRemoveFreeBytes(block->Size);
    OrynHeapAddAllocatedBytes(block->Size);
    cache->Stats.FreeObjects -= 1ULL;
    cache->Stats.ActiveObjects += 1ULL;
    cache->Stats.AllocationCount += 1ULL;
    gOrynHeapStats.SlabAllocations += 1ULL;
    gOrynHeapStats.AllocationCount += 1ULL;
    gOrynHeapStats.ActiveAllocations += 1ULL;
    gOrynHeapStats.LeakCounter = gOrynHeapStats.ActiveAllocations;
    return object;
}

int OrynHeapSlabFree(void* pointer)
{
    OrynKernelHeapBlock* block = OrynHeapPointerToBlock(pointer);
    if (block == 0 || block->Magic != ORYN_KERNEL_SLAB_MAGIC)
    {
        return 0;
    }
    if ((block->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) != 0U)
    {
        gOrynHeapStats.DoubleFreeCount += 1ULL;
        return 1;
    }

    unsigned int cacheIndex = block->SlabCacheIndex;
    if (cacheIndex >= ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT)
    {
        gOrynHeapStats.InvalidFreeCount += 1ULL;
        return 1;
    }

    OrynKernelSlabCache* cache = &gOrynSlabCaches[cacheIndex];
    block->Flags |= ORYN_KERNEL_HEAP_FLAG_FREE;
    block->RequestedSize = 0ULL;
    OrynKernelSlabFreeObject* object = (OrynKernelSlabFreeObject*)pointer;
    object->Next = cache->FreeList;
    cache->FreeList = object;
    cache->Stats.FreeObjects += 1ULL;
    if (cache->Stats.ActiveObjects > 0ULL)
    {
        cache->Stats.ActiveObjects -= 1ULL;
    }
    cache->Stats.FreeCount += 1ULL;
    gOrynHeapStats.SlabFrees += 1ULL;
    gOrynHeapStats.FreeCount += 1ULL;
    OrynHeapRemoveAllocatedBytes(block->Size);
    OrynHeapAddFreeBytes(block->Size);
    if (gOrynHeapStats.ActiveAllocations > 0ULL)
    {
        gOrynHeapStats.ActiveAllocations -= 1ULL;
    }
    gOrynHeapStats.LeakCounter = gOrynHeapStats.ActiveAllocations;
    return 1;
}
