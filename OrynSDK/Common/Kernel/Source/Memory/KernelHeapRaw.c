#include "KernelHeapInternal.h"

static void LinkBlock(OrynKernelHeapBlock* block)
{
    block->Previous = 0;
    block->Next = gOrynHeapHead;
    if (gOrynHeapHead != 0)
    {
        gOrynHeapHead->Previous = block;
    }
    gOrynHeapHead = block;
}

static void UnlinkBlock(OrynKernelHeapBlock* block)
{
    if (block == 0)
    {
        return;
    }
    if (block->Previous != 0)
    {
        block->Previous->Next = block->Next;
    }
    else
    {
        gOrynHeapHead = block->Next;
    }
    if (block->Next != 0)
    {
        block->Next->Previous = block->Previous;
    }
}

static int BlocksTouch(OrynKernelHeapBlock* first, OrynKernelHeapBlock* second)
{
    unsigned long long firstEnd = (unsigned long long)OrynHeapBlockToPointer(first) + first->Size;
    return firstEnd == (unsigned long long)second;
}

static void CoalesceWithNext(OrynKernelHeapBlock* block)
{
    OrynKernelHeapBlock* next = block == 0 ? 0 : block->Next;
    if (block == 0 || next == 0)
    {
        return;
    }
    if ((block->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) == 0U ||
        (next->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) == 0U ||
        (block->Flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL) != (next->Flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL) ||
        !BlocksTouch(block, next))
    {
        return;
    }

    block->Size += sizeof(OrynKernelHeapBlock) + next->Size;
    UnlinkBlock(next);
    gOrynHeapStats.CoalesceCount += 1ULL;
}

static OrynKernelHeapBlock* AddHeapSpan(unsigned long long requestedBytes, unsigned int flags)
{
    unsigned long long requiredBytes = OrynHeapAlignUp(requestedBytes + sizeof(OrynKernelHeapBlock));
    unsigned long long pages = (requiredBytes + ORYN_PHYSICAL_PAGE_SIZE - 1ULL) / ORYN_PHYSICAL_PAGE_SIZE;
    unsigned long long lowestPage = 0ULL;
    unsigned long long highestPage = 0ULL;
    unsigned long long allocatedPages[64];

    if (pages == 0ULL || pages > 64ULL)
    {
        gOrynHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    for (unsigned long long index = 0ULL; index < pages; ++index)
    {
        unsigned long long page = OrynHeapAllocatePage();
        if (page == ORYN_PHYSICAL_ALLOC_FAIL)
        {
            for (unsigned long long rollback = 0ULL; rollback < index; ++rollback)
            {
                (void)OrynPhysicalMemoryFreePage(gOrynHeapPhysicalMemory, allocatedPages[rollback]);
            }
            gOrynHeapStats.FailedAllocations += 1ULL;
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
        (void)OrynPhysicalMemorySetPageOwner(gOrynHeapPhysicalMemory, page, OrynPhysicalPageOwnerKernelHeap, flags);
    }

    if (pages > 1ULL && highestPage - lowestPage != (pages - 1ULL) * ORYN_PHYSICAL_PAGE_SIZE)
    {
        for (unsigned long long index = 0ULL; index < pages; ++index)
        {
            (void)OrynPhysicalMemoryFreePage(gOrynHeapPhysicalMemory, allocatedPages[index]);
        }
        gOrynHeapStats.FailedAllocations += 1ULL;
        return 0;
    }

    OrynKernelHeapBlock* block = (OrynKernelHeapBlock*)(unsigned long long)lowestPage;
    block->Magic = ORYN_KERNEL_HEAP_MAGIC;
    block->Size = pages * ORYN_PHYSICAL_PAGE_SIZE - sizeof(OrynKernelHeapBlock);
    block->RequestedSize = 0ULL;
    block->Flags = ORYN_KERNEL_HEAP_FLAG_FREE | flags;
    block->SlabCacheIndex = 0U;
    block->ObjectCacheIndex = ORYN_KERNEL_HEAP_NO_OBJECT_CACHE;
    LinkBlock(block);
    gOrynHeapStats.HeapPages += pages;
    gOrynHeapStats.TotalBytes += block->Size;
    OrynHeapAddFreeBytes(block->Size);
    return block;
}

static void SplitBlockIfUseful(OrynKernelHeapBlock* block, unsigned long long requested)
{
    unsigned long long aligned = OrynHeapAlignUp(requested);
    if (block == 0 || block->Size <= aligned + sizeof(OrynKernelHeapBlock) + ORYN_KERNEL_HEAP_MIN_SPLIT)
    {
        return;
    }

    unsigned char* raw = (unsigned char*)OrynHeapBlockToPointer(block);
    OrynKernelHeapBlock* split = (OrynKernelHeapBlock*)(raw + aligned);
    split->Magic = ORYN_KERNEL_HEAP_MAGIC;
    split->Size = block->Size - aligned - sizeof(OrynKernelHeapBlock);
    split->RequestedSize = 0ULL;
    split->Flags = ORYN_KERNEL_HEAP_FLAG_FREE | (block->Flags & ORYN_KERNEL_HEAP_FLAG_CRITICAL);
    split->SlabCacheIndex = 0U;
    split->ObjectCacheIndex = ORYN_KERNEL_HEAP_NO_OBJECT_CACHE;
    split->Previous = block;
    split->Next = block->Next;
    if (split->Next != 0)
    {
        split->Next->Previous = split;
    }
    block->Next = split;
    block->Size = aligned;
    OrynHeapAddFreeBytes(split->Size);
}

static OrynKernelHeapBlock* FindFreeBlock(unsigned long long size, unsigned int flags)
{
    OrynKernelHeapBlock* cursor = gOrynHeapHead;
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

void* OrynHeapAllocateRaw(unsigned long long size, unsigned int flags)
{
    if (size == 0ULL || gOrynHeapStats.Initialized == 0U)
    {
        return 0;
    }

    unsigned long long aligned = OrynHeapAlignUp(size);
    OrynKernelHeapBlock* block = FindFreeBlock(aligned, flags);
    if (block == 0)
    {
        block = AddHeapSpan(aligned, flags);
        if (block == 0)
        {
            gOrynHeapStats.FailedAllocations += 1ULL;
            return 0;
        }
    }

    OrynHeapRemoveFreeBytes(block->Size);
    SplitBlockIfUseful(block, aligned);
    block->Flags &= ~ORYN_KERNEL_HEAP_FLAG_FREE;
    block->RequestedSize = size;
    block->ObjectCacheIndex = ORYN_KERNEL_HEAP_NO_OBJECT_CACHE;
    OrynHeapAddAllocatedBytes(block->Size);
    OrynHeapAddRequestedBytes(size);
    gOrynHeapStats.AllocationCount += 1ULL;
    gOrynHeapStats.RawAllocationCount += 1ULL;
    gOrynHeapStats.ActiveAllocations += 1ULL;
    gOrynHeapStats.ActiveRawAllocations += 1ULL;
    OrynHeapRefreshLeakCounters();
    return OrynHeapBlockToPointer(block);
}

void OrynHeapFreeRaw(void* pointer)
{
    OrynKernelHeapBlock* block = OrynHeapPointerToBlock(pointer);
    if (block == 0 || block->Magic != ORYN_KERNEL_HEAP_MAGIC)
    {
        gOrynHeapStats.InvalidFreeCount += 1ULL;
        return;
    }
    if ((block->Flags & ORYN_KERNEL_HEAP_FLAG_FREE) != 0U)
    {
        gOrynHeapStats.DoubleFreeCount += 1ULL;
        return;
    }

    unsigned long long requested = block->RequestedSize;
    OrynHeapRemoveAllocatedBytes(block->Size);
    OrynHeapRemoveRequestedBytes(requested);
    block->RequestedSize = 0ULL;
    block->ObjectCacheIndex = ORYN_KERNEL_HEAP_NO_OBJECT_CACHE;
    block->Flags |= ORYN_KERNEL_HEAP_FLAG_FREE;
    OrynHeapAddFreeBytes(block->Size);
    gOrynHeapStats.FreeCount += 1ULL;
    gOrynHeapStats.RawFreeCount += 1ULL;
    if (gOrynHeapStats.ActiveAllocations > 0ULL)
    {
        gOrynHeapStats.ActiveAllocations -= 1ULL;
    }
    if (gOrynHeapStats.ActiveRawAllocations > 0ULL)
    {
        gOrynHeapStats.ActiveRawAllocations -= 1ULL;
    }
    OrynHeapRefreshLeakCounters();
    CoalesceWithNext(block);
}
