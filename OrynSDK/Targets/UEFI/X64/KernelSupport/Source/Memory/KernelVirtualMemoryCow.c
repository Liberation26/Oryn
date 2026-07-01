#include "KernelVirtualMemory.h"
#include "string.h"

#define ORYN_COW_PAGE_PRESENT 0x001ULL
#define ORYN_COW_PAGE_WRITABLE 0x002ULL
#define ORYN_COW_PAGE_USER 0x004ULL
#define ORYN_COW_PAGE_SOFTWARE_COW 0x200ULL
#define ORYN_COW_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL

typedef unsigned long long OrynCowEntry;

static void CowClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0ULL; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static OrynCowEntry* CowNewTable(OrynKernelPhysicalMemory* physicalMemory)
{
    unsigned long long physical = OrynPhysicalMemoryAllocatePageBelow(
        physicalMemory,
        ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
    if (physical == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        return 0;
    }
    CowClearBytes((void*)physical, ORYN_VIRTUAL_PAGE_SIZE);
    (void)OrynPhysicalMemorySetPageOwner(
        physicalMemory,
        physical,
        OrynPhysicalPageOwnerPageTable,
        0ULL);
    return (OrynCowEntry*)physical;
}

static OrynCowEntry* CowExistingTable(OrynCowEntry* table, unsigned int index)
{
    if (table == 0 || (table[index] & ORYN_COW_PAGE_PRESENT) == 0ULL)
    {
        return 0;
    }
    return (OrynCowEntry*)(table[index] & ORYN_COW_PAGE_ADDRESS_MASK);
}

static OrynCowEntry* CowWalk(OrynKernelAddressSpace* space, unsigned long long address)
{
    OrynCowEntry* pml4;
    OrynCowEntry* pdpt;
    OrynCowEntry* pd;
    OrynCowEntry* pt;
    if (space == 0 || space->Initialized == 0U || space->Pml4Physical == 0ULL)
    {
        return 0;
    }
    pml4 = (OrynCowEntry*)(space->Pml4Physical & ORYN_COW_PAGE_ADDRESS_MASK);
    pdpt = CowExistingTable(pml4, (unsigned int)((address >> 39) & 0x1FFULL));
    pd = CowExistingTable(pdpt, (unsigned int)((address >> 30) & 0x1FFULL));
    pt = CowExistingTable(pd, (unsigned int)((address >> 21) & 0x1FFULL));
    if (pt == 0)
    {
        return 0;
    }
    return pt + (unsigned int)((address >> 12) & 0x1FFULL);
}

static int CowCopyLeaf(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelAddressSpace* parent,
    OrynKernelAddressSpace* child,
    OrynCowEntry* parentEntry,
    OrynCowEntry* childEntry)
{
    unsigned long long physical;
    unsigned long long sharedEntry;
    if ((*parentEntry & ORYN_COW_PAGE_PRESENT) == 0ULL ||
        (*parentEntry & ORYN_COW_PAGE_USER) == 0ULL)
    {
        return 1;
    }
    physical = *parentEntry & ORYN_COW_PAGE_ADDRESS_MASK;
    sharedEntry = (*parentEntry & ~ORYN_COW_PAGE_WRITABLE) | ORYN_COW_PAGE_SOFTWARE_COW;
    *parentEntry = sharedEntry;
    *childEntry = sharedEntry;
    (void)OrynPhysicalMemoryAddPageReference(physicalMemory, physical);
    parent->CopyOnWriteSharedPages += 1ULL;
    child->CopyOnWriteSharedPages += 1ULL;
    child->CopyOnWriteClonePages += 1ULL;
    child->MappedPages += 1ULL;
    return 1;
}

static int CowCloneTable(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelAddressSpace* parent,
    OrynKernelAddressSpace* child,
    OrynCowEntry* parentTable,
    OrynCowEntry* childTable,
    unsigned int level)
{
    unsigned int max = level == 4U ? 256U : ORYN_VIRTUAL_TABLE_ENTRY_COUNT;
    for (unsigned int index = 0U; index < max; ++index)
    {
        OrynCowEntry parentEntry = parentTable[index];
        OrynCowEntry* newChild;
        if ((parentEntry & ORYN_COW_PAGE_PRESENT) == 0ULL)
        {
            continue;
        }
        if (level == 1U)
        {
            if (!CowCopyLeaf(physicalMemory, parent, child, &parentTable[index], &childTable[index]))
            {
                return 0;
            }
            continue;
        }
        newChild = CowNewTable(physicalMemory);
        if (newChild == 0)
        {
            return 0;
        }
        childTable[index] = ((unsigned long long)newChild & ORYN_COW_PAGE_ADDRESS_MASK) |
            ORYN_COW_PAGE_PRESENT | ORYN_COW_PAGE_WRITABLE | ORYN_COW_PAGE_USER;
        if (!CowCloneTable(
            physicalMemory,
            parent,
            child,
            (OrynCowEntry*)(parentEntry & ORYN_COW_PAGE_ADDRESS_MASK),
            newChild,
            level - 1U))
        {
            return 0;
        }
    }
    return 1;
}

static void CowCopyAnonymousRegions(
    const OrynKernelAddressSpace* parent,
    OrynKernelAddressSpace* child)
{
    for (unsigned int index = 0U; index < ORYN_VIRTUAL_MAX_ANONYMOUS_REGIONS; ++index)
    {
        child->AnonymousRegions[index] = parent->AnonymousRegions[index];
        if (child->AnonymousRegions[index].Used != 0U)
        {
            child->AnonymousRegions[index].CopyOnWriteInherited = 1U;
        }
    }
    child->AnonymousRegionCount = parent->AnonymousRegionCount;
}

int OrynVirtualMemoryCreateCopyOnWriteClone(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelAddressSpace* parentAddressSpace,
    OrynKernelAddressSpace* childAddressSpace)
{
    OrynCowEntry* parentPml4;
    OrynCowEntry* childPml4;
    if (physicalMemory == 0 || parentAddressSpace == 0 || childAddressSpace == 0 ||
        parentAddressSpace->Initialized == 0U || parentAddressSpace->ProcessOwned == 0U)
    {
        return 0;
    }
    if (!OrynVirtualMemoryCreateProcessAddressSpace(physicalMemory, childAddressSpace))
    {
        return 0;
    }
    parentPml4 = (OrynCowEntry*)(parentAddressSpace->Pml4Physical & ORYN_COW_PAGE_ADDRESS_MASK);
    childPml4 = (OrynCowEntry*)(childAddressSpace->Pml4Physical & ORYN_COW_PAGE_ADDRESS_MASK);
    CowCopyAnonymousRegions(parentAddressSpace, childAddressSpace);
    return CowCloneTable(
        physicalMemory,
        parentAddressSpace,
        childAddressSpace,
        parentPml4,
        childPml4,
        4U);
}

int OrynVirtualMemoryResolveCopyOnWriteFault(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long faultAddress)
{
    OrynCowEntry* entry = CowWalk(addressSpace, faultAddress);
    unsigned long long oldPhysical;
    unsigned long long newPhysical;
    if (entry == 0 || (*entry & ORYN_COW_PAGE_PRESENT) == 0ULL ||
        (*entry & ORYN_COW_PAGE_USER) == 0ULL ||
        (*entry & ORYN_COW_PAGE_SOFTWARE_COW) == 0ULL || physicalMemory == 0)
    {
        return 0;
    }
    oldPhysical = *entry & ORYN_COW_PAGE_ADDRESS_MASK;
    newPhysical = OrynPhysicalMemoryAllocatePageBelow(
        physicalMemory,
        ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
    if (newPhysical == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        return 0;
    }
    (void)memcpy((void*)newPhysical, (const void*)oldPhysical, ORYN_VIRTUAL_PAGE_SIZE);
    (void)OrynPhysicalMemorySetPageOwner(
        physicalMemory,
        newPhysical,
        OrynPhysicalPageOwnerUserPage,
        addressSpace->AddressSpaceId);
    (void)OrynPhysicalMemoryReleasePageReference(physicalMemory, oldPhysical);
    *entry = (newPhysical & ORYN_COW_PAGE_ADDRESS_MASK) |
        (((*entry & ~ORYN_COW_PAGE_ADDRESS_MASK) | ORYN_COW_PAGE_WRITABLE) &
        ~ORYN_COW_PAGE_SOFTWARE_COW);
    __asm__ volatile ("invlpg (%0)" :: "r"(faultAddress) : "memory");
    addressSpace->CopyOnWriteResolvedPages += 1ULL;
    return 1;
}
