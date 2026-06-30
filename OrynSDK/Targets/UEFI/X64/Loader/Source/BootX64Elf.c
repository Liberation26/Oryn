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

static EFI_STATUS FindElfLoadRange(const void* kernelBuffer, UINTN kernelSize, UINT64* outStart, UINT64* outEnd)
{
    const Elf64_Ehdr* elf = (const Elf64_Ehdr*)kernelBuffer;
    const UINT8* bytes = (const UINT8*)kernelBuffer;
    UINT64 loadStart = 0xFFFFFFFFFFFFFFFFULL;
    UINT64 loadEnd = 0;
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

        UINT64 segmentStart = AlignDown(header->p_paddr);
        UINT64 segmentEnd = AlignUp(header->p_paddr + header->p_memsz);
        if (segmentStart < loadStart)
        {
            loadStart = segmentStart;
        }
        if (segmentEnd > loadEnd)
        {
            loadEnd = segmentEnd;
        }
        ++loadCount;
    }

    if (loadCount == 0 || loadStart >= loadEnd || elf->e_entry < loadStart || elf->e_entry >= loadEnd)
    {
        Print("[BOOT] FAIL: Kernel ELF load range is invalid.\n");
        return EFI_LOAD_ERROR;
    }

    *outStart = loadStart;
    *outEnd = loadEnd;
    return EFI_SUCCESS;
}

static EFI_STATUS AllocateKernelRange(UINT64 requestedStart, UINT64 requestedEnd, UINT64* outActualStart)
{
    UINTN pages = (UINTN)((requestedEnd - requestedStart) / PAGE_SIZE);
    EFI_PHYSICAL_ADDRESS loadAddress = requestedStart;

    Print("[BOOT] Kernel requested load base: ");
    PrintHex64(requestedStart);
    Print("\n");
    Print("[BOOT] Kernel load size: ");
    PrintHex64(requestedEnd - requestedStart);
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

static EFI_STATUS CopyElfSegments(const void* kernelBuffer, UINTN kernelSize, UINT64 requestedStart, UINT64 actualStart, UINT64 loadSize, UINT64* outEntry, UINT64* outKernelBase, UINT64* outKernelSize)
{
    const Elf64_Ehdr* elf = (const Elf64_Ehdr*)kernelBuffer;
    const UINT8* bytes = (const UINT8*)kernelBuffer;

    for (UINT16 index = 0; index < elf->e_phnum; ++index)
    {
        const Elf64_Phdr* header = (const Elf64_Phdr*)(bytes + elf->e_phoff + ((UINT64)index * elf->e_phentsize));
        if (header->p_type != ELF_PT_LOAD)
        {
            continue;
        }

        UINT64 destination = actualStart + (header->p_paddr - requestedStart);
        SetMemory((void*)(UINTN)destination, 0, (UINTN)header->p_memsz);
        CopyMemory((void*)(UINTN)destination, bytes + header->p_offset, (UINTN)header->p_filesz);
    }

    *outEntry = actualStart + (elf->e_entry - requestedStart);
    *outKernelBase = actualStart;
    *outKernelSize = loadSize;
    Print("[BOOT] Kernel entry address: ");
    PrintHex64(*outEntry);
    Print("\n");
    return EFI_SUCCESS;
}

EFI_STATUS LoadElfSegments(const void* kernelBuffer, UINTN kernelSize, UINT64* outEntry, UINT64* outKernelBase, UINT64* outKernelSize)
{
    const Elf64_Ehdr* elf = (const Elf64_Ehdr*)kernelBuffer;
    if (!ValidateElf(elf, kernelSize))
    {
        Print("[BOOT] FAIL: kernel image is not a valid x86_64 executable ELF.\n");
        return EFI_LOAD_ERROR;
    }

    Print("[BOOT] Stage 04: Validated x86_64 ELF kernel.\n");
    UINT64 requestedStart = 0;
    UINT64 requestedEnd = 0;
    EFI_STATUS status = FindElfLoadRange(kernelBuffer, kernelSize, &requestedStart, &requestedEnd);
    if (IsError(status))
    {
        return status;
    }

    UINT64 actualStart = 0;
    status = AllocateKernelRange(requestedStart, requestedEnd, &actualStart);
    if (IsError(status))
    {
        return status;
    }

    Print("[BOOT] Stage 05: Copying ELF load segments.\n");
    return CopyElfSegments(kernelBuffer, kernelSize, requestedStart, actualStart, requestedEnd - requestedStart,
        outEntry, outKernelBase, outKernelSize);
}
