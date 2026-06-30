#include "BootX64Internal.h"
#include "OrynBootInfoSelection.h"

static EFI_STATUS AllocateMemoryMapStorage(
    EFI_MEMORY_DESCRIPTOR** outMemoryMap,
    UINTN* outMemoryMapSize,
    OrynBootMemoryEntry** outEntries,
    UINTN* outEntryCapacity)
{
    UINTN memoryMapSize = 0;
    UINTN mapKey = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;

    EFI_STATUS status = gBootServices->GetMemoryMap(
        &memoryMapSize,
        ORYN_NULL,
        &mapKey,
        &descriptorSize,
        &descriptorVersion);

    if (descriptorSize == 0)
    {
        descriptorSize = sizeof(EFI_MEMORY_DESCRIPTOR);
    }

    if (status != EFI_BUFFER_TOO_SMALL && IsError(status))
    {
        Print("[BOOT] FAIL: Could not size memory map. Status ");
        PrintHex64(status);
        Print(".\n");
        return status;
    }

    memoryMapSize += descriptorSize * 32U;
    EFI_MEMORY_DESCRIPTOR* memoryMap = ORYN_NULL;
    status = gBootServices->AllocatePool(EfiLoaderData, memoryMapSize, (void**)&memoryMap);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not allocate raw memory map.\n");
        return status;
    }

    UINTN entryCapacity = (memoryMapSize / descriptorSize) + 16U;
    OrynBootMemoryEntry* entries = ORYN_NULL;
    status = gBootServices->AllocatePool(EfiLoaderData, entryCapacity * sizeof(OrynBootMemoryEntry), (void**)&entries);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not allocate BootInfo memory entries.\n");
        return status;
    }

    *outMemoryMap = memoryMap;
    *outMemoryMapSize = memoryMapSize;
    *outEntries = entries;
    *outEntryCapacity = entryCapacity;
    return EFI_SUCCESS;
}

static void CopyMemoryMapEntries(
    OrynBootInfo* bootInfo,
    EFI_MEMORY_DESCRIPTOR* memoryMap,
    UINTN mapSize,
    UINTN descriptorSize,
    UINT32 descriptorVersion,
    OrynBootMemoryEntry* entries,
    UINTN entryCapacity)
{
    UINTN count = mapSize / descriptorSize;
    if (count > entryCapacity)
    {
        count = entryCapacity;
    }

    for (UINTN index = 0; index < count; ++index)
    {
        EFI_MEMORY_DESCRIPTOR* source = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)memoryMap + (index * descriptorSize));
        entries[index].Type = source->Type;
        entries[index].Reserved = 0;
        entries[index].PhysicalStart = source->PhysicalStart;
        entries[index].VirtualStart = source->VirtualStart;
        entries[index].PageCount = source->NumberOfPages;
        entries[index].Attribute = source->Attribute;
    }

    bootInfo->MemoryMap = (UINT64)(UINTN)entries;
    bootInfo->MemoryMapEntryCount = count;
    bootInfo->MemoryMapEntrySize = sizeof(OrynBootMemoryEntry);
    bootInfo->MemoryMapVersion = descriptorVersion;
    bootInfo->Flags |= ORYN_BOOTINFO_FLAG_MEMORY_MAP;
}

EFI_STATUS ExitBootServicesWithBootInfo(EFI_HANDLE imageHandle, OrynBootInfo* bootInfo)
{
    EFI_MEMORY_DESCRIPTOR* memoryMap = ORYN_NULL;
    OrynBootMemoryEntry* entries = ORYN_NULL;
    UINTN memoryMapSize = 0;
    UINTN entryCapacity = 0;
    EFI_STATUS status = AllocateMemoryMapStorage(&memoryMap, &memoryMapSize, &entries, &entryCapacity);
    if (IsError(status))
    {
        return status;
    }

    Print("[BOOT] Stage 06: Reading UEFI memory map for BootInfo.\n");
    for (int attempt = 0; attempt < 4; ++attempt)
    {
        UINTN mapSize = memoryMapSize;
        UINTN mapKey = 0;
        UINTN descriptorSize = 0;
        UINT32 descriptorVersion = 0;
        status = gBootServices->GetMemoryMap(
            &mapSize,
            memoryMap,
            &mapKey,
            &descriptorSize,
            &descriptorVersion);

        if (IsError(status))
        {
            Print("[BOOT] FAIL: Could not read memory map. Status ");
            PrintHex64(status);
            Print(".\n");
            return status;
        }

#if ORYN_BOOTINFO_WANT_MEMORY_MAP
        CopyMemoryMapEntries(bootInfo, memoryMap, mapSize, descriptorSize, descriptorVersion, entries, entryCapacity);
        Print("[BOOT] BootInfo memory map entries: ");
        PrintHex64(bootInfo->MemoryMapEntryCount);
        Print("\n");
#else
        (void)entries;
        (void)entryCapacity;
        Print("[BOOT] BootInfo memory map: disabled by user selection.\n");
#endif

        status = gBootServices->ExitBootServices(imageHandle, mapKey);
        if (!IsError(status))
        {
            Print("[BOOT] Stage 07: ExitBootServices succeeded.\n");
            return EFI_SUCCESS;
        }
    }

    Print("[BOOT] FAIL: ExitBootServices failed. Status ");
    PrintHex64(status);
    Print(".\n");
    return status;
}

__attribute__((noreturn)) void JumpToKernel(UINT64 kernelEntry, OrynBootInfo* bootInfo)
{
    Print("[BOOT] Stage 08: Jumping to kernel entry with one plain OrynBootInfo pointer in RDI.\n");
    Print("[BOOT] Kernel handoff excludes EFI_HANDLE, EFI_SYSTEM_TABLE, BootServices, and GOP objects.\n");
    __asm__ volatile (
        "movq %0, %%rdi\n"
        "jmp *%1\n"
        :
        : "r"(bootInfo), "r"((void*)(UINTN)kernelEntry)
        : "rdi", "memory");

    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}
