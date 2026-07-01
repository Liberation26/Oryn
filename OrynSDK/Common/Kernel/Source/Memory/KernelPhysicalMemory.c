#include "KernelPhysicalMemory.h"

static void ClearAllocator(OrynKernelPhysicalMemory* allocator)
{
    unsigned int index;

    for (index = 0; index < sizeof(*allocator); ++index)
    {
        ((unsigned char*)allocator)[index] = 0U;
    }

    allocator->CapacityPages = ORYN_PHYSICAL_MAX_FREE_PAGES;
    allocator->PageSize = ORYN_PHYSICAL_PAGE_SIZE;
}

static unsigned long long AlignUp(unsigned long long value)
{
    return (value + ORYN_PHYSICAL_PAGE_SIZE - 1ULL) & ~(ORYN_PHYSICAL_PAGE_SIZE - 1ULL);
}

static unsigned long long AlignDown(unsigned long long value)
{
    return value & ~(ORYN_PHYSICAL_PAGE_SIZE - 1ULL);
}

static int IsAlignedPage(unsigned long long physicalAddress)
{
    return (physicalAddress & (ORYN_PHYSICAL_PAGE_SIZE - 1ULL)) == 0ULL;
}

static void TrackAddressRange(OrynKernelPhysicalMemory* allocator, unsigned long long address)
{
    if (allocator->TrackedUsablePages == 0U)
    {
        allocator->LowestFreeAddress = address;
        allocator->HighestFreeAddress = address;
        return;
    }

    if (address < allocator->LowestFreeAddress)
    {
        allocator->LowestFreeAddress = address;
    }

    if (address > allocator->HighestFreeAddress)
    {
        allocator->HighestFreeAddress = address;
    }
}

static void AddUsablePage(OrynKernelPhysicalMemory* allocator, unsigned long long address)
{
    allocator->TotalUsablePages += 1U;

    if (address < ORYN_PHYSICAL_MIN_ALLOC_ADDRESS)
    {
        allocator->ReservedLowPages += 1U;
        return;
    }

    if (allocator->FreePageCount >= allocator->CapacityPages)
    {
        allocator->UntrackedUsablePages += 1U;
        return;
    }

    TrackAddressRange(allocator, address);
    allocator->FreePages[allocator->FreePageCount] = address;
    allocator->FreePageCount += 1U;
    allocator->TrackedUsablePages += 1U;
}

static void AddUsableEntry(OrynKernelPhysicalMemory* allocator, const OrynKernelMemoryEntry* entry)
{
    unsigned long long start = AlignUp(entry->PhysicalStart);
    unsigned long long end = AlignDown(entry->PhysicalStart + (entry->PageCount * ORYN_PHYSICAL_PAGE_SIZE));
    unsigned long long address;

    if (end <= start)
    {
        return;
    }

    for (address = start; address < end; address += ORYN_PHYSICAL_PAGE_SIZE)
    {
        AddUsablePage(allocator, address);
    }
}

int OrynPhysicalMemoryInit(const OrynKernelMemoryMap* memoryMap, OrynKernelPhysicalMemory* allocator)
{
    unsigned int index;

    if (allocator == 0)
    {
        return 0;
    }

    ClearAllocator(allocator);

    if (memoryMap == 0 || memoryMap->EntryCount == 0U)
    {
        return 0;
    }

    for (index = 0; index < memoryMap->EntryCount; ++index)
    {
        const OrynKernelMemoryEntry* entry = &memoryMap->Entries[index];
        if (entry->Type == OrynKernelMemoryUsable)
        {
            AddUsableEntry(allocator, entry);
        }
    }

    allocator->Initialized = allocator->TrackedUsablePages > 0U ? 1U : 0U;
    return allocator->Initialized ? 1 : 0;
}

unsigned long long OrynPhysicalMemoryAllocatePageBelow(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long exclusiveLimit)
{
    if (allocator == 0 || allocator->Initialized == 0U || allocator->FreePageCount == 0U)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }

    for (unsigned int index = allocator->FreePageCount; index > 0U; --index)
    {
        unsigned int pageIndex = index - 1U;
        unsigned long long page = allocator->FreePages[pageIndex];
        if (page < exclusiveLimit)
        {
            allocator->FreePageCount -= 1U;
            allocator->FreePages[pageIndex] = allocator->FreePages[allocator->FreePageCount];
            allocator->UsedPageCount += 1U;
            (void)OrynPhysicalMemorySetPageOwner(allocator, page, OrynPhysicalPageOwnerGeneric, 0ULL);
            return page;
        }
    }

    return ORYN_PHYSICAL_ALLOC_FAIL;
}

unsigned long long OrynPhysicalMemoryAllocatePage(OrynKernelPhysicalMemory* allocator)
{
    if (allocator == 0 || allocator->Initialized == 0U || allocator->FreePageCount == 0U)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }

    allocator->FreePageCount -= 1U;
    allocator->UsedPageCount += 1U;
    unsigned long long page = allocator->FreePages[allocator->FreePageCount];
    (void)OrynPhysicalMemorySetPageOwner(allocator, page, OrynPhysicalPageOwnerGeneric, 0ULL);
    return page;
}

unsigned int OrynPhysicalMemoryReserveRange(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalStart,
    unsigned long long byteCount)
{
    unsigned int removed = 0U;

    if (allocator == 0 || allocator->Initialized == 0U || byteCount == 0ULL)
    {
        return 0U;
    }

    unsigned long long first = AlignDown(physicalStart);
    unsigned long long end = AlignUp(physicalStart + byteCount);
    for (unsigned int index = 0U; index < allocator->FreePageCount;)
    {
        unsigned long long page = allocator->FreePages[index];
        if (page >= first && page < end)
        {
            allocator->FreePageCount -= 1U;
            allocator->FreePages[index] = allocator->FreePages[allocator->FreePageCount];
            allocator->ReservedPages += 1U;
            (void)OrynPhysicalMemorySetPageOwner(allocator, page, OrynPhysicalPageOwnerReserved, physicalStart);
            removed += 1U;
            continue;
        }

        ++index;
    }

    return removed;
}

int OrynPhysicalMemoryFreePage(OrynKernelPhysicalMemory* allocator, unsigned long long physicalAddress)
{
    if (allocator == 0 || allocator->Initialized == 0U)
    {
        return 0;
    }

    if (physicalAddress < ORYN_PHYSICAL_MIN_ALLOC_ADDRESS || !IsAlignedPage(physicalAddress))
    {
        return 0;
    }

    if (allocator->FreePageCount >= allocator->CapacityPages)
    {
        return 0;
    }

    (void)OrynPhysicalMemorySetPageOwner(allocator, physicalAddress, OrynPhysicalPageOwnerFree, 0ULL);
    allocator->FreePages[allocator->FreePageCount] = physicalAddress;
    allocator->FreePageCount += 1U;

    if (allocator->UsedPageCount > 0U)
    {
        allocator->UsedPageCount -= 1U;
    }

    return 1;
}


static OrynPhysicalPageRecord* FindPageRecord(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress)
{
    unsigned long long page = AlignDown(physicalAddress);
    if (allocator == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < allocator->PageRecordCount; ++index)
    {
        if (allocator->PageRecords[index].PhysicalAddress == page)
        {
            return &allocator->PageRecords[index];
        }
    }
    return 0;
}

int OrynPhysicalMemorySetPageOwner(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress,
    unsigned int owner,
    unsigned long long tag)
{
    OrynPhysicalPageRecord* record;
    unsigned long long page = AlignDown(physicalAddress);
    if (allocator == 0 || allocator->Initialized == 0U || !IsAlignedPage(page))
    {
        return 0;
    }
    record = FindPageRecord(allocator, page);
    if (record == 0)
    {
        if (allocator->PageRecordCount >= ORYN_PHYSICAL_MAX_OWNERSHIP_RECORDS)
        {
            allocator->OwnershipRecordOverflows += 1ULL;
            return 0;
        }
        record = &allocator->PageRecords[allocator->PageRecordCount++];
        record->PhysicalAddress = page;
        record->ReferenceCount = 0U;
    }
    record->Owner = owner;
    record->Tag = tag;
    if (owner == OrynPhysicalPageOwnerFree)
    {
        record->ReferenceCount = 0U;
    }
    else if (record->ReferenceCount == 0U)
    {
        record->ReferenceCount = 1U;
    }
    return 1;
}

int OrynPhysicalMemoryAddPageReference(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress)
{
    OrynPhysicalPageRecord* record = FindPageRecord(allocator, physicalAddress);
    if (record == 0 || record->Owner == OrynPhysicalPageOwnerFree)
    {
        if (allocator != 0)
        {
            allocator->OwnershipMismatches += 1ULL;
        }
        return 0;
    }
    record->ReferenceCount += 1U;
    return 1;
}

int OrynPhysicalMemoryReleasePageReference(
    OrynKernelPhysicalMemory* allocator,
    unsigned long long physicalAddress)
{
    OrynPhysicalPageRecord* record = FindPageRecord(allocator, physicalAddress);
    if (record == 0 || record->ReferenceCount == 0U)
    {
        if (allocator != 0)
        {
            allocator->OwnershipMismatches += 1ULL;
        }
        return 0;
    }
    record->ReferenceCount -= 1U;
    if (record->ReferenceCount == 0U)
    {
        record->Owner = OrynPhysicalPageOwnerFree;
        record->Tag = 0ULL;
    }
    return 1;
}

int OrynPhysicalMemoryGetOwnershipStats(
    const OrynKernelPhysicalMemory* allocator,
    OrynPhysicalPageOwnershipStats* stats)
{
    if (allocator == 0 || stats == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < sizeof(*stats); ++index)
    {
        ((unsigned char*)stats)[index] = 0U;
    }
    stats->RecordsUsed = allocator->PageRecordCount;
    stats->RecordsCapacity = ORYN_PHYSICAL_MAX_OWNERSHIP_RECORDS;
    stats->OwnershipRecordOverflows = allocator->OwnershipRecordOverflows;
    stats->OwnershipMismatches = allocator->OwnershipMismatches;
    for (unsigned int index = 0U; index < allocator->PageRecordCount; ++index)
    {
        const OrynPhysicalPageRecord* record = &allocator->PageRecords[index];
        if (record->ReferenceCount != 0U)
        {
            stats->PagesWithReferences += 1ULL;
            stats->TotalReferences += record->ReferenceCount;
        }
        if (record->Owner == OrynPhysicalPageOwnerGeneric) stats->GenericPages += 1ULL;
        else if (record->Owner == OrynPhysicalPageOwnerPageTable) stats->PageTablePages += 1ULL;
        else if (record->Owner == OrynPhysicalPageOwnerKernelHeap) stats->KernelHeapPages += 1ULL;
        else if (record->Owner == OrynPhysicalPageOwnerUserPage) stats->UserPages += 1ULL;
        else if (record->Owner == OrynPhysicalPageOwnerReserved) stats->ReservedPages += 1ULL;
    }
    return 1;
}
