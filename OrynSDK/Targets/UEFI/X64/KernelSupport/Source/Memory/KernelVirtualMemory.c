#include "KernelVirtualMemory.h"
#include "KernelBootInfo.h"
#include "KernelIo.h"

#define ORYN_PAGE_PRESENT 0x001ULL
#define ORYN_PAGE_WRITABLE 0x002ULL
#define ORYN_PAGE_FLAGS (ORYN_PAGE_PRESENT | ORYN_PAGE_WRITABLE)
#define ORYN_PAGE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL
#define ORYN_LOW_IDENTITY_BYTES (1024ULL * 1024ULL * 1024ULL)
#define ORYN_EARLY_STACK_BYTES (1024ULL * 1024ULL)
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

unsigned long long OrynVirtualMemoryReadCr3(void)
{
    unsigned long long value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static unsigned long long OrynVirtualMemoryReadRsp(void)
{
    unsigned long long value;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(value));
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
    unsigned long long physicalAddress = OrynPhysicalMemoryAllocatePageBelow(
        physicalMemory,
        ORYN_PHYSICAL_EARLY_DIRECT_MAP_LIMIT);
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

    (void)virtualMemory;
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
        virtualMemory->IdentityMappedPages += 1ULL;
    }

    return 1;
}

static int MapVirtualRangeToPhysical(
    OrynPageTableEntry* pml4,
    unsigned long long virtualStart,
    unsigned long long physicalStart,
    unsigned long long bytes,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long firstVirtual = AlignDown(virtualStart);
    unsigned long long firstPhysical = AlignDown(physicalStart);
    unsigned long long offset = virtualStart - firstVirtual;
    unsigned long long endVirtual = AlignUp(virtualStart + bytes);

    if (bytes == 0ULL || endVirtual <= firstVirtual)
    {
        return 1;
    }

    firstPhysical += offset;
    firstPhysical = AlignDown(firstPhysical);

    for (unsigned long long virtualAddress = firstVirtual;
         virtualAddress < endVirtual;
         virtualAddress += ORYN_VIRTUAL_PAGE_SIZE)
    {
        unsigned long long physicalAddress = firstPhysical + (virtualAddress - firstVirtual);
        if (!MapPage(pml4, virtualAddress, physicalAddress, physicalMemory, virtualMemory))
        {
            return 0;
        }
        virtualMemory->KernelVirtualMappedPages += 1ULL;
    }

    return 1;
}

static int IsInsideKernelVirtualRange(
    const OrynBootInfo* bootInfo,
    unsigned long long start,
    unsigned long long bytes)
{
    if (bootInfo == 0 ||
        !KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_VIRTUAL_LAYOUT) ||
        bootInfo->KernelVirtualSize == 0ULL || bytes == 0ULL)
    {
        return 0;
    }

    unsigned long long layoutEnd = bootInfo->KernelVirtualBase + bootInfo->KernelVirtualSize;
    unsigned long long end = start + bytes;
    if (layoutEnd < bootInfo->KernelVirtualBase || end < start)
    {
        return 0;
    }

    return start >= bootInfo->KernelVirtualBase && end <= layoutEnd;
}

static int MapKernelOrIdentityRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    unsigned long long start,
    unsigned long long bytes,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    if (IsInsideKernelVirtualRange(bootInfo, start, bytes))
    {
        unsigned long long physicalStart =
            bootInfo->KernelPhysicalBase + (start - bootInfo->KernelVirtualBase);
        return MapVirtualRangeToPhysical(
            pml4,
            start,
            physicalStart,
            bytes,
            physicalMemory,
            virtualMemory);
    }

    return MapRange(pml4, start, bytes, physicalMemory, virtualMemory);
}

static int BootInfoHasUsableFramebufferFields(const OrynBootInfo* bootInfo)
{
    if (bootInfo == 0)
    {
        return 0;
    }

    if (bootInfo->Framebuffer.Base == 0ULL || bootInfo->Framebuffer.Size == 0ULL ||
        bootInfo->Framebuffer.Width == 0U || bootInfo->Framebuffer.Height == 0U ||
        bootInfo->Framebuffer.PixelsPerScanLine < bootInfo->Framebuffer.Width ||
        bootInfo->Framebuffer.BytesPerPixel == 0U)
    {
        return 0;
    }

    return 1;
}

static int MapCurrentStackRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    unsigned long long stackPointer = OrynVirtualMemoryReadRsp();
    unsigned long long halfWindow = ORYN_EARLY_STACK_BYTES / 2ULL;
    unsigned long long start = stackPointer > halfWindow ? stackPointer - halfWindow : 0ULL;

    virtualMemory->CurrentStackPointer = stackPointer;
    virtualMemory->StackMapStart = AlignDown(start);
    virtualMemory->StackMapEnd = AlignUp(stackPointer + halfWindow);
    return MapKernelOrIdentityRange(
        pml4,
        bootInfo,
        virtualMemory->StackMapStart,
        virtualMemory->StackMapEnd - virtualMemory->StackMapStart,
        physicalMemory,
        virtualMemory);
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

static int MapKernelVirtualRange(
    OrynPageTableEntry* pml4,
    const OrynBootInfo* bootInfo,
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelVirtualMemory* virtualMemory)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_VIRTUAL_LAYOUT))
    {
        virtualMemory->MapFailure = 1U;
        return 0;
    }

    virtualMemory->KernelVirtualMapStart = AlignDown(bootInfo->KernelVirtualBase);
    virtualMemory->KernelVirtualMapEnd = AlignUp(bootInfo->KernelVirtualBase + bootInfo->KernelVirtualSize);
    virtualMemory->KernelEntryPhysical = bootInfo->KernelEntryPhysical;
    virtualMemory->KernelEntryVirtual = bootInfo->KernelEntryVirtual;

    return MapVirtualRangeToPhysical(
        pml4,
        bootInfo->KernelVirtualBase,
        bootInfo->KernelPhysicalBase,
        bootInfo->KernelVirtualSize,
        physicalMemory,
        virtualMemory);
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
    return MapKernelOrIdentityRange(pml4, bootInfo, start, bytes, physicalMemory, virtualMemory);
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
        virtualMemory->FramebufferMapStart = AlignDown(bootInfo->Framebuffer.Base);
        virtualMemory->FramebufferMapEnd = AlignUp(bootInfo->Framebuffer.Base + bootInfo->Framebuffer.Size);
    }
    else
    {
        virtualMemory->DefaultScreenMapStart = AlignDown(bootInfo->Framebuffer.Base);
        virtualMemory->DefaultScreenMapEnd = AlignUp(bootInfo->Framebuffer.Base + bootInfo->Framebuffer.Size);
    }

    if (!MapRange(pml4, bootInfo->Framebuffer.Base, bootInfo->Framebuffer.Size, physicalMemory, virtualMemory))
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

    ClearBytes(virtualMemory, sizeof(*virtualMemory));
    virtualMemory->CurrentCr3 = OrynVirtualMemoryReadCr3();
    virtualMemory->UserBase = ORYN_VIRTUAL_USER_BASE;
    virtualMemory->UserLimit = ORYN_VIRTUAL_USER_LIMIT;
    virtualMemory->KernelBase = ORYN_VIRTUAL_KERNEL_BASE;
    virtualMemory->KernelLimit = ORYN_VIRTUAL_KERNEL_LIMIT;
    KernelIoWriteString("[KERNEL] Virtual memory: current CR3 captured\n");

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
    KernelIoWriteString("[KERNEL] Virtual memory: new PML4 allocated below early direct-map limit\n");
    if (!MapRange(pml4, 0ULL, ORYN_LOW_IDENTITY_BYTES, physicalMemory, virtualMemory) ||
        !MapCurrentStackRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapVgaTextRange(pml4, physicalMemory, virtualMemory) ||
        !MapKernelRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapKernelVirtualRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapBootInfoRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapBootMemoryMapRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapFontRange(pml4, bootInfo, physicalMemory, virtualMemory) ||
        !MapFramebufferRange(pml4, bootInfo, physicalMemory, virtualMemory))
    {
        return 0;
    }

    KernelIoWriteString("[KERNEL] Virtual memory: required ranges mapped\n");
    KernelIoWriteString("[KERNEL] Virtual memory: switching CR3 to kernel-owned PML4\n");
    OrynVirtualMemoryWriteCr3(virtualMemory->NewPml4);
    KernelIoWriteString("[KERNEL] Virtual memory: CR3 switched to kernel-owned PML4\n");
    virtualMemory->Active = 1U;
    virtualMemory->Initialized = 1U;
    if (!OrynVirtualMemoryInitKernelAddressSpace(virtualMemory) ||
        !OrynVirtualMemoryValidateHigherHalfKernelMap(virtualMemory))
    {
        virtualMemory->MapFailure = 1U;
        return 0;
    }
    return 1;
}
