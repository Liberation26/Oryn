#include "KernelVirtualMemory.h"
#include "KernelPageFaultPolicy.h"

#define ORYN_PAGE_PRESENT 0x001ULL
#define ORYN_PAGE_WRITABLE 0x002ULL
#define ORYN_PAGE_USER 0x004ULL
#define ORYN_PAGE_GLOBAL 0x100ULL
#define ORYN_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_PAGE_NX 0x8000000000000000ULL

typedef unsigned long long OrynVmEntry;

static OrynVmEntry* VmWalk(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long virtualAddress,
    int create);

static unsigned int gNextAddressSpaceId = 1U;

static void VmClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static unsigned long long VmAlignDown(unsigned long long value)
{
    return value & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
}

static unsigned long long VmAlignUp(unsigned long long value)
{
    return (value + ORYN_VIRTUAL_PAGE_SIZE - 1ULL) & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
}

static unsigned long long VmFlagsToEntryFlags(unsigned long long flags)
{
    unsigned long long entryFlags = ORYN_PAGE_PRESENT;
    if ((flags & ORYN_VIRTUAL_FLAG_WRITE) != 0ULL)
    {
        entryFlags |= ORYN_PAGE_WRITABLE;
    }
    if ((flags & ORYN_VIRTUAL_FLAG_USER) != 0ULL)
    {
        entryFlags |= ORYN_PAGE_USER;
    }
    if ((flags & ORYN_VIRTUAL_FLAG_GLOBAL) != 0ULL)
    {
        entryFlags |= ORYN_PAGE_GLOBAL;
    }
    if ((flags & ORYN_VIRTUAL_FLAG_EXECUTE) == 0ULL)
    {
        entryFlags |= ORYN_PAGE_NX;
    }
    return entryFlags;
}

int OrynVirtualMemoryIsUserAddress(unsigned long long virtualAddress)
{
    return virtualAddress >= ORYN_VIRTUAL_USER_BASE && virtualAddress < ORYN_VIRTUAL_USER_LIMIT;
}

int OrynVirtualMemoryIsKernelAddress(unsigned long long virtualAddress)
{
    return virtualAddress >= ORYN_VIRTUAL_KERNEL_BASE;
}

static int VmRangeOverflow(unsigned long long start, unsigned long long bytes)
{
    return bytes != 0ULL && (start + bytes) < start;
}

static int VmRangeAllowed(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long flags)
{
    unsigned long long end;
    if (addressSpace == 0 || addressSpace->Initialized == 0U || bytes == 0ULL ||
        VmRangeOverflow(virtualAddress, bytes))
    {
        if (addressSpace != 0)
        {
            addressSpace->ApiInvalidRangeRejects += 1ULL;
        }
        return 0;
    }
    end = virtualAddress + bytes;
    if (end <= virtualAddress)
    {
        addressSpace->ApiInvalidRangeRejects += 1ULL;
        return 0;
    }
    if ((flags & ORYN_VIRTUAL_FLAG_USER) != 0ULL)
    {
        if (virtualAddress < addressSpace->UserBase || end > addressSpace->UserLimit)
        {
            addressSpace->ApiInvalidRangeRejects += 1ULL;
            return 0;
        }
    }
    else if (addressSpace->ProcessOwned != 0U && virtualAddress < addressSpace->KernelBase)
    {
        addressSpace->ApiInvalidRangeRejects += 1ULL;
        return 0;
    }
    return 1;
}

static int VmRangeEntriesPresent(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long start,
    unsigned long long end)
{
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, 0, address, 0);
        if (entry == 0 || (*entry & ORYN_PAGE_PRESENT) == 0ULL)
        {
            addressSpace->ApiMissingMappingRejects += 1ULL;
            return 0;
        }
    }
    return 1;
}

static int VmRangeEntriesFree(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long start,
    unsigned long long end)
{
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, 0, address, 0);
        if (entry != 0 && (*entry & ORYN_PAGE_PRESENT) != 0ULL)
        {
            addressSpace->ApiOverwriteRejects += 1ULL;
            return 0;
        }
    }
    return 1;
}

static OrynVmEntry* VmExistingTable(OrynVmEntry* table, unsigned int index)
{
    if (table == 0 || (table[index] & ORYN_PAGE_PRESENT) == 0ULL)
    {
        return 0;
    }
    return (OrynVmEntry*)(table[index] & ORYN_PAGE_ADDRESS_MASK);
}

static OrynVmEntry* VmNewTable(OrynKernelPhysicalMemory* physicalMemory)
{
    unsigned long long physicalAddress;
    if (physicalMemory == 0 || physicalMemory->Initialized == 0U)
    {
        return 0;
    }
    physicalAddress = OrynPhysicalMemoryAllocatePageBelow(
        physicalMemory,
        ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
    if (physicalAddress == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        return 0;
    }
    VmClearBytes((void*)physicalAddress, ORYN_VIRTUAL_PAGE_SIZE);
    (void)OrynPhysicalMemorySetPageOwner(
        physicalMemory,
        physicalAddress,
        OrynPhysicalPageOwnerPageTable,
        0ULL);
    return (OrynVmEntry*)physicalAddress;
}

static OrynVmEntry* VmTable(OrynVmEntry* table, unsigned int index, OrynKernelPhysicalMemory* physicalMemory)
{
    OrynVmEntry* next = VmExistingTable(table, index);
    if (next != 0)
    {
        return next;
    }
    next = VmNewTable(physicalMemory);
    if (next == 0)
    {
        return 0;
    }
    table[index] = ((unsigned long long)next & ORYN_PAGE_ADDRESS_MASK) |
        ORYN_PAGE_PRESENT | ORYN_PAGE_WRITABLE | ORYN_PAGE_USER;
    return next;
}

static OrynVmEntry* VmWalk(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long virtualAddress,
    int create)
{
    OrynVmEntry* pml4;
    OrynVmEntry* pdpt;
    OrynVmEntry* pd;
    unsigned int pml4Index;
    unsigned int pdptIndex;
    unsigned int pdIndex;
    unsigned int ptIndex;

    if (addressSpace == 0 || !addressSpace->Initialized || addressSpace->Pml4Physical == 0ULL)
    {
        return 0;
    }

    pml4 = (OrynVmEntry*)(addressSpace->Pml4Physical & ORYN_PAGE_ADDRESS_MASK);
    pml4Index = (unsigned int)((virtualAddress >> 39) & 0x1FFULL);
    pdptIndex = (unsigned int)((virtualAddress >> 30) & 0x1FFULL);
    pdIndex = (unsigned int)((virtualAddress >> 21) & 0x1FFULL);
    ptIndex = (unsigned int)((virtualAddress >> 12) & 0x1FFULL);

    pdpt = create ? VmTable(pml4, pml4Index, physicalMemory) : VmExistingTable(pml4, pml4Index);
    if (pdpt == 0)
    {
        return 0;
    }
    pd = create ? VmTable(pdpt, pdptIndex, physicalMemory) : VmExistingTable(pdpt, pdptIndex);
    if (pd == 0)
    {
        return 0;
    }
    {
        OrynVmEntry* pt = create ? VmTable(pd, pdIndex, physicalMemory) : VmExistingTable(pd, pdIndex);
        if (pt == 0)
        {
            return 0;
        }
        return pt + ptIndex;
    }
}

int OrynVirtualMemoryInitKernelAddressSpace(OrynKernelVirtualMemory* virtualMemory)
{
    OrynKernelAddressSpace* addressSpace;
    if (virtualMemory == 0 || virtualMemory->NewPml4 == 0ULL)
    {
        return 0;
    }
    addressSpace = &virtualMemory->KernelAddressSpace;
    VmClearBytes(addressSpace, sizeof(*addressSpace));
    addressSpace->Initialized = 1U;
    addressSpace->ProcessOwned = 0U;
    addressSpace->AddressSpaceId = 0U;
    addressSpace->Pml4Physical = virtualMemory->NewPml4 & ORYN_PAGE_ADDRESS_MASK;
    addressSpace->UserBase = ORYN_VIRTUAL_USER_BASE;
    addressSpace->UserLimit = ORYN_VIRTUAL_USER_LIMIT;
    addressSpace->KernelBase = ORYN_VIRTUAL_KERNEL_BASE;
    addressSpace->KernelLimit = ORYN_VIRTUAL_KERNEL_LIMIT;
    virtualMemory->UserBase = ORYN_VIRTUAL_USER_BASE;
    virtualMemory->UserLimit = ORYN_VIRTUAL_USER_LIMIT;
    virtualMemory->KernelBase = ORYN_VIRTUAL_KERNEL_BASE;
    virtualMemory->KernelLimit = ORYN_VIRTUAL_KERNEL_LIMIT;
    virtualMemory->AddressSpaceApiReady = 1U;
    return 1;
}

int OrynVirtualMemoryCreateProcessAddressSpace(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelAddressSpace* addressSpace)
{
    OrynVmEntry* pml4;
    OrynVmEntry* current;
    if (addressSpace == 0)
    {
        return 0;
    }
    VmClearBytes(addressSpace, sizeof(*addressSpace));
    pml4 = VmNewTable(physicalMemory);
    if (pml4 == 0)
    {
        return 0;
    }
    current = (OrynVmEntry*)(OrynVirtualMemoryReadCr3() & ORYN_PAGE_ADDRESS_MASK);
    if (current != 0)
    {
        for (unsigned int index = 256U; index < 512U; ++index)
        {
            pml4[index] = current[index];
        }
    }
    addressSpace->Initialized = 1U;
    addressSpace->ProcessOwned = 1U;
    addressSpace->AddressSpaceId = gNextAddressSpaceId++;
    addressSpace->Pml4Physical = (unsigned long long)pml4 & ORYN_PAGE_ADDRESS_MASK;
    addressSpace->UserBase = ORYN_VIRTUAL_USER_BASE;
    addressSpace->UserLimit = ORYN_VIRTUAL_USER_LIMIT;
    addressSpace->KernelBase = ORYN_VIRTUAL_KERNEL_BASE;
    addressSpace->KernelLimit = ORYN_VIRTUAL_KERNEL_LIMIT;
    return 1;
}

int OrynVirtualMemoryMap(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long virtualAddress,
    unsigned long long physicalAddress,
    unsigned long long bytes,
    unsigned long long flags)
{
    unsigned long long start = VmAlignDown(virtualAddress);
    unsigned long long end = VmAlignUp(virtualAddress + bytes);
    unsigned long long physical = VmAlignDown(physicalAddress);
    unsigned long long mappedNow = 0ULL;
    if (addressSpace != 0)
    {
        addressSpace->ApiValidationRuns += 1ULL;
    }
    if (physicalMemory == 0 || physicalMemory->Initialized == 0U ||
        physicalAddress != physical || (flags & ORYN_VIRTUAL_FLAG_GUARD) != 0ULL ||
        !VmRangeAllowed(addressSpace, virtualAddress, bytes, flags) ||
        end <= start || !VmRangeEntriesFree(addressSpace, start, end))
    {
        if (addressSpace != 0) addressSpace->ApiValidationFailures += 1ULL;
        return 0;
    }
    addressSpace->WriteExecutePolicyChecks += 1ULL;
    if (!OrynVirtualMemoryFlagsRespectWriteXorExecute(flags))
    {
        addressSpace->WriteExecuteDeniedCount += 1ULL;
        addressSpace->ApiValidationFailures += 1ULL;
        return 0;
    }
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, physicalMemory, address, 1);
        if (entry == 0)
        {
            (void)OrynVirtualMemoryUnmap(addressSpace, start, mappedNow * ORYN_VIRTUAL_PAGE_SIZE);
            addressSpace->ApiPartialRollbackPages += mappedNow;
            addressSpace->ApiValidationFailures += 1ULL;
            return 0;
        }
        *entry = (physical & ORYN_PAGE_ADDRESS_MASK) | VmFlagsToEntryFlags(flags);
        physical += ORYN_VIRTUAL_PAGE_SIZE;
        addressSpace->MappedPages += 1ULL;
        mappedNow += 1ULL;
    }
    return 1;
}

int OrynVirtualMemoryUnmap(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes)
{
    unsigned long long start = VmAlignDown(virtualAddress);
    unsigned long long end = VmAlignUp(virtualAddress + bytes);
    unsigned long long rangeFlags = OrynVirtualMemoryIsUserAddress(virtualAddress) ?
        ORYN_VIRTUAL_FLAG_USER : 0ULL;
    if (addressSpace != 0)
    {
        addressSpace->ApiValidationRuns += 1ULL;
    }
    if (!VmRangeAllowed(addressSpace, virtualAddress, bytes, rangeFlags) ||
        end <= start || !VmRangeEntriesPresent(addressSpace, start, end))
    {
        if (addressSpace != 0) addressSpace->ApiValidationFailures += 1ULL;
        return 0;
    }
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, 0, address, 0);
        *entry = 0ULL;
        __asm__ volatile ("invlpg (%0)" :: "r"(address) : "memory");
        addressSpace->UnmappedPages += 1ULL;
    }
    return 1;
}

int OrynVirtualMemoryProtect(
    OrynKernelAddressSpace* addressSpace,
    unsigned long long virtualAddress,
    unsigned long long bytes,
    unsigned long long flags)
{
    unsigned long long start = VmAlignDown(virtualAddress);
    unsigned long long end = VmAlignUp(virtualAddress + bytes);
    if (addressSpace != 0)
    {
        addressSpace->ApiValidationRuns += 1ULL;
    }
    if (!VmRangeAllowed(addressSpace, virtualAddress, bytes, flags) ||
        end <= start || !VmRangeEntriesPresent(addressSpace, start, end))
    {
        if (addressSpace != 0) addressSpace->ApiValidationFailures += 1ULL;
        return 0;
    }
    addressSpace->WriteExecutePolicyChecks += 1ULL;
    if (!OrynVirtualMemoryFlagsRespectWriteXorExecute(flags))
    {
        addressSpace->WriteExecuteDeniedCount += 1ULL;
        addressSpace->ApiValidationFailures += 1ULL;
        return 0;
    }
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, 0, address, 0);
        *entry = (*entry & ORYN_PAGE_ADDRESS_MASK) | VmFlagsToEntryFlags(flags);
        __asm__ volatile ("invlpg (%0)" :: "r"(address) : "memory");
        addressSpace->ProtectedPages += 1ULL;
    }
    return 1;
}
