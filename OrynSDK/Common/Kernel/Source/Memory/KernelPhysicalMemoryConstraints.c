#include "KernelPhysicalMemory.h"

static int PageMatchesConstraints(
    unsigned long long page,
    const OrynPhysicalAllocationConstraints* constraints)
{
    unsigned long long alignment;
    unsigned long long maxExclusive;
    if (constraints == 0)
    {
        return 1;
    }
    alignment = constraints->Alignment == 0ULL ? ORYN_PHYSICAL_PAGE_SIZE : constraints->Alignment;
    maxExclusive = constraints->MaxExclusiveAddress;
    if (constraints->DmaSafe != 0U && maxExclusive == 0ULL)
    {
        maxExclusive = ORYN_PHYSICAL_DMA_32BIT_LIMIT;
    }
    if (page < constraints->MinAddress)
    {
        return 0;
    }
    if (maxExclusive != 0ULL && page + ORYN_PHYSICAL_PAGE_SIZE > maxExclusive)
    {
        return 0;
    }
    if ((page & (alignment - 1ULL)) != 0ULL)
    {
        return 0;
    }
    return 1;
}

static unsigned int FindFreePageIndex(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long page)
{
    for (unsigned int index = 0U; index < allocator->FreePageCount; ++index)
    {
        if (allocator->FreePages[index] == page)
        {
            return index;
        }
    }
    return allocator->FreePageCount;
}

static void RemoveFreePageAt(OrynKernelPhysicalMemory* allocator, unsigned int index)
{
    allocator->FreePageCount -= 1U;
    allocator->FreePages[index] = allocator->FreePages[allocator->FreePageCount];
    allocator->UsedPageCount += 1U;
}

static unsigned int ConstraintOwner(const OrynPhysicalAllocationConstraints* constraints)
{
    if (constraints == 0 || constraints->Owner == 0U)
    {
        return OrynPhysicalPageOwnerGeneric;
    }
    return constraints->Owner;
}

static unsigned long long ConstraintTag(const OrynPhysicalAllocationConstraints* constraints)
{
    return constraints == 0 ? 0ULL : constraints->Tag;
}

unsigned long long OrynPhysicalMemoryAllocateConstrainedPage(
    OrynKernelPhysicalMemory* allocator,
    const OrynPhysicalAllocationConstraints* constraints)
{
    if (allocator == 0 || allocator->Initialized == 0U || allocator->FreePageCount == 0U)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }
    for (unsigned int index = allocator->FreePageCount; index > 0U; --index)
    {
        unsigned int pageIndex = index - 1U;
        unsigned long long page = allocator->FreePages[pageIndex];
        if (PageMatchesConstraints(page, constraints))
        {
            RemoveFreePageAt(allocator, pageIndex);
            allocator->ConstrainedAllocations += 1ULL;
            if (constraints != 0 && constraints->DmaSafe != 0U)
            {
                allocator->DmaSafeAllocations += 1ULL;
            }
            (void)OrynPhysicalMemorySetPageOwner(
                allocator,
                page,
                ConstraintOwner(constraints),
                ConstraintTag(constraints));
            OrynPhysicalMemoryPressureRefresh(allocator);
            return page;
        }
    }
    allocator->ConstrainedAllocationFailures += 1ULL;
    allocator->Pressure.AllocationFailures += 1ULL;
    OrynPhysicalMemoryPressureRefresh(allocator);
    return ORYN_PHYSICAL_ALLOC_FAIL;
}

unsigned long long OrynPhysicalMemoryAllocateDmaPage(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long maxExclusiveAddress,
    unsigned long long alignment)
{
    OrynPhysicalAllocationConstraints constraints;
    constraints.MinAddress = ORYN_PHYSICAL_MIN_ALLOC_ADDRESS;
    constraints.MaxExclusiveAddress = maxExclusiveAddress == 0ULL ? ORYN_PHYSICAL_DMA_32BIT_LIMIT : maxExclusiveAddress;
    constraints.Alignment = alignment == 0ULL ? ORYN_PHYSICAL_PAGE_SIZE : alignment;
    constraints.PageCount = 1U;
    constraints.RequireContiguous = 1U;
    constraints.DmaSafe = 1U;
    constraints.Owner = OrynPhysicalPageOwnerDma;
    constraints.Tag = constraints.MaxExclusiveAddress;
    return OrynPhysicalMemoryAllocateConstrainedPage(allocator, &constraints);
}

unsigned long long OrynPhysicalMemoryAllocateContiguousPages(
    OrynKernelPhysicalMemory* allocator,
    const OrynPhysicalAllocationConstraints* constraints)
{
    unsigned int pageCount;
    if (allocator == 0 || allocator->Initialized == 0U || constraints == 0)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }
    pageCount = constraints->PageCount == 0U ? 1U : constraints->PageCount;
    for (unsigned int baseIndex = 0U; baseIndex < allocator->FreePageCount; ++baseIndex)
    {
        unsigned long long base = allocator->FreePages[baseIndex];
        unsigned int found = 1U;
        if (!PageMatchesConstraints(base, constraints))
        {
            continue;
        }
        for (unsigned int offset = 1U; offset < pageCount; ++offset)
        {
            unsigned long long page = base + ((unsigned long long)offset * ORYN_PHYSICAL_PAGE_SIZE);
            if (FindFreePageIndex(allocator, page) == allocator->FreePageCount)
            {
                found = 0U;
                break;
            }
            if (!PageMatchesConstraints(page, constraints))
            {
                found = 0U;
                break;
            }
        }
        if (found != 0U)
        {
            for (unsigned int offset = 0U; offset < pageCount; ++offset)
            {
                unsigned long long page = base + ((unsigned long long)offset * ORYN_PHYSICAL_PAGE_SIZE);
                unsigned int removeIndex = FindFreePageIndex(allocator, page);
                RemoveFreePageAt(allocator, removeIndex);
                (void)OrynPhysicalMemorySetPageOwner(
                    allocator,
                    page,
                    ConstraintOwner(constraints),
                    ConstraintTag(constraints));
            }
            allocator->ConstrainedAllocations += 1ULL;
            allocator->ContiguousAllocationPages += pageCount;
            if (constraints->DmaSafe != 0U)
            {
                allocator->DmaSafeAllocations += pageCount;
            }
            OrynPhysicalMemoryPressureRefresh(allocator);
            return base;
        }
    }
    allocator->ConstrainedAllocationFailures += 1ULL;
    allocator->Pressure.AllocationFailures += 1ULL;
    OrynPhysicalMemoryPressureRefresh(allocator);
    return ORYN_PHYSICAL_ALLOC_FAIL;
}

int OrynPhysicalMemoryFreeContiguousPages(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress,
    unsigned int pageCount)
{
    unsigned int freed = 0U;
    if (pageCount == 0U)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < pageCount; ++index)
    {
        if (OrynPhysicalMemoryFreePage(
            allocator,
            physicalAddress + ((unsigned long long)index * ORYN_PHYSICAL_PAGE_SIZE)))
        {
            freed += 1U;
        }
    }
    return freed == pageCount;
}

