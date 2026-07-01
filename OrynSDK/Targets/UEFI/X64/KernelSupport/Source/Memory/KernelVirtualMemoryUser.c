#include "KernelVirtualMemory.h"

#define ORYN_VM_PAGE_PRESENT 0x001ULL
#define ORYN_VM_PAGE_WRITABLE 0x002ULL
#define ORYN_VM_PAGE_USER 0x004ULL
#define ORYN_VM_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL

typedef unsigned long long OrynVmUserEntry;

static unsigned long long UserAlignDown(unsigned long long value)
{
    return value & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
}

static unsigned long long UserAlignUp(unsigned long long value)
{
    return (value + ORYN_VIRTUAL_PAGE_SIZE - 1ULL) & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
}

static OrynVmUserEntry* UserExistingTable(OrynVmUserEntry* table, unsigned int index)
{
    if (table == 0 || (table[index] & ORYN_VM_PAGE_PRESENT) == 0ULL)
    {
        return 0;
    }
    return (OrynVmUserEntry*)(table[index] & ORYN_VM_PAGE_ADDRESS_MASK);
}

static OrynVmUserEntry* UserWalk(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress)
{
    OrynVmUserEntry* pml4;
    OrynVmUserEntry* pdpt;
    OrynVmUserEntry* pd;
    OrynVmUserEntry* pt;
    unsigned int pml4Index;
    unsigned int pdptIndex;
    unsigned int pdIndex;
    unsigned int ptIndex;

    if (addressSpace == 0 || addressSpace->Initialized == 0U ||
        addressSpace->Pml4Physical == 0ULL)
    {
        return 0;
    }

    pml4 = (OrynVmUserEntry*)(addressSpace->Pml4Physical & ORYN_VM_PAGE_ADDRESS_MASK);
    pml4Index = (unsigned int)((virtualAddress >> 39) & 0x1FFULL);
    pdptIndex = (unsigned int)((virtualAddress >> 30) & 0x1FFULL);
    pdIndex = (unsigned int)((virtualAddress >> 21) & 0x1FFULL);
    ptIndex = (unsigned int)((virtualAddress >> 12) & 0x1FFULL);

    pdpt = UserExistingTable(pml4, pml4Index);
    if (pdpt == 0) return 0;
    pd = UserExistingTable(pdpt, pdptIndex);
    if (pd == 0) return 0;
    pt = UserExistingTable(pd, pdIndex);
    if (pt == 0) return 0;
    return pt + ptIndex;
}

static int RangeOverflow(unsigned long long start, unsigned long long bytes)
{
    return bytes != 0ULL && (start + bytes) < start;
}

int OrynVirtualMemoryIsRangeInUserSpace(
    unsigned long long virtualAddress,
    unsigned long long bytes)
{
    unsigned long long end;
    if (bytes == 0ULL || RangeOverflow(virtualAddress, bytes))
    {
        return 0;
    }
    end = virtualAddress + bytes;
    return virtualAddress >= ORYN_VIRTUAL_USER_BASE && end <= ORYN_VIRTUAL_USER_LIMIT;
}

static OrynVirtualAnonymousRegion* FindAnonymousRegion(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long address)
{
    if (addressSpace == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < ORYN_VIRTUAL_MAX_ANONYMOUS_REGIONS; ++index)
    {
        OrynVirtualAnonymousRegion* region = &addressSpace->AnonymousRegions[index];
        if (region->Used != 0U && address >= region->Base && address < region->Base + region->Bytes)
        {
            return region;
        }
    }
    return 0;
}

int OrynVirtualMemoryReserveAnonymousRegion(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long flags)
{
    unsigned long long base = UserAlignDown(virtualAddress);
    unsigned long long end = UserAlignUp(virtualAddress + bytes);
    if (addressSpace == 0 || addressSpace->Initialized == 0U ||
        addressSpace->ProcessOwned == 0U || bytes == 0ULL || end <= base ||
        !OrynVirtualMemoryIsRangeInUserSpace(base, end - base))
    {
        return 0;
    }

    for (unsigned int index = 0U; index < ORYN_VIRTUAL_MAX_ANONYMOUS_REGIONS; ++index)
    {
        OrynVirtualAnonymousRegion* region = &addressSpace->AnonymousRegions[index];
        if (region->Used == 0U)
        {
            region->Used = 1U;
            region->Base = base;
            region->Bytes = end - base;
            region->Flags = (flags | ORYN_VIRTUAL_FLAG_USER) & ~ORYN_VIRTUAL_FLAG_GUARD;
            region->CommittedPages = 0ULL;
            addressSpace->AnonymousRegionCount += 1ULL;
            return 1;
        }
    }
    return 0;
}

int OrynVirtualMemoryDemandAllocateUserPage(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long faultAddress,
    unsigned long long flags)
{
    OrynVirtualAnonymousRegion* region;
    unsigned long long page;
    unsigned long long physical;
    if (addressSpace == 0 || physicalMemory == 0 ||
        !OrynVirtualMemoryIsRangeInUserSpace(faultAddress, 1ULL))
    {
        return 0;
    }
    page = UserAlignDown(faultAddress);
    region = FindAnonymousRegion(addressSpace, page);
    if (region == 0)
    {
        return 0;
    }
    {
        OrynVmUserEntry* existing = UserWalk(addressSpace, page);
        if (existing != 0 && (*existing & ORYN_VM_PAGE_PRESENT) != 0ULL)
        {
            return 1;
        }
    }
    physical = OrynPhysicalMemoryAllocatePageBelow(
        physicalMemory,
        ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
    if (physical == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        return 0;
    }
    if (!OrynVirtualMemoryMap(addressSpace, physicalMemory, page, physical,
        ORYN_VIRTUAL_PAGE_SIZE, (region->Flags | flags | ORYN_VIRTUAL_FLAG_USER)))
    {
        (void)OrynPhysicalMemoryFreePage(physicalMemory, physical);
        return 0;
    }
    (void)OrynPhysicalMemorySetPageOwner(
        physicalMemory,
        physical,
        OrynPhysicalPageOwnerUserPage,
        addressSpace->AddressSpaceId);
    region->CommittedPages += 1ULL;
    addressSpace->DemandAllocatedPages += 1ULL;
    return 1;
}

int OrynVirtualMemoryIsUserRangeMapped(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long requiredFlags)
{
    unsigned long long start;
    unsigned long long end;
    if (addressSpace == 0 || !OrynVirtualMemoryIsRangeInUserSpace(virtualAddress, bytes))
    {
        return 0;
    }
    start = UserAlignDown(virtualAddress);
    end = UserAlignUp(virtualAddress + bytes);
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmUserEntry* entry = UserWalk(addressSpace, address);
        if (entry == 0 || (*entry & ORYN_VM_PAGE_PRESENT) == 0ULL ||
            (*entry & ORYN_VM_PAGE_USER) == 0ULL)
        {
            return 0;
        }
        if ((requiredFlags & ORYN_VIRTUAL_FLAG_WRITE) != 0ULL &&
            (*entry & ORYN_VM_PAGE_WRITABLE) == 0ULL)
        {
            return 0;
        }
    }
    return 1;
}

static void* TranslateUserPointer(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long userAddress)
{
    OrynVmUserEntry* entry = UserWalk(addressSpace, userAddress);
    unsigned long long offset = userAddress & (ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
    if (entry == 0 || (*entry & ORYN_VM_PAGE_PRESENT) == 0ULL ||
        (*entry & ORYN_VM_PAGE_USER) == 0ULL)
    {
        return 0;
    }
    return (void*)((*entry & ORYN_VM_PAGE_ADDRESS_MASK) + offset);
}

static int CopyFromTranslatedUser(
    OrynKernelAddressSpace* addressSpace,
    void* kernelDestination,
    unsigned long long userAddress,
    unsigned long long bytes)
{
    unsigned char* d = (unsigned char*)kernelDestination;
    for (unsigned long long index = 0ULL; index < bytes; ++index)
    {
        const unsigned char* s = (const unsigned char*)TranslateUserPointer(addressSpace, userAddress + index);
        if (s == 0)
        {
            return 0;
        }
        d[index] = *s;
    }
    return 1;
}

static int CopyToTranslatedUser(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long userAddress,
    const void* kernelSource,
    unsigned long long bytes)
{
    const unsigned char* s = (const unsigned char*)kernelSource;
    for (unsigned long long index = 0ULL; index < bytes; ++index)
    {
        unsigned char* d = (unsigned char*)TranslateUserPointer(addressSpace, userAddress + index);
        if (d == 0)
        {
            return 0;
        }
        d[index] = s[index];
    }
    return 1;
}

int OrynCopyFromUser(
    OrynKernelAddressSpace* addressSpace,
    void* kernelDestination,
    const void* userSource,
    unsigned long long bytes)
{
    if (bytes == 0ULL)
    {
        return ORYN_USER_COPY_OK;
    }
    if (!OrynVirtualMemoryIsUserRangeMapped(
        addressSpace,
        (unsigned long long)userSource,
        bytes,
        ORYN_VIRTUAL_FLAG_READ))
    {
        return ORYN_USER_COPY_FAIL;
    }
    if (kernelDestination == 0 || userSource == 0)
    {
        return ORYN_USER_COPY_FAIL;
    }
    return CopyFromTranslatedUser(
        addressSpace,
        kernelDestination,
        (unsigned long long)userSource,
        bytes) ? ORYN_USER_COPY_OK : ORYN_USER_COPY_FAIL;
}

int OrynCopyToUser(
    OrynKernelAddressSpace* addressSpace,
    void* userDestination,
    const void* kernelSource,
    unsigned long long bytes)
{
    if (bytes == 0ULL)
    {
        return ORYN_USER_COPY_OK;
    }
    if (!OrynVirtualMemoryIsUserRangeMapped(
        addressSpace,
        (unsigned long long)userDestination,
        bytes,
        ORYN_VIRTUAL_FLAG_WRITE))
    {
        return ORYN_USER_COPY_FAIL;
    }
    if (userDestination == 0 || kernelSource == 0)
    {
        return ORYN_USER_COPY_FAIL;
    }
    return CopyToTranslatedUser(
        addressSpace,
        (unsigned long long)userDestination,
        kernelSource,
        bytes) ? ORYN_USER_COPY_OK : ORYN_USER_COPY_FAIL;
}
