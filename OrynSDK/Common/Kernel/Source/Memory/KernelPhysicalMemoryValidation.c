#include "KernelPhysicalMemory.h"

static unsigned long long AlignPhysicalPage(unsigned long long physicalAddress)
{
    return physicalAddress & ~(ORYN_PHYSICAL_PAGE_SIZE - 1ULL);
}

static int IsAlignedPhysicalPage(unsigned long long physicalAddress)
{
    return (physicalAddress & (ORYN_PHYSICAL_PAGE_SIZE - 1ULL)) == 0ULL;
}

static void SwapFreePages(unsigned long long* left, unsigned long long* right)
{
    unsigned long long temp = *left;
    *left = *right;
    *right = temp;
}

static void HeapSiftDown(
    unsigned long long* pages,
    unsigned int start,
    unsigned int end)
{
    unsigned int root = start;

    while ((root * 2U) + 1U <= end)
    {
        unsigned int child = (root * 2U) + 1U;
        unsigned int swapIndex = root;

        if (pages[swapIndex] < pages[child])
        {
            swapIndex = child;
        }

        if (child + 1U <= end && pages[swapIndex] < pages[child + 1U])
        {
            swapIndex = child + 1U;
        }

        if (swapIndex == root)
        {
            return;
        }

        SwapFreePages(&pages[root], &pages[swapIndex]);
        root = swapIndex;
    }
}

static void SortFreePages(OrynKernelPhysicalMemory* allocator)
{
    unsigned int count;
    unsigned int start;
    unsigned int end;

    if (allocator == 0 || allocator->FreePageCount < 2U)
    {
        return;
    }

    count = allocator->FreePageCount;
    start = (count - 2U) / 2U;
    for (;;)
    {
        HeapSiftDown(allocator->FreePages, start, count - 1U);
        if (start == 0U)
        {
            break;
        }
        start -= 1U;
    }

    end = count - 1U;
    while (end > 0U)
    {
        SwapFreePages(&allocator->FreePages[end], &allocator->FreePages[0]);
        end -= 1U;
        HeapSiftDown(allocator->FreePages, 0U, end);
    }
}

static int SortedFreeListContains(
    const OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress)
{
    unsigned int low = 0U;
    unsigned int high;
    unsigned long long page = AlignPhysicalPage(physicalAddress);

    if (allocator == 0 || allocator->FreePageCount == 0U)
    {
        return 0;
    }

    high = allocator->FreePageCount;
    while (low < high)
    {
        unsigned int mid = low + ((high - low) / 2U);
        unsigned long long current = allocator->FreePages[mid];

        if (current == page)
        {
            return 1;
        }

        if (current < page)
        {
            low = mid + 1U;
        }
        else
        {
            high = mid;
        }
    }

    return 0;
}

static const OrynPhysicalPageRecord* FindRecord(
    const OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress)
{
    unsigned long long page = AlignPhysicalPage(physicalAddress);

    for (unsigned int index = 0U; index < allocator->PageRecordCount; ++index)
    {
        if (allocator->PageRecords[index].PhysicalAddress == page)
        {
            return &allocator->PageRecords[index];
        }
    }

    return 0;
}

static int ValidateFreePagesSorted(const OrynKernelPhysicalMemory* allocator)
{
    unsigned long long previous = 0ULL;

    for (unsigned int index = 0U; index < allocator->FreePageCount; ++index)
    {
        const OrynPhysicalPageRecord* record;
        unsigned long long page = allocator->FreePages[index];

        if (page < ORYN_PHYSICAL_MIN_ALLOC_ADDRESS || !IsAlignedPhysicalPage(page))
        {
            return 0;
        }

        if (index != 0U && page <= previous)
        {
            return 0;
        }

        record = FindRecord(allocator, page);
        if (record != 0 && record->Owner != OrynPhysicalPageOwnerFree)
        {
            return 0;
        }

        previous = page;
    }

    return 1;
}

static int ValidateOwnershipRecords(const OrynKernelPhysicalMemory* allocator)
{
    for (unsigned int index = 0U; index < allocator->PageRecordCount; ++index)
    {
        const OrynPhysicalPageRecord* record = &allocator->PageRecords[index];
        int inFreeList;

        if (!IsAlignedPhysicalPage(record->PhysicalAddress))
        {
            return 0;
        }

        inFreeList = SortedFreeListContains(allocator, record->PhysicalAddress);
        if (record->Owner == OrynPhysicalPageOwnerFree)
        {
            if (record->ReferenceCount != 0U || !inFreeList)
            {
                return 0;
            }
        }
        else
        {
            if (record->ReferenceCount == 0U || inFreeList)
            {
                return 0;
            }
        }
    }

    return 1;
}

static int ValidatePageAccounting(const OrynKernelPhysicalMemory* allocator)
{
    unsigned int accounted;

    if (allocator->FreePageCount > allocator->CapacityPages ||
        allocator->PageRecordCount > ORYN_PHYSICAL_MAX_OWNERSHIP_RECORDS)
    {
        return 0;
    }

    accounted = allocator->FreePageCount + allocator->UsedPageCount + allocator->ReservedPages;
    if (accounted > allocator->TrackedUsablePages)
    {
        return 0;
    }

    return allocator->PageSize == ORYN_PHYSICAL_PAGE_SIZE;
}

int OrynPhysicalMemoryValidateAllocator(const OrynKernelPhysicalMemory* allocator)
{
    OrynKernelPhysicalMemory* writable = (OrynKernelPhysicalMemory*)allocator;
    int ok;

    if (allocator == 0 || allocator->Initialized == 0U)
    {
        return 0;
    }

    writable->IntegrityChecks += 1ULL;
    SortFreePages(writable);
    ok = ValidatePageAccounting(allocator) &&
        ValidateFreePagesSorted(allocator) &&
        ValidateOwnershipRecords(allocator);

    if (!ok)
    {
        writable->IntegrityFailures += 1ULL;
    }

    return ok ? 1 : 0;
}
