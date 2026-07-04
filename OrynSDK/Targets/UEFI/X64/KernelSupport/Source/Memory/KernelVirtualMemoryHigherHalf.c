#include "KernelVirtualMemory.h"

#define ORYN_HIGHER_HALF_PAGE_PRESENT 0x001ULL
#define ORYN_HIGHER_HALF_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_HIGHER_HALF_MAX_PROOF_PAGES 131072ULL

typedef unsigned long long OrynHigherHalfEntry;

static int IsCanonical(unsigned long long value)
{
    unsigned long long bit47 = value & 0x0000800000000000ULL;
    unsigned long long high = value & 0xFFFF000000000000ULL;
    return bit47 != 0ULL ? high == 0xFFFF000000000000ULL : high == 0ULL;
}

static OrynHigherHalfEntry* ExistingTable(OrynHigherHalfEntry* table, unsigned int index)
{
    OrynHigherHalfEntry entry;
    if (table == 0)
    {
        return 0;
    }

    entry = table[index];
    if ((entry & ORYN_HIGHER_HALF_PAGE_PRESENT) == 0ULL)
    {
        return 0;
    }

    return (OrynHigherHalfEntry*)(entry & ORYN_HIGHER_HALF_PAGE_ADDRESS_MASK);
}

static OrynHigherHalfEntry* WalkPageEntry(
    OrynHigherHalfEntry* pml4,
    unsigned long long virtualAddress)
{
    unsigned int pml4Index = (unsigned int)((virtualAddress >> 39) & 0x1FFULL);
    unsigned int pdptIndex = (unsigned int)((virtualAddress >> 30) & 0x1FFULL);
    unsigned int pdIndex = (unsigned int)((virtualAddress >> 21) & 0x1FFULL);
    unsigned int ptIndex = (unsigned int)((virtualAddress >> 12) & 0x1FFULL);
    OrynHigherHalfEntry* pdpt = ExistingTable(pml4, pml4Index);
    OrynHigherHalfEntry* pd;
    OrynHigherHalfEntry* pt;

    if (pdpt == 0)
    {
        return 0;
    }

    pd = ExistingTable(pdpt, pdptIndex);
    if (pd == 0)
    {
        return 0;
    }

    pt = ExistingTable(pd, pdIndex);
    if (pt == 0)
    {
        return 0;
    }

    return &pt[ptIndex];
}

static int PageMapsToExpectedPhysical(
    OrynHigherHalfEntry* pml4,
    unsigned long long virtualAddress,
    unsigned long long physicalAddress)
{
    OrynHigherHalfEntry* entry = WalkPageEntry(pml4, virtualAddress);
    if (entry == 0 || ((*entry & ORYN_HIGHER_HALF_PAGE_PRESENT) == 0ULL))
    {
        return 0;
    }

    return ((*entry & ORYN_HIGHER_HALF_PAGE_ADDRESS_MASK) ==
        (physicalAddress & ORYN_HIGHER_HALF_PAGE_ADDRESS_MASK));
}

static int ValidateEveryHigherHalfPage(OrynKernelVirtualMemory* virtualMemory)
{
    OrynHigherHalfEntry* pml4 =
        (OrynHigherHalfEntry*)(virtualMemory->NewPml4 & ORYN_HIGHER_HALF_PAGE_ADDRESS_MASK);
    unsigned long long virtualStart = virtualMemory->KernelVirtualMapStart;
    unsigned long long physicalStart = virtualMemory->KernelMapStart;
    unsigned long long virtualEnd = virtualMemory->KernelVirtualMapEnd;
    unsigned long long pages = (virtualEnd - virtualStart) / ORYN_VIRTUAL_PAGE_SIZE;

    if (pages == 0ULL || pages > ORYN_HIGHER_HALF_MAX_PROOF_PAGES)
    {
        return 0;
    }

    for (unsigned long long page = 0ULL; page < pages; ++page)
    {
        unsigned long long virtualAddress = virtualStart + (page * ORYN_VIRTUAL_PAGE_SIZE);
        unsigned long long physicalAddress = physicalStart + (page * ORYN_VIRTUAL_PAGE_SIZE);
        if (!PageMapsToExpectedPhysical(pml4, virtualAddress, physicalAddress))
        {
            return 0;
        }
        virtualMemory->HigherHalfMappedPageProofCount += 1ULL;
    }

    return 1;
}

int OrynVirtualMemoryValidateHigherHalfKernelMap(OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long start;
    unsigned long long end;
    unsigned long long physicalBytes;
    unsigned int pml4Index;

    if (virtualMemory == 0 || virtualMemory->NewPml4 == 0ULL)
    {
        return 0;
    }

    start = virtualMemory->KernelVirtualMapStart;
    end = virtualMemory->KernelVirtualMapEnd;
    physicalBytes = virtualMemory->KernelMapEnd - virtualMemory->KernelMapStart;
    pml4Index = (unsigned int)((start >> 39) & 0x1FFULL);

    virtualMemory->HigherHalfCanonical = IsCanonical(start) && IsCanonical(end - 1ULL) ? 1U : 0U;
    virtualMemory->HigherHalfAligned =
        (start & (ORYN_VIRTUAL_PAGE_SIZE - 1ULL)) == 0ULL &&
        (end & (ORYN_VIRTUAL_PAGE_SIZE - 1ULL)) == 0ULL ? 1U : 0U;
    virtualMemory->HigherHalfNoUserOverlap =
        start >= ORYN_VIRTUAL_KERNEL_BASE && end > start ? 1U : 0U;
    virtualMemory->HigherHalfEntryMapped =
        virtualMemory->KernelEntryVirtual >= start &&
        virtualMemory->KernelEntryVirtual < end &&
        virtualMemory->KernelEntryPhysical >= virtualMemory->KernelMapStart &&
        virtualMemory->KernelEntryPhysical < virtualMemory->KernelMapEnd ? 1U : 0U;
    virtualMemory->HigherHalfPml4SlotReady = pml4Index >= 256U ? 1U : 0U;
    virtualMemory->HigherHalfPhysicalWindowValid =
        physicalBytes != 0ULL && physicalBytes == (end - start) ? 1U : 0U;

    if (!virtualMemory->HigherHalfCanonical || !virtualMemory->HigherHalfAligned ||
        !virtualMemory->HigherHalfNoUserOverlap || !virtualMemory->HigherHalfEntryMapped ||
        !virtualMemory->HigherHalfPml4SlotReady || !virtualMemory->HigherHalfPhysicalWindowValid ||
        !ValidateEveryHigherHalfPage(virtualMemory))
    {
        virtualMemory->HigherHalfValidationFailures += 1ULL;
        return 0;
    }

    virtualMemory->HigherHalfReady = 1U;
    return 1;
}
