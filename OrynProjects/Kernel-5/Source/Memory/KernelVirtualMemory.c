#include "KernelVirtualMemory.h"
#include "KernelBootInfo.h"

#define ORYN_PAGE_PRESENT 0x001ULL
#define ORYN_PAGE_WRITABLE 0x002ULL
#define ORYN_PAGE_FLAGS (ORYN_PAGE_PRESENT | ORYN_PAGE_WRITABLE)
#define ORYN_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_LOW_IDENTITY_BYTES (64ULL * 1024ULL * 1024ULL)
#define ORYN_VGA_TEXT_BASE 0x00000000000B8000ULL
#define ORYN_VGA_TEXT_BYTES 0x8000ULL

typedef unsigned long long OrynPageTableEntry;

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static unsigned long long AlignDown(unsigned long long value)
{
    return value & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
}

static unsigned long long AlignUp(unsigned long long value)
{
    return (value + ORYN_VIRTUAL_PAGE_SIZE - 1ULL) & ~(ORYN_VIRTUAL_PAGE_SIZE - 1ULL);
}

static void ClearVirtualMemory(OrynKernelVirtualMemory* virtualMemory)
{
    ClearBytes(virtualMemory, sizeof(*virtualMemory));
}

unsigned long long OrynVirtualMemoryReadCr3(void)
{
    unsigned long long value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static void OrynVirtualMemoryWriteCr3(unsigned long long value)
{
    __asm__ volatile ("mov %0, %%cr3" :: "r"(value) : "memory");
}

static OrynPageTableEntry* AllocateTable(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long physicalAddress = OrynPhysicalMemoryAllocatePage(physicalMemory);
    if (physicalAddress == ORYN_PHYSICAL_ALLOC_FAIL)
    {
        virtualMemory->MapFailure = 1U;
        return 0;
    }

    OrynPageTableEntry* table = (OrynPageTableEntry*)(unsigned long long)physicalAddress;
    ClearBytes(table, ORYN_VIRTUAL_PAGE_SIZE);
    virtualMemory->TablesAllocated += 1U;
    return table;
}

static OrynPageTableEntry* GetOrCreateNextTable(
    OrynPageTableEntry* table,
    unsigned int index,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    OrynPageTableEntry entry = table[index];
    if ((entry & ORYN_PAGE_PRESENT) != 0ULL)
    {
        return (OrynPageTableEntry*)(entry & ORYN_PAGE_ADDRESS_MASK);
    }

    OrynPageTableEntry* next = AllocateTable(physicalMemory, virtualMemory);
    if (next == 0)
    {
        return 0;
    }

    table[index] = ((unsigned long long)next & ORYN_PAGE_ADDRESS_MASK) | ORYN_PAGE_FLAGS;
    return next;
}

static int MapPage(
    OrynPageTableEntry* pml4,
    unsigned long long virtualAddress,
    unsigned long long physicalAddress,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned int pml4Index = (unsigned int)((virtualAddress >> 39) & 0x1FFULL);
    unsigned int pdptIndex = (unsigned int)((virtualAddress >> 30) & 0x1FFULL);
    unsigned int pdIndex = (unsigned int)((virtualAddress >> 21) & 0x1FFULL);
    unsigned int ptIndex = (unsigned int)((virtualAddress >> 12) & 0x1FFULL);

    OrynPageTableEntry* pdpt = GetOrCreateNextTable(pml4, pml4Index, physicalMemory, virtualMemory);
    if (pdpt == 0)
    {
        return 0;
    }

    OrynPageTableEntry* pd = GetOrCreateNextTable(pdpt, pdptIndex, physicalMemory, virtualMemory);
    if (pd == 0)
    {
        return 0;
    }

    OrynPageTableEntry* pt = GetOrCreateNextTable(pd, pdIndex, physicalMemory, virtualMemory);
    if (pt == 0)
    {
        return 0;
    }

    if ((pt[ptIndex] & ORYN_PAGE_PRESENT) == 0ULL)
    {
        virtualMemory->IdentityMappedPages += 1ULL;
    }

    pt[ptIndex] = (physicalAddress & ORYN_PAGE_ADDRESS_MASK) | ORYN_PAGE_FLAGS;
    return 1;
}

static int MapRange(
    OrynPageTableEntry* pml4,
    unsigned long long start,
    unsigned long long bytes,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long first = AlignDown(start);
    unsigned long long end = AlignUp(start + bytes);

    if (bytes == 0ULL || end <= first)
    {
        return 1;
    }

    for (unsigned long long address = first; address < end; address += ORYN_VIRTUAL_PAGE_SIZE)
    {
        if (!MapPage(pml4, address, address, physicalMemory, virtualMemory))
        {
            return 0;
        }
    }

    return 1;
}

static int ShouldIdentityMapEntry(const OrynKernelMemoryEntry* entry)
{
    if (entry->Type == OrynKernelMemoryBad)
    {
        return 0;
    }

    if (entry->Type == OrynKernelMemoryMmio)
    {
        return 0;
    }

    return 1;
}

static int BootInfoHasUsableFramebufferFields(const OrynBootInfo* bootInfo)
{
    if (bootInfo == 0)
    {
        return 0;
    }

    if (bootInfo->FramebufferBase == 0ULL || bootInfo->FramebufferSize == 0ULL ||
        bootInfo->FramebufferWidth == 0U || bootInfo->FramebufferHeight == 0U ||
        bootInfo->FramebufferPixelsPerScanLine < bootInfo->FramebufferWidth)
    {
        return 0;
    }

    return 1;
}

static int MapMemoryMapEntries(
    OrynPageTableEntry* pml4,
    const OrynKernelMemoryMap* memoryMap,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    for (unsigned int index = 0; index < memoryMap->EntryCount; ++index)
    {
        const OrynKernelMemoryEntry* entry = &memoryMap->Entries[index];
        unsigned long long bytes = entry->PageCount * ORYN_VIRTUAL_PAGE_SIZE;
        if (!ShouldIdentityMapEntry(entry))
        {
            continue;
        }

        if (!MapRange(pml4, entry->PhysicalStart, bytes, physicalMemory, virtualMemory))
        {
            return 0;
        }
    }

    return 1;
}

static int MapKernelRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
    {
        return 1;
    }

    virtualMemory->KernelMapStart = AlignDown(bootInfo->KernelPhysicalBase);
    virtualMemory->KernelMapEnd = AlignUp(bootInfo->KernelPhysicalBase + bootInfo->KernelSize);
    return MapRange(pml4, bootInfo->KernelPhysicalBase, bootInfo->KernelSize, physicalMemory, virtualMemory);
}

static int MapBootInfoRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long start = (unsigned long long)bootInfo;
    unsigned long long bytes = (unsigned long long)sizeof(*bootInfo);

    virtualMemory->BootInfoMapStart = AlignDown(start);
    virtualMemory->BootInfoMapEnd = AlignUp(start + bytes);
    return MapRange(pml4, start, bytes, physicalMemory, virtualMemory);
}

static int MapBootMemoryMapRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long bytes;

    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP))
    {
        return 1;
    }

    bytes = bootInfo->MemoryMapEntryCount * bootInfo->MemoryMapEntrySize;
    virtualMemory->MemoryMapMapStart = AlignDown(bootInfo->MemoryMap);
    virtualMemory->MemoryMapMapEnd = AlignUp(bootInfo->MemoryMap + bytes);
    return MapRange(pml4, bootInfo->MemoryMap, bytes, physicalMemory, virtualMemory);
}

static int MapVgaTextRange(
    OrynPageTableEntry* pml4,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    virtualMemory->VgaTextMapStart = AlignDown(ORYN_VGA_TEXT_BASE);
    virtualMemory->VgaTextMapEnd = AlignUp(ORYN_VGA_TEXT_BASE + ORYN_VGA_TEXT_BYTES);
    if (!MapRange(pml4, ORYN_VGA_TEXT_BASE, ORYN_VGA_TEXT_BYTES, physicalMemory, virtualMemory))
    {
        return 0;
    }

    virtualMemory->VgaTextMapped = 1U;
    return 1;
}

static int MapFontRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FONT))
    {
        return 1;
    }

    if (bootInfo->FontBase == 0ULL || bootInfo->FontSize == 0ULL)
    {
        return 1;
    }

    virtualMemory->FontMapStart = AlignDown(bootInfo->FontBase);
    virtualMemory->FontMapEnd = AlignUp(bootInfo->FontBase + bootInfo->FontSize);
    if (!MapRange(pml4, bootInfo->FontBase, bootInfo->FontSize, physicalMemory, virtualMemory))
    {
        return 0;
    }

    virtualMemory->FontMapped = 1U;
    return 1;
}

static int MapFramebufferRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    int usableFramebuffer = BootInfoHasUsableFramebufferFields(bootInfo);
    virtualMemory->FramebufferSelected = KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER) ? 1U : 0U;
    if (!usableFramebuffer)
    {
        return 1;
    }

    if (virtualMemory->FramebufferSelected)
    {
        virtualMemory->FramebufferMapStart = AlignDown(bootInfo->FramebufferBase);
        virtualMemory->FramebufferMapEnd = AlignUp(bootInfo->FramebufferBase + bootInfo->FramebufferSize);
    }
    else
    {
        virtualMemory->DefaultScreenMapStart = AlignDown(bootInfo->FramebufferBase);
        virtualMemory->DefaultScreenMapEnd = AlignUp(bootInfo->FramebufferBase + bootInfo->FramebufferSize);
    }

    if (!MapRange(pml4, bootInfo->FramebufferBase, bootInfo->FramebufferSize, physicalMemory, virtualMemory))
    {
        return 0;
    }

    if (virtualMemory->FramebufferSelected)
    {
        virtualMemory->FramebufferMapped = 1U;
    }
    else
    {
        virtualMemory->DefaultScreenMapped = 1U;
    }

    return 1;
}

int OrynVirtualMemoryInit(
    const OrynBootInfo* bootInfo,
    const OrynKernelMemoryMap* memoryMap,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    if (virtualMemory == 0)
    {
        return 0;
    }

    ClearVirtualMemory(virtualMemory);
    virtualMemory->CurrentCr3 = OrynVirtualMemoryReadCr3();

    if (bootInfo == 0 || memoryMap == 0 || physicalMemory == 0 || physicalMemory->Initialized == 0U)
    {
        virtualMemory->MapFailure = 1U;
        return 0;
    }

    OrynPageTableEntry* pml4 = AllocateTable(physicalMemory, virtualMemory);
    if (pml4 == 0)
    {
        return 0;
    }

    virtualMemory->NewPml4 = (unsigned long long)pml4;
    if (!MapRange(pml4, 0ULL, ORYN_LOW_IDENTITY_BYTES, physicalMemory, virtualMemory) ||
        !MapMemoryMapEntries(pml4, memoryMap, physicalMemory, virtualMemory) ||
        !MapVgaTextRange(pml4, physicalMemory, virtualMemory) ||
        !MapKernelRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapBootInfoRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapBootMemoryMapRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapFontRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapFramebufferRange(pml4, bootInfo, physicalMemory, virtualMemory))
    {
        return 0;
    }

    OrynVirtualMemoryWriteCr3(virtualMemory->NewPml4);
    virtualMemory->Active = 1U;
    virtualMemory->Initialized = 1U;
    return 1;
}
