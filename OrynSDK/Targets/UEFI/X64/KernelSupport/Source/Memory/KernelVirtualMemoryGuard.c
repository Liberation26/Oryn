#include "KernelVirtualMemory.h"

#define ORYN_GUARD_PAGE_PRESENT 0x001ULL
#define ORYN_GUARD_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL

typedef unsigned long long OrynGuardPageTableEntry;

static OrynGuardPageTableEntry* GuardExistingTable(
    OrynGuardPageTableEntry* table,
    unsigned int index)
{
    OrynGuardPageTableEntry entry;
    if (table == 0)
    {
        return 0;
    }

    entry = table[index];
    if ((entry & ORYN_GUARD_PAGE_PRESENT) == 0ULL)
    {
        return 0;
    }

    return (OrynGuardPageTableEntry*)(entry & ORYN_GUARD_PAGE_ADDRESS_MASK);
}

int OrynVirtualMemoryUnmapGuardPage(unsigned long long virtualAddress)
{
    unsigned long long cr3 = OrynVirtualMemoryReadCr3();
    OrynGuardPageTableEntry* pml4 =
        (OrynGuardPageTableEntry*)(cr3 & ORYN_GUARD_PAGE_ADDRESS_MASK);
    unsigned int pml4Index = (unsigned int)((virtualAddress >> 39) & 0x1FFULL);
    unsigned int pdptIndex = (unsigned int)((virtualAddress >> 30) & 0x1FFULL);
    unsigned int pdIndex = (unsigned int)((virtualAddress >> 21) & 0x1FFULL);
    unsigned int ptIndex = (unsigned int)((virtualAddress >> 12) & 0x1FFULL);

    OrynGuardPageTableEntry* pdpt = GuardExistingTable(pml4, pml4Index);
    OrynGuardPageTableEntry* pd;
    OrynGuardPageTableEntry* pt;

    if (pdpt == 0)
    {
        return 0;
    }

    pd = GuardExistingTable(pdpt, pdptIndex);
    if (pd == 0)
    {
        return 0;
    }

    pt = GuardExistingTable(pd, pdIndex);
    if (pt == 0)
    {
        return 0;
    }

    pt[ptIndex] = 0ULL;
    __asm__ volatile ("invlpg (%0)" :: "r"(virtualAddress) : "memory");
    return 1;
}
