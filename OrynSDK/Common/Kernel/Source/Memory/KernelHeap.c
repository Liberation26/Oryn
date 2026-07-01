#include "KernelHeap.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"
#include "string.h"

#define ORYN_KERNEL_HEAP_MAGIC 0x4F52484D414C4C4FULL
#define ORYN_KERNEL_HEAP_FREE_MAGIC 0x4F5248465245454FULL
#define ORYN_KERNEL_SLAB_MAGIC 0x4F52534C41424C4FULL
#define ORYN_KERNEL_HEAP_MIN_SPLIT 64ULL

#define ORYN_KERNEL_HEAP_FLAG_FREE 0x00000001U
#define ORYN_KERNEL_HEAP_FLAG_SLAB 0x00000002U
#define ORYN_KERNEL_HEAP_FLAG_CRITICAL 0x00000004U

#define ORYN_KERNEL_HEAP_ALIGN 16ULL

typedef struct OrynKernelHeapBlock
{
    unsigned long long Magic;
    unsigned long long Size;
    unsigned long long RequestedSize;
    unsigned int Flags;
    unsigned int SlabCacheIndex;
    struct OrynKernelHeapBlock* Previous;
    struct OrynKernelHeapBlock* Next;
} OrynKernelHeapBlock;

typedef struct OrynKernelSlabFreeObject
{
    struct OrynKernelSlabFreeObject* Next;
} OrynKernelSlabFreeObject;

typedef struct OrynKernelSlabCache
{
    unsigned long long ObjectSize;
    OrynKernelSlabFreeObject* FreeList;
    OrynKernelSlabCacheStats Stats;
} OrynKernelSlabCache;

static OrynKernelPhysicalMemory* gPhysicalMemory;
static OrynKernelVirtualMemory* gVirtualMemory;
static OrynKernelHeapBlock* gHeapHead;
static OrynKernelHeapStats gHeapStats;
static OrynKernelSlabCache gSlabCaches[ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT];

static unsigned long long AlignUpHeap(unsigned long long value)
{
    return (value + ORYN_KERNEL_HEAP_ALIGN - 1ULL) & ~(ORYN_KERNEL_HEAP_ALIGN - 1ULL);
}

static void HeapLog(const char* text)
{
    OrynKernelDiagnosticsLogText(text);
}

static void ClearMemory(void* pointer, unsigned long long size)
{
    if (pointer != 0 && size != 0ULL)
    {
        (void)memset(pointer, 0, (size_t)size);
    }
}

static void CopyMemory(void* target, const void* source, unsigned long long size)
{
    if (target != 0 && source != 0 && size != 0ULL)
    {
        (void)memmove(target, source, (size_t)size);
    }
}

static OrynKernelHeapBlock* PointerToBlock(void* pointer)
{
    if (pointer == 0)
    {
        return 0;
    }
    return ((OrynKernelHeapBlock*)pointer) - 1;
}

static void* BlockToPointer(OrynKernelHeapBlock* block)
{
    if (block == 0)
    {
        return 0;
    }
    return (void*)(block + 1);
}

static void LinkBlock(OrynKernelHeapBlock* block)
{
    block->Previous = 0;
    block->Next = gHeapHead;
    if (gHeapHead != 0)
    {
        gHeapHead->Previous = block;
    }
    gHeapHead = block;
}

static void AddFreeBytes(unsigned long long bytes)
{
    gHeapStats.FreeBytes += bytes;
}

static void RemoveFreeBytes(unsigned long long bytes)
{
    if (gHeapStats.FreeBytes >= bytes)
    {
        gHeapStats.FreeBytes -= bytes;
    }
    else
    {
        gHeapStats.FreeBytes = 0ULL;
    }
}

static void AddAllocatedBytes(unsigned long long bytes)
{
    gHeapStats.AllocatedBytes += bytes;
    if (gHeapStats.AllocatedBytes > gHeapStats.PeakAllocatedBytes)
    {
        gHeapStats.PeakAllocatedBytes = gHeapStats.AllocatedBytes;
    }
}

static void RemoveAllocatedBytes(unsigned long long bytes)
{
    if (gHeapStats.AllocatedBytes >= bytes)
    {
        gHeapStats.AllocatedBytes -= bytes;
    }
    else
    {
        gHeapStats.AllocatedBytes = 0ULL;
    }
}

static unsigned long long AllocateHeapPage(void)
{
    if (gPhysicalMemory == 0)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }
    return OrynPhysicalMemoryAllocatePage(gPhysicalMemory);
}

static OrynKernelHeapBlock* AddHeapSpan(unsigned long long requestedBytes, unsigned int flags)
{
    unsigned long long requiredBytes = AlignUpHeap(requestedBytes + sizeof(OrynKernelHeapBlock));
    unsigned long long pages = (requiredBytes + ORYN_PHYSICAL_PAGE_SIZE - 1ULL) / ORYN_PHYSICAL_PAGE_SIZE;
    unsigned long long totalBytes = pages * ORYN_PHYSICAL_PAGE_SIZE;
    unsigned long long firstPage = 0ULL;
    unsigned long long lowestPage = 0ULL;
    unsigned long long highestPage = 0ULL;
    unsigned long long guardBefore = 0ULL;
    unsigned long long guardAfter = 0ULL;
    unsigned long long allocatedPages[64];

    if (pages == 0ULL || pages > 64ULL)
    {
        gHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    if ((flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL) != 0U)
    {
        guardBefore = AllocateHeapPage();
        if (guardBefore == ORYN_PHYSICAL_ALLOC_FAIL)
        {
            gHeapStats.FailedAllocations += 1ULL;
            return 0;
        }
        (void)OrynPhysicalMemorySetPageOwner(
            gPhysicalMemory,
            guardBefore,
            OrynPhysicalPageOwnerReserved,
            ORYN_KERNEL_HEAP_FLAG_CRITICAL);
        gHeapStats.GuardPages += 1ULL;
        gHeapStats.CriticalHeapGuardPages += 1ULL;
        if (gVirtualMemory != 0)
        {
            OrynVirtualMemoryUnmapGuardPage(guardBefore);
        }
    }

    for (unsigned long long index = 0ULL; index < pages; ++index)
    {
        unsigned long long page = AllocateHeapPage();
        if (page == ORYN_PHYSICAL_ALLOC_FAIL)
        {
            for (unsigned long long rollback = 0ULL; rollback < index; ++rollback)
            {
                (void)OrynPhysicalMemoryFreePage(gPhysicalMemory, allocatedPages[rollback]);
            }
            gHeapStats.FailedAllocations += 1ULL;
            return 0;
        }
        allocatedPages[index] = page;
        if (lowestPage == 0ULL || page < lowestPage)
        {
            lowestPage = page;
        }
        if (page > highestPage)
        {
            highestPage = page;
        }
        (void)OrynPhysicalMemorySetPageOwner(
            gPhysicalMemory,
            page,
            OrynPhysicalPageOwnerKernelHeap,
            flags);
    }

    if (pages > 1ULL && highestPage - lowestPage != (pages - 1ULL) * ORYN_PHYSICAL_PAGE_SIZE)
    {
        for (unsigned long long index = 0ULL; index < pages; ++index)
        {
            (void)OrynPhysicalMemoryFreePage(gPhysicalMemory, allocatedPages[index]);
        }
        gHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    firstPage = lowestPage;

    if ((flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL) != 0U)
    {
        guardAfter = AllocateHeapPage();
        if (guardAfter == ORYN_PHYSICAL_ALLOC_FAIL)
        {
            for (unsigned long long index = 0ULL; index < pages; ++index)
            {
                (void)OrynPhysicalMemoryFreePage(gPhysicalMemory, allocatedPages[index]);
            }
            gHeapStats.FailedAllocations += 1ULL;
            return 0;
        }
        (void)OrynPhysicalMemorySetPageOwner(
            gPhysicalMemory,
            guardAfter,
            OrynPhysicalPageOwnerReserved,
            ORYN_KERNEL_HEAP_FLAG_CRITICAL);
        gHeapStats.GuardPages += 1ULL;
        gHeapStats.CriticalHeapGuardPages += 1ULL;
        if (gVirtualMemory != 0)
        {
            OrynVirtualMemoryUnmapGuardPage(guardAfter);
        }
    }

    (void)guardBefore;
    (void)guardAfter;

    OrynKernelHeapBlock* block = (OrynKernelHeapBlock*)(unsigned long long)firstPage;
    block->Magic = ORYN_KERNEL_HEAP_MAGIC;
    block->Size = totalBytes - sizeof(OrynKernelHeapBlock);
    block->RequestedSize = 0ULL;
    block->Flags = ORYN_KERNEL_HEAP_FLAG_FREE | flags;
    block->SlabCacheIndex = 0U;
    LinkBlock(block);

    gHeapStats.HeapPages += pages;
    gHeapStats.TotalBytes += block->Size;
    AddFreeBytes(block->Size);
    return block;
}

static void SplitBlockIfUseful(OrynKernelHeapBlock* block, unsigned long long requested)
{
    unsigned long long aligned = AlignUpHeap(requested);
    if (block == 0 || block->Size <= aligned + sizeof(OrynKernelHeapBlock) + ORYN_KERNEL_HEAP_MIN_SPLIT)
    {
        return;
    }

    unsigned char* raw = (unsigned char*)BlockToPointer(block);
    OrynKernelHeapBlock* split = (OrynKernelHeapBlock*)(raw + aligned);
    split->Magic = ORYN_KERNEL_HEAP_MAGIC;
    split->Size = block->Size - aligned - sizeof(OrynKernelHeapBlock);
    split->RequestedSize = 0ULL;
    split->Flags = ORYN_KERNEL_HEAP_FLAG_FREE;
    split->SlabCacheIndex = 0U;
    split->Previous = block;
    split->Next = block->Next;
    if (split->Next != 0)
    {
        split->Next->Previous = split;
    }
    block->Next = split;
    block->Size = aligned;
    AddFreeBytes(split->Size);
}

static OrynKernelHeapBlock* FindFreeBlock(unsigned long long size, unsigned int flags)
{
    OrynKernelHeapBlock* cursor = gHeapHead;
    while (cursor != 0)
    {
        if (cursor->Magic == ORYN_KERNEL_HEAP_MAGIC &&
            (cursor->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) != 0U &&
            (cursor->Flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL) == (flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL) &&
            cursor->Size >= size)
        {
            return cursor;
        }
        cursor = cursor->Next;
    }
    return 0;
}

static void* HeapAllocateRaw(unsigned long long size, unsigned int flags)
{
    if (size == 0ULL || gHeapStats.Initialized == 0U)
    {
        return 0;
    }

    unsigned long long aligned = AlignUpHeap(size);
    OrynKernelHeapBlock* block = FindFreeBlock(aligned, flags);
    if (block == 0)
    {
        block = AddHeapSpan(aligned, flags);
        if (block == 0)
        {
            gHeapStats.FailedAllocations += 1ULL;
            return 0;
        }
    }

    RemoveFreeBytes(block->Size);
    SplitBlockIfUseful(block, aligned);
    block->Flags &= ~ORYN_KERNEL_HEAP_FLAG_FREE;
    block->RequestedSize = size;
    AddAllocatedBytes(block->Size);
    gHeapStats.AllocationCount += 1ULL;
    gHeapStats.ActiveAllocations += 1ULL;
    gHeapStats.LeakCounter = gHeapStats.ActiveAllocations;
    return BlockToPointer(block);
}

static void HeapFreeRaw(void* pointer)
{
    OrynKernelHeapBlock* block = PointerToBlock(pointer);
    if (block == 0 || block->Magic != ORYN_KERNEL_HEAP_MAGIC)
    {
        return;
    }
    if ((block->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) != 0U)
    {
        return;
    }

    RemoveAllocatedBytes(block->Size);
    block->RequestedSize = 0ULL;
    block->Flags |= ORYN_KERNEL_HEAP_FLAG_FREE;
    block->Magic = ORYN_KERNEL_HEAP_FREE_MAGIC;
    block->Magic = ORYN_KERNEL_HEAP_MAGIC;
    AddFreeBytes(block->Size);
    gHeapStats.FreeCount += 1ULL;
    if (gHeapStats.ActiveAllocations > 0ULL)
    {
        gHeapStats.ActiveAllocations -= 1ULL;
    }
    gHeapStats.LeakCounter = gHeapStats.ActiveAllocations;
}

static unsigned int FindSlabCache(unsigned long long size)
{
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT; ++index)
    {
        if (size <= gSlabCaches[index].ObjectSize)
        {
            return index;
        }
    }
    return ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT;
}

static int GrowSlabCache(unsigned int cacheIndex)
{
    OrynKernelSlabCache* cache = &gSlabCaches[cacheIndex];
    unsigned long long page = AllocateHeapPage();
    if (page == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        gHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    unsigned long long objectBytes = cache->ObjectSize + sizeof(OrynKernelHeapBlock);
    unsigned long long objects = ORYN_PHYSICAL_PAGE_SIZE / objectBytes;
    if (objects == 0ULL)
    {
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
        OrynKernelSlabFreeObject* object = (OrynKernelSlabFreeObject*)BlockToPointer(block);
        object->Next = cache->FreeList;
        cache->FreeList = object;
        cache->Stats.FreeObjects += 1ULL;
    }

    cache->Stats.PagesAllocated += 1ULL;
    cache->Stats.ObjectsPerPage = objects;
    gHeapStats.HeapPages += 1ULL;
    gHeapStats.TotalBytes += ORYN_PHYSICAL_PAGE_SIZE;
    gHeapStats.FreeBytes += objects * cache->ObjectSize;
    return 1;
}

static void* SlabAllocate(unsigned int cacheIndex, unsigned long long requested)
{
    OrynKernelSlabCache* cache = &gSlabCaches[cacheIndex];
    if (cache->FreeList == 0 && !GrowSlabCache(cacheIndex))
    {
        return 0;
    }

    OrynKernelSlabFreeObject* object = cache->FreeList;
    cache->FreeList = object->Next;
    OrynKernelHeapBlock* block = PointerToBlock(object);
    block->Flags &= ~ORYN_KERNEL_HEAP_FLAG_FREE;
    block->RequestedSize = requested;
    RemoveFreeBytes(block->Size);
    AddAllocatedBytes(block->Size);
    cache->Stats.FreeObjects -= 1ULL;
    cache->Stats.ActiveObjects += 1ULL;
    cache->Stats.AllocationCount += 1ULL;
    gHeapStats.SlabAllocations += 1ULL;
    gHeapStats.AllocationCount += 1ULL;
    gHeapStats.ActiveAllocations += 1ULL;
    gHeapStats.LeakCounter = gHeapStats.ActiveAllocations;
    return object;
}

static int SlabFree(void* pointer)
{
    OrynKernelHeapBlock* block = PointerToBlock(pointer);
    if (block == 0 || block->Magic != ORYN_KERNEL_SLAB_MAGIC)
    {
        return 0;
    }
    if ((block->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) != 0U)
    {
        return 1;
    }

    unsigned int cacheIndex = block->SlabCacheIndex;
    if (cacheIndex >= ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT)
    {
        return 0;
    }

    OrynKernelSlabCache* cache = &gSlabCaches[cacheIndex];
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
    gHeapStats.SlabFrees += 1ULL;
    gHeapStats.FreeCount += 1ULL;
    RemoveAllocatedBytes(block->Size);
    AddFreeBytes(block->Size);
    if (gHeapStats.ActiveAllocations > 0ULL)
    {
        gHeapStats.ActiveAllocations -= 1ULL;
    }
    gHeapStats.LeakCounter = gHeapStats.ActiveAllocations;
    return 1;
}

int OrynKernelHeapInit(OrynKernelPhysicalMemory* physicalMemory)
{
    if (physicalMemory == 0 || physicalMemory->Initialized == 0U)
    {
        return 0;
    }

    (void)memset(&gHeapStats, 0, sizeof(gHeapStats));
    (void)memset(&gSlabCaches, 0, sizeof(gSlabCaches));
    gPhysicalMemory = physicalMemory;
    gVirtualMemory = 0;
    gHeapHead = 0;
    gHeapStats.Initialized = 1U;
    gHeapStats.PageSize = ORYN_PHYSICAL_PAGE_SIZE;
    gHeapStats.SlabCacheCount = ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT;

    const unsigned long long sizes[ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT] = { 32ULL, 64ULL, 128ULL, 256ULL };
    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT; ++index)
    {
        gSlabCaches[index].ObjectSize = sizes[index];
        gSlabCaches[index].Stats.ObjectSize = sizes[index];
    }

    return AddHeapSpan(ORYN_PHYSICAL_PAGE_SIZE, 0U) != 0;
}

void OrynKernelHeapAttachVirtualMemory(OrynKernelVirtualMemory* virtualMemory)
{
    gVirtualMemory = virtualMemory;
}

void* kmalloc(unsigned long long size)
{
    unsigned int cacheIndex = FindSlabCache(size);
    if (cacheIndex < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT)
    {
        void* slab = SlabAllocate(cacheIndex, size);
        if (slab != 0)
        {
            return slab;
        }
    }
    return HeapAllocateRaw(size, 0U);
}

void kfree(void* pointer)
{
    if (pointer == 0)
    {
        return;
    }
    if (SlabFree(pointer))
    {
        return;
    }
    HeapFreeRaw(pointer);
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

    OrynKernelHeapBlock* block = PointerToBlock(pointer);
    unsigned long long oldSize = 0ULL;
    if (block != 0 && (block->Magic == ORYN_KERNEL_HEAP_MAGIC || block->Magic == ORYN_KERNEL_SLAB_MAGIC))
    {
        oldSize = block->RequestedSize;
        if (oldSize >= size)
        {
            block->RequestedSize = size;
            gHeapStats.ReallocationCount += 1ULL;
            return pointer;
        }
    }

    void* replacement = kmalloc(size);
    if (replacement == 0)
    {
        return 0;
    }
    CopyMemory(replacement, pointer, oldSize);
    kfree(pointer);
    gHeapStats.ReallocationCount += 1ULL;
    return replacement;
}

void* kcalloc(unsigned long long count, unsigned long long size)
{
    if (count != 0ULL && size > (~0ULL / count))
    {
        gHeapStats.FailedAllocations += 1ULL;
        return 0;
    }
    unsigned long long total = count * size;
    void* pointer = kmalloc(total);
    if (pointer != 0)
    {
        ClearMemory(pointer, total);
        gHeapStats.CallocCount += 1ULL;
    }
    return pointer;
}

void* OrynKernelHeapAllocCritical(unsigned long long size)
{
    return HeapAllocateRaw(size, ORYN_KERNEL_HEAP_FLAG_CRITICAL);
}

void OrynKernelHeapInstallStackGuard(unsigned long long stackBase, unsigned long long stackBytes)
{
    if (stackBase == 0ULL || stackBytes < ORYN_KERNEL_HEAP_GUARD_PAGE_SIZE)
    {
        return;
    }
    unsigned long long guard = stackBase;
    if (gVirtualMemory != 0)
    {
        OrynVirtualMemoryUnmapGuardPage(guard);
    }
    gHeapStats.GuardPages += 1ULL;
    gHeapStats.StackGuardPages += 1ULL;
}

const OrynKernelHeapStats* OrynKernelHeapGetStats(void)
{
    return &gHeapStats;
}

int OrynKernelHeapGetSlabCacheStats(unsigned int index, OrynKernelSlabCacheStats* stats)
{
    if (index >= ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT || stats == 0)
    {
        return 0;
    }
    *stats = gSlabCaches[index].Stats;
    return 1;
}

int OrynKernelHeapRunSelfTest(void)
{
    void* small = kmalloc(24ULL);
    void* medium = kmalloc(128ULL);
    unsigned char* zeroed = (unsigned char*)kcalloc(8ULL, 8ULL);
    char* grown = (char*)kmalloc(8ULL);
    void* critical = OrynKernelHeapAllocCritical(64ULL);

    if (small == 0 || medium == 0 || zeroed == 0 || grown == 0 || critical == 0)
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
    kfree(critical);
    return gHeapStats.ActiveAllocations == 0ULL;
}

void OrynKernelHeapPrintProof(void)
{
    HeapLog("[KERNEL] Heap pages: ");
    OrynKernelDiagnosticsLogDec64(gHeapStats.HeapPages);
    HeapLog("\n");
    HeapLog("[KERNEL] Heap active allocations: ");
    OrynKernelDiagnosticsLogDec64(gHeapStats.ActiveAllocations);
    HeapLog("\n");
    HeapLog("[KERNEL] Heap leak counter: ");
    OrynKernelDiagnosticsLogDec64(gHeapStats.LeakCounter);
    HeapLog("\n");
    HeapLog("[KERNEL] Heap slab caches: ");
    OrynKernelDiagnosticsLogDec64(gHeapStats.SlabCacheCount);
    HeapLog("\n");
    HeapLog("[KERNEL] Heap guard pages: ");
    OrynKernelDiagnosticsLogDec64(gHeapStats.GuardPages);
    HeapLog("\n");

    for (unsigned int index = 0U; index < ORYN_KERNEL_HEAP_SLAB_CACHE_COUNT; ++index)
    {
        HeapLog("[KERNEL] Heap slab cache ");
        OrynKernelDiagnosticsLogDec64(index);
        HeapLog(": object size ");
        OrynKernelDiagnosticsLogDec64(gSlabCaches[index].ObjectSize);
        HeapLog(", pages ");
        OrynKernelDiagnosticsLogDec64(gSlabCaches[index].Stats.PagesAllocated);
        HeapLog(", active ");
        OrynKernelDiagnosticsLogDec64(gSlabCaches[index].Stats.ActiveObjects);
        HeapLog("\n");
    }
}
