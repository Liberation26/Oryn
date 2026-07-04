#include "KernelPhysicalMemory.h"

static unsigned long long AlignPhysicalPage(unsigned long long physicalAddress)
{
    return physicalAddress & ~(ORYN_PHYSICAL_PAGE_SIZE - 1ULL);
}

static int IsAlignedPhysicalPage(unsigned long long physicalAddress)
{
    return (physicalAddress & (ORYN_PHYSICAL_PAGE_SIZE - 1ULL)) == 0ULL;
}

static int FreeListContains(
    const OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress,
    unsigned int* matchCount)
{
    unsigned int matches = 0U;
    unsigned long long page = AlignPhysicalPage(physicalAddress);

    for (unsigned int index = 0U; index < allocator->FreePageCount; ++index)
    {
        if (allocator->FreePages[index] == page)
        {
            matches += 1U;
        }
    }

    if (matchCount != 0)
    {
        *matchCount = matches;
    }

    return matches != 0U;
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

static int ValidateFreePages(const OrynKernelPhysicalMemory* allocator)
{
    for (unsigned int index = 0U; index < allocator->FreePageCount; ++index)
    {
        unsigned int matches;
        const OrynPhysicalPageRecord* record;
        unsigned long long page = allocator->FreePages[index];

        if (page < ORYN_PHYSICAL_MIN_ALLOC_ADDRESS || !IsAlignedPhysicalPage(page))
        {
            return 0;
        }

        (void)FreeListContains(allocator, page, &matches);
        if (matches != 1U)
        {
            return 0;
        }

        record = FindRecord(allocator, page);
        if (record != 0 && record->Owner != OrynPhysicalPageOwnerFree)
        {
            return 0;
        }
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

        inFreeList = FreeListContains(allocator, record->PhysicalAddress, 0);
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

    if (writable != 0)
    {
        writable->IntegrityChecks += 1ULL;
    }

    ok = ValidatePageAccounting(allocator) &&
        ValidateFreePages(allocator) &&
        ValidateOwnershipRecords(allocator);

    if (!ok && writable != 0)
    {
        writable->IntegrityFailures += 1ULL;
    }

    return ok ? 1 : 0;
}
