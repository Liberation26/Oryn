#include "KernelHeapInternal.h"

static void CopyCacheName(char* target, const char* source)
{
    unsigned int index = 0U;
    if (target == 0)
    {
        return;
    }
    while (index + 1U < ORYN_KERNEL_HEAP_OBJECT_CACHE_NAME_BYTES &&
           source != 0 &&
           source[index] != '\0')
    {
        target[index] = source[index];
        index += 1U;
    }
    target[index] = '\0';
}

static void UpdateObjectCachePeak(OrynKernelObjectCacheStats* cache)
{
    if (cache->ActiveObjects > cache->PeakActiveObjects)
    {
        cache->PeakActiveObjects = cache->ActiveObjects;
    }
    if (gOrynHeapStats.ActiveObjectCacheObjects > gOrynHeapStats.PeakActiveObjectCacheObjects)
    {
        gOrynHeapStats.PeakActiveObjectCacheObjects = gOrynHeapStats.ActiveObjectCacheObjects;
    }
}

int OrynKernelHeapCreateObjectCache(
    unsigned int index,
    const char* name,
    unsigned long long objectSize,
    unsigned long long alignment)
{
    if (index >= ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT || objectSize == 0ULL)
    {
        return 0;
    }
    if (alignment == 0ULL)
    {
        alignment = ORYN_KERNEL_HEAP_ALIGN;
    }
    objectSize = OrynHeapAlignUp(objectSize);
    unsigned int slabIndex = OrynHeapFindSlabCache(objectSize);
    if (slabIndex >= ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT)
    {
        return 0;
    }

    OrynKernelObjectCacheStats* cache = &gOrynObjectCaches[index];
    (void)memset(cache, 0, sizeof(*cache));
    CopyCacheName(cache->Name, name);
    cache->Configured = 1U;
    cache->ObjectSize = objectSize;
    cache->Alignment = alignment;
    cache->BackingSlabSize = gOrynSlabCaches[slabIndex].ObjectSize;
    return 1;
}

void* OrynKernelHeapObjectAlloc(unsigned int index)
{
    if (index >= ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT ||
        gOrynObjectCaches[index].Configured == 0U)
    {
        gOrynHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    OrynKernelObjectCacheStats* cache = &gOrynObjectCaches[index];
    unsigned int slabIndex = OrynHeapFindSlabCache(cache->ObjectSize);
    void* pointer = OrynHeapSlabAllocate(slabIndex, cache->ObjectSize);
    if (pointer == 0)
    {
        cache->FailedAllocations += 1ULL;
        return 0;
    }

    OrynKernelHeapBlock* block = OrynHeapPointerToBlock(pointer);
    if (block == 0 || block->Magic != ORYN_KERNEL_SLAB_MAGIC)
    {
        cache->FailedAllocations += 1ULL;
        return 0;
    }
    block->Flags |= ORYN_KERNEL_HEAP_FLAG_OBJECT_CACHE;
    block->ObjectCacheIndex = index;
    cache->AllocationCount += 1ULL;
    cache->ActiveObjects += 1ULL;
    gOrynHeapStats.ObjectCacheAllocations += 1ULL;
    gOrynHeapStats.ActiveObjectCacheObjects += 1ULL;
    UpdateObjectCachePeak(cache);
    return pointer;
}

void OrynKernelHeapObjectFree(unsigned int index, void* pointer)
{
    if (pointer == 0)
    {
        return;
    }
    if (index >= ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT ||
        gOrynObjectCaches[index].Configured == 0U)
    {
        gOrynHeapStats.InvalidFreeCount += 1ULL;
        return;
    }

    OrynKernelHeapBlock* block = OrynHeapPointerToBlock(pointer);
    if (block == 0 ||
        block->Magic != ORYN_KERNEL_SLAB_MAGIC ||
        block->ObjectCacheIndex != index ||
        (block->Flags & ORYN_KERNEL_HEAP_FLAG_OBJECT_CACHE) == 0U)
    {
        gOrynHeapStats.InvalidFreeCount += 1ULL;
        return;
    }

    OrynKernelObjectCacheStats* cache = &gOrynObjectCaches[index];
    block->Flags &= ~ORYN_KERNEL_HEAP_FLAG_OBJECT_CACHE;
    block->ObjectCacheIndex = ORYN_KERNEL_HEAP_NO_OBJECT_CACHE;
    if (cache->ActiveObjects > 0ULL)
    {
        cache->ActiveObjects -= 1ULL;
    }
    cache->FreeCount += 1ULL;
    gOrynHeapStats.ObjectCacheFrees += 1ULL;
    if (gOrynHeapStats.ActiveObjectCacheObjects > 0ULL)
    {
        gOrynHeapStats.ActiveObjectCacheObjects -= 1ULL;
    }
    OrynHeapSlabFree(pointer);
}

int OrynKernelHeapGetObjectCacheStats(unsigned int index, OrynKernelObjectCacheStats* stats)
{
    if (index >= ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT || stats == 0)
    {
        return 0;
    }
    *stats = gOrynObjectCaches[index];
    return 1;
}

int OrynHeapValidateObjectCaches(void)
{
    unsigned long long active = 0ULL;
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_OBJECT_CACHE_COUNT; ++index)
    {
        OrynKernelObjectCacheStats* cache = &gOrynObjectCaches[index];
        if (cache->Configured == 0U)
        {
            continue;
        }
        cache->ValidationRuns += 1ULL;
        if (cache->ObjectSize == 0ULL ||
            cache->BackingSlabSize < cache->ObjectSize ||
            cache->ActiveObjects > cache->AllocationCount ||
            cache->FreeCount > cache->AllocationCount)
        {
            cache->ValidationFailures += 1ULL;
            gOrynHeapStats.ObjectCacheValidationFailures += 1ULL;
            return 0;
        }
        active += cache->ActiveObjects;
    }
    gOrynHeapStats.ObjectCacheValidationRuns += 1ULL;
    return active == gOrynHeapStats.ActiveObjectCacheObjects;
}

int OrynHeapRunObjectCacheSelfTest(void)
{
    if (!OrynKernelHeapCreateObjectCache(0U, "Process", 96ULL, ORYN_KERNEL_HEAP_ALIGN))
    {
        return 0;
    }
    if (!OrynKernelHeapCreateObjectCache(1U, "Thread", 128ULL, ORYN_KERNEL_HEAP_ALIGN))
    {
        return 0;
    }
    void* process = OrynKernelHeapObjectAlloc(0U);
    void* thread = OrynKernelHeapObjectAlloc(1U);
    if (process == 0 || thread == 0)
    {
        return 0;
    }
    OrynKernelHeapObjectFree(0U, process);
    OrynKernelHeapObjectFree(1U, thread);
    return OrynHeapValidateObjectCaches();
}
