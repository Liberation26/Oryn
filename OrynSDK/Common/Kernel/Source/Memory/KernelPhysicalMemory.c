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

unsigned long long OrynPhysicalMemoryAllocatePage(OrynKernelPhysicalMemory* allocator)
{
    if (allocator == 0 || allocator->Initialized == 0U || allocator->FreePageCount == 0U)
    {
        return ORYN_PHYSICAL_ALLOC_FAIL;
    }

    allocator->FreePageCount -= 1U;
    allocator->UsedPageCount += 1U;
    return allocator->FreePages[allocator->FreePageCount];
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

    allocator->FreePages[allocator->FreePageCount] = physicalAddress;
    allocator->FreePageCount += 1U;

    if (allocator->UsedPageCount > 0U)
    {
        allocator->UsedPageCount -= 1U;
    }

    return 1;
}
