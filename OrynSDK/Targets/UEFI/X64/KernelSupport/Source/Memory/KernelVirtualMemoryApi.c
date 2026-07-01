#include "KernelVirtualMemory.h"
#include "KernelPageFaultPolicy.h"

#define ORYN_PAGE_PRESENT 0x001ULL
#define ORYN_PAGE_WRITABLE 0x002ULL
#define ORYN_PAGE_USER 0x004ULL
#define ORYN_PAGE_GLOBAL 0x100ULL
#define ORYN_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_PAGE_NX 0x8000000000000000ULL

typedef unsigned long long OrynVmEntry;

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
    if (bytes == 0ULL || end <= start || (flags & ORYN_VIRTUAL_FLAG_GUARD) != 0ULL)
    {
        return 0;
    }
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, physicalMemory, address, 1);
        if (entry == 0)
        {
            return 0;
        }
        *entry = (physical & ORYN_PAGE_ADDRESS_MASK) | VmFlagsToEntryFlags(flags);
        physical += ORYN_VIRTUAL_PAGE_SIZE;
        addressSpace->MappedPages += 1ULL;
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
    if (bytes == 0ULL || end <= start)
    {
        return 0;
    }
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, 0, address, 0);
        if (entry != 0)
        {
            *entry = 0ULL;
            __asm__ volatile ("invlpg (%0)" :: "r"(address) : "memory");
            addressSpace->UnmappedPages += 1ULL;
        }
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
    if (bytes == 0ULL || end <= start)
    {
        return 0;
    }
    for (unsigned long long address = start; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        OrynVmEntry* entry = VmWalk(addressSpace, 0, address, 0);
        if (entry == 0 || (*entry & ORYN_PAGE_PRESENT) == 0ULL)
        {
            return 0;
        }
        *entry = (*entry & ORYN_PAGE_ADDRESS_MASK) | VmFlagsToEntryFlags(flags);
        __asm__ volatile ("invlpg (%0)" :: "r"(address) : "memory");
        addressSpace->ProtectedPages += 1ULL;
    }
    return 1;
}

int OrynVirtualMemoryRunAddressSpaceSelfTest(
    OrynKernelVirtualMemory* virtualMemory,
    OrynKernelPhysicalMemory* physicalMemory)
{
    OrynKernelAddressSpace processSpace;
    OrynKernelAddressSpace childSpace;
    unsigned long long physical;
    unsigned long long userAddress = ORYN_VIRTUAL_USER_BASE + 0x200000ULL;
    unsigned long long demandAddress = ORYN_VIRTUAL_USER_BASE + 0x300000ULL;
    unsigned char userSeed[8];
    unsigned char kernelCopy[8];
    OrynIdtInterruptFrame demandFrame;
    OrynIdtInterruptFrame cowFrame;
    int ok;
    if (virtualMemory == 0 || physicalMemory == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < sizeof(userSeed); ++index)
    {
        userSeed[index] = (unsigned char)(0x41U + index);
        kernelCopy[index] = 0U;
    }
    VmClearBytes(&demandFrame, sizeof(demandFrame));
    VmClearBytes(&cowFrame, sizeof(cowFrame));
    VmClearBytes(&childSpace, sizeof(childSpace));
    demandFrame.ErrorCode = ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;
    cowFrame.ErrorCode = ORYN_PAGE_FAULT_PRESENT | ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;

    ok = OrynVirtualMemoryInitKernelAddressSpace(virtualMemory);
    ok = ok && OrynVirtualMemoryCreateProcessAddressSpace(physicalMemory, &processSpace);
    physical = OrynPhysicalMemoryAllocatePageBelow(physicalMemory, ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
    ok = ok && physical != ORYN_PHYSICAL_ALLOC_FAIL;
    if (ok)
    {
        ok = OrynVirtualMemoryMap(&processSpace, physicalMemory, userAddress, physical,
            ORYN_VIRTUAL_PAGE_SIZE, ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        (void)OrynPhysicalMemorySetPageOwner(
            physicalMemory,
            physical,
            OrynPhysicalPageOwnerUserPage,
            processSpace.AddressSpaceId);
        unsigned char* physicalBytes = (unsigned char*)physical;
        for (unsigned int index = 0U; index < sizeof(userSeed); ++index)
        {
            physicalBytes[index] = userSeed[index];
        }
        ok = OrynCopyFromUser(&processSpace, kernelCopy, (const void*)userAddress, sizeof(kernelCopy));
    }
    if (ok)
    {
        for (unsigned int index = 0U; index < sizeof(userSeed); ++index)
        {
            if (kernelCopy[index] != userSeed[index])
            {
                ok = 0;
            }
        }
    }
    if (ok)
    {
        userSeed[0] = 0x5AU;
        ok = OrynCopyToUser(&processSpace, (void*)userAddress, userSeed, sizeof(userSeed));
    }
    if (ok)
    {
        ok = OrynVirtualMemoryCreateCopyOnWriteClone(physicalMemory, &processSpace, &childSpace);
    }
    if (ok)
    {
        OrynKernelPageFaultPolicySetProcessContext(&childSpace);
        OrynKernelPageFaultPolicySetDemandAllocator(&childSpace, physicalMemory);
        ok = OrynKernelPageFaultPolicyHandle(&cowFrame, userAddress) ==
            OrynKernelPageFaultActionRecover;
        OrynKernelPageFaultPolicySetDemandAllocator(0, 0);
        OrynKernelPageFaultPolicySetProcessContext(0);
    }
    if (ok)
    {
        ok = childSpace.CopyOnWriteResolvedPages != 0ULL;
    }
    if (ok)
    {
        ok = OrynVirtualMemoryProtect(&processSpace, userAddress, ORYN_VIRTUAL_PAGE_SIZE,
            ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryReserveAnonymousRegion(&processSpace, demandAddress,
            ORYN_VIRTUAL_PAGE_SIZE * 2ULL,
            ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_USER);
    }
    if (ok)
    {
        OrynKernelPageFaultPolicySetProcessContext(&processSpace);
        OrynKernelPageFaultPolicySetDemandAllocator(&processSpace, physicalMemory);
        ok = OrynKernelPageFaultPolicyHandle(&demandFrame, demandAddress) ==
            OrynKernelPageFaultActionRecover;
        OrynKernelPageFaultPolicySetDemandAllocator(0, 0);
        OrynKernelPageFaultPolicySetProcessContext(0);
    }
    if (ok)
    {
        ok = OrynVirtualMemoryUnmap(&processSpace, userAddress, ORYN_VIRTUAL_PAGE_SIZE);
    }
    if (ok)
    {
        virtualMemory->ProcessAddressSpacesCreated += 1U;
        virtualMemory->ApiMappedPages += processSpace.MappedPages;
        virtualMemory->ApiProtectedPages += processSpace.ProtectedPages;
        virtualMemory->ApiUnmappedPages += processSpace.UnmappedPages;
        virtualMemory->DemandAllocatedUserPages += processSpace.DemandAllocatedPages;
        virtualMemory->AnonymousRegionsCreated += processSpace.AnonymousRegionCount;
        virtualMemory->UserCopyBytesIn += sizeof(kernelCopy);
        virtualMemory->UserCopyBytesOut += sizeof(userSeed);
        virtualMemory->CopyOnWriteCloneCount += 1ULL;
        virtualMemory->CopyOnWriteSharedPages += processSpace.CopyOnWriteSharedPages + childSpace.CopyOnWriteSharedPages;
        virtualMemory->CopyOnWriteResolvedPages += childSpace.CopyOnWriteResolvedPages;
    }
    return ok;
}
