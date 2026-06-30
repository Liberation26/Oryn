#include "BootX64Internal.h"

static UINT64 AlignDown(UINT64 value)
{
    return value & ~(PAGE_SIZE - 1ULL);
}

static UINT64 AlignUp(UINT64 value)
{
    return (value + PAGE_SIZE - 1ULL) & ~(PAGE_SIZE - 1ULL);
}

static int ValidateElf(const Elf64_Ehdr* elf, UINTN fileSize)
{
    if (fileSize < sizeof(Elf64_Ehdr))
    {
        return 0;
    }

    if (elf->e_ident[0] != ELF_MAGIC_0 || elf->e_ident[1] != ELF_MAGIC_1 ||
        elf->e_ident[2] != ELF_MAGIC_2 || elf->e_ident[3] != ELF_MAGIC_3)
    {
        return 0;
    }

    if (elf->e_ident[4] != ELF_CLASS_64 || elf->e_ident[5] != ELF_DATA_LITTLE)
    {
        return 0;
    }

    if (elf->e_type != ELF_TYPE_EXEC || elf->e_machine != ELF_MACHINE_X86_64)
    {
        return 0;
    }

    if (elf->e_phoff + ((UINT64)elf->e_phentsize * elf->e_phnum) > fileSize)
    {
        return 0;
    }

    return 1;
}

static EFI_STATUS FindElfLayout(
    const void* kernelBuffer,
    UINTN kernelSize,
    OrynKernelElfLayout* layout,
    UINT64* outRequestedPhysicalStart)
{
    const Elf64_Ehdr* elf = (const Elf64_Ehdr*)kernelBuffer;
    const UINT8* bytes = (const UINT8*)kernelBuffer;
    UINT64 physicalStart = 0xFFFFFFFFFFFFFFFFULL;
    UINT64 physicalEnd = 0;
    UINT64 virtualStart = 0xFFFFFFFFFFFFFFFFULL;
    UINT64 virtualEnd = 0;
    UINT64 entryPhysical = 0;
    int loadCount = 0;

    for (UINT16 index = 0; index < elf->e_phnum; ++index)
    {
        const Elf64_Phdr* header = (const Elf64_Phdr*)(bytes + elf->e_phoff + ((UINT64)index * elf->e_phentsize));
        if (header->p_type != ELF_PT_LOAD)
        {
            continue;
        }

        if (header->p_offset + header->p_filesz > kernelSize || header->p_memsz < header->p_filesz)
        {
            Print("[BOOT] FAIL: Kernel ELF segment is invalid.\n");
            return EFI_LOAD_ERROR;
        }

        UINT64 segmentPhysicalStart = AlignDown(header->p_paddr);
        UINT64 segmentPhysicalEnd = AlignUp(header->p_paddr + header->p_memsz);
        UINT64 segmentVirtualStart = AlignDown(header->p_vaddr);
        UINT64 segmentVirtualEnd = AlignUp(header->p_vaddr + header->p_memsz);

        if (segmentPhysicalStart < physicalStart)
        {
            physicalStart = segmentPhysicalStart;
        }
        if (segmentPhysicalEnd > physicalEnd)
        {
            physicalEnd = segmentPhysicalEnd;
        }
        if (segmentVirtualStart < virtualStart)
        {
            virtualStart = segmentVirtualStart;
        }
        if (segmentVirtualEnd > virtualEnd)
        {
            virtualEnd = segmentVirtualEnd;
        }

        if (elf->e_entry >= header->p_vaddr && elf->e_entry < header->p_vaddr + header->p_memsz)
        {
            entryPhysical = header->p_paddr + (elf->e_entry - header->p_vaddr);
        }

        ++loadCount;
    }

    if (loadCount == 0 || physicalStart >= physicalEnd || virtualStart >= virtualEnd || entryPhysical == 0ULL)
    {
        Print("[BOOT] FAIL: Kernel ELF load layout is invalid.\n");
        return EFI_LOAD_ERROR;
    }

    if (physicalEnd - physicalStart != virtualEnd - virtualStart)
    {
        Print("[BOOT] FAIL: Kernel ELF physical and virtual layout sizes differ.\n");
        return EFI_LOAD_ERROR;
    }

    layout->PhysicalBase = physicalStart;
    layout->PhysicalSize = physicalEnd - physicalStart;
    layout->VirtualBase = virtualStart;
    layout->VirtualSize = virtualEnd - virtualStart;
    layout->EntryPhysical = entryPhysical;
    layout->EntryVirtual = elf->e_entry;
    *outRequestedPhysicalStart = physicalStart;
    return EFI_SUCCESS;
}

static EFI_STATUS AllocateKernelRange(UINT64 requestedStart, UINT64 requestedSize, UINT64* outActualStart)
{
    UINTN pages = (UINTN)(requestedSize / PAGE_SIZE);
    EFI_PHYSICAL_ADDRESS loadAddress = requestedStart;

    Print("[BOOT] Kernel requested physical load base: ");
    PrintHex64(requestedStart);
    Print("\n");
    Print("[BOOT] Kernel physical load size: ");
    PrintHex64(requestedSize);
    Print(" bytes.\n");

    EFI_STATUS status = gBootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &loadAddress);
    if (!IsError(status))
    {
        *outActualStart = requestedStart;
        Print("[BOOT] Kernel loaded physical base: ");
        PrintHex64(*outActualStart);
        Print("\n");
        return EFI_SUCCESS;
    }

    Print("[BOOT] WARN: Fixed-address allocation failed. Trying relocatable load. Status ");
    PrintHex64(status);
    Print(".\n");

    loadAddress = 0;
    status = gBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &loadAddress);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not allocate physical pages for kernel. Status ");
        PrintHex64(status);
        Print(".\n");
        return status;
    }

    *outActualStart = (UINT64)loadAddress;
    Print("[BOOT] Kernel loaded physical base: ");
    PrintHex64(*outActualStart);
    Print("\n");
    return EFI_SUCCESS;
}

static EFI_STATUS CopyElfSegments(
    const void* kernelBuffer,
    UINTN kernelSize,
    UINT64 requestedPhysicalStart,
    UINT64 actualPhysicalStart,
    OrynKernelElfLayout* layout)
{
    const Elf64_Ehdr* elf = (const Elf64_Ehdr*)kernelBuffer;
    const UINT8* bytes = (const UINT8*)kernelBuffer;

    (void)kernelSize;
    for (UINT16 index = 0; index < elf->e_phnum; ++index)
    {
        const Elf64_Phdr* header = (const Elf64_Phdr*)(bytes + elf->e_phoff + ((UINT64)index * elf->e_phentsize));
        if (header->p_type != ELF_PT_LOAD)
        {
            continue;
        }

        UINT64 destination = actualPhysicalStart + (header->p_paddr - requestedPhysicalStart);
        SetMemory((void*)(UINTN)destination, 0, (UINTN)header->p_memsz);
        CopyMemory((void*)(UINTN)destination, bytes + header->p_offset, (UINTN)header->p_filesz);
    }

    layout->EntryPhysical = actualPhysicalStart + (layout->EntryPhysical - requestedPhysicalStart);
    layout->PhysicalBase = actualPhysicalStart;

    Print("[BOOT] Kernel physical entry address: ");
    PrintHex64(layout->EntryPhysical);
    Print("\n");
    Print("[BOOT] Kernel virtual entry address: ");
    PrintHex64(layout->EntryVirtual);
    Print("\n");
    Print("[BOOT] Kernel chosen virtual base: ");
    PrintHex64(layout->VirtualBase);
    Print("\n");
    return EFI_SUCCESS;
}

EFI_STATUS LoadElfSegments(const void* kernelBuffer, UINTN kernelSize, OrynKernelElfLayout* outLayout)
{
    const Elf64_Ehdr* elf = (const Elf64_Ehdr*)kernelBuffer;
    if (!ValidateElf(elf, kernelSize))
    {
        Print("[BOOT] FAIL: kernel image is not a valid x86_64 executable ELF.\n");
        return EFI_LOAD_ERROR;
    }

    Print("[BOOT] Stage 04: Validated x86_64 ELF kernel.\n");
    OrynKernelElfLayout layout;
    UINT64 requestedPhysicalStart = 0;
    EFI_STATUS status = FindElfLayout(kernelBuffer, kernelSize, &layout, &requestedPhysicalStart);
    if (IsError(status))
    {
        return status;
    }

    UINT64 actualPhysicalStart = 0;
    status = AllocateKernelRange(requestedPhysicalStart, layout.PhysicalSize, &actualPhysicalStart);
    if (IsError(status))
    {
        return status;
    }

    Print("[BOOT] Stage 05: Copying ELF load segments.\n");
    status = CopyElfSegments(kernelBuffer, kernelSize, requestedPhysicalStart, actualPhysicalStart, &layout);
    if (IsError(status))
    {
        return status;
    }

    outLayout->PhysicalBase = layout.PhysicalBase;
    outLayout->PhysicalSize = layout.PhysicalSize;
    outLayout->VirtualBase = layout.VirtualBase;
    outLayout->VirtualSize = layout.VirtualSize;
    outLayout->EntryPhysical = layout.EntryPhysical;
    outLayout->EntryVirtual = layout.EntryVirtual;
    return EFI_SUCCESS;
}
