#include "KernelMemoryMap.h"

#define EFI_RESERVED_MEMORY_TYPE 0U
#define EFI_LOADER_CODE 1U
#define EFI_LOADER_DATA 2U
#define EFI_BOOT_SERVICES_CODE 3U
#define EFI_BOOT_SERVICES_DATA 4U
#define EFI_RUNTIME_SERVICES_CODE 5U
#define EFI_RUNTIME_SERVICES_DATA 6U
#define EFI_CONVENTIONAL_MEMORY 7U
#define EFI_UNUSABLE_MEMORY 8U
#define EFI_ACPI_RECLAIM_MEMORY 9U
#define EFI_ACPI_MEMORY_NVS 10U
#define EFI_MEMORY_MAPPED_IO 11U
#define EFI_MEMORY_MAPPED_IO_PORT_SPACE 12U
#define EFI_PAL_CODE 13U
#define EFI_PERSISTENT_MEMORY 14U

static unsigned long long PagesToBytes(unsigned long long pages)
{
    return pages * ORYN_KERNEL_PAGE_SIZE;
}

static int RangesOverlap(
    unsigned long long leftStart,
    unsigned long long leftBytes,
    unsigned long long rightStart,
    unsigned long long rightBytes)
{
    unsigned long long leftEnd = leftStart + leftBytes;
    unsigned long long rightEnd = rightStart + rightBytes;

    if (leftBytes == 0ULL || rightBytes == 0ULL)
    {
        return 0;
    }

    return leftStart < rightEnd && rightStart < leftEnd;
}

static OrynKernelMemoryType ClassifyUefiType(unsigned int sourceType)
{
    switch (sourceType)
    {
        case EFI_CONVENTIONAL_MEMORY:
            return OrynKernelMemoryUsable;
        case EFI_LOADER_CODE:
        case EFI_LOADER_DATA:
        case EFI_BOOT_SERVICES_CODE:
        case EFI_BOOT_SERVICES_DATA:
            return OrynKernelMemoryBootloaderReclaim;
        case EFI_ACPI_RECLAIM_MEMORY:
            return OrynKernelMemoryAcpiReclaim;
        case EFI_ACPI_MEMORY_NVS:
            return OrynKernelMemoryAcpiNvs;
        case EFI_MEMORY_MAPPED_IO:
        case EFI_MEMORY_MAPPED_IO_PORT_SPACE:
            return OrynKernelMemoryMmio;
        case EFI_UNUSABLE_MEMORY:
            return OrynKernelMemoryBad;
        case EFI_RUNTIME_SERVICES_CODE:
        case EFI_RUNTIME_SERVICES_DATA:
            return OrynKernelMemoryRuntime;
        case EFI_RESERVED_MEMORY_TYPE:
        case EFI_PAL_CODE:
        case EFI_PERSISTENT_MEMORY:
        default:
            return OrynKernelMemoryReserved;
    }
}

static OrynKernelMemoryType ClassifyEntry(const OrynBootInfo* bootInfo, const OrynBootMemoryEntry* source)
{
    OrynKernelMemoryType type = ClassifyUefiType(source->Type);
    unsigned long long entryBytes = PagesToBytes(source->PageCount);

    if ((bootInfo->Flags & ORYN_BOOTINFO_FLAG_KERNEL_RANGE) != 0ULL)
    {
        if (RangesOverlap(source->PhysicalStart, entryBytes, bootInfo->KernelPhysicalBase, bootInfo->KernelSize))
        {
            return OrynKernelMemoryKernelReserved;
        }
    }

    return type;
}

static void AddBytes(OrynKernelMemoryMap* memoryMap, OrynKernelMemoryType type, unsigned long long bytes)
{
    memoryMap->TotalBytes += bytes;

    switch (type)
    {
        case OrynKernelMemoryUsable:
            memoryMap->UsableBytes += bytes;
            break;
        case OrynKernelMemoryBootloaderReclaim:
            memoryMap->BootloaderReclaimBytes += bytes;
            break;
        case OrynKernelMemoryAcpiReclaim:
            memoryMap->AcpiReclaimBytes += bytes;
            break;
        case OrynKernelMemoryAcpiNvs:
            memoryMap->AcpiNvsBytes += bytes;
            break;
        case OrynKernelMemoryMmio:
            memoryMap->MmioBytes += bytes;
            break;
        case OrynKernelMemoryBad:
            memoryMap->BadBytes += bytes;
            break;
        case OrynKernelMemoryKernelReserved:
            memoryMap->KernelReservedBytes += bytes;
            break;
        case OrynKernelMemoryRuntime:
            memoryMap->RuntimeBytes += bytes;
            break;
        case OrynKernelMemoryReserved:
        default:
            memoryMap->ReservedBytes += bytes;
            break;
    }
}

const char* OrynMemoryMapTypeName(OrynKernelMemoryType type)
{
    switch (type)
    {
        case OrynKernelMemoryUsable:
            return "usable";
        case OrynKernelMemoryBootloaderReclaim:
            return "bootloader reclaim";
        case OrynKernelMemoryAcpiReclaim:
            return "ACPI reclaim";
        case OrynKernelMemoryAcpiNvs:
            return "ACPI NVS";
        case OrynKernelMemoryMmio:
            return "MMIO";
        case OrynKernelMemoryBad:
            return "bad";
        case OrynKernelMemoryKernelReserved:
            return "kernel reserved";
        case OrynKernelMemoryRuntime:
            return "runtime";
        case OrynKernelMemoryReserved:
        default:
            return "reserved";
    }
}

int OrynMemoryMapBuildFromBootInfo(const OrynBootInfo* bootInfo, OrynKernelMemoryMap* memoryMap)
{
    unsigned long long sourceCount;
    const unsigned char* sourceBytes;

    if (memoryMap == 0)
    {
        return 0;
    }

    for (unsigned int index = 0; index < sizeof(*memoryMap); ++index)
    {
        ((unsigned char*)memoryMap)[index] = 0U;
    }

    if (bootInfo == 0 || (bootInfo->Flags & ORYN_BOOTINFO_FLAG_MEMORY_MAP) == 0ULL)
    {
        return 0;
    }

    if (bootInfo->MemoryMap == 0ULL || bootInfo->MemoryMapEntryCount == 0ULL)
    {
        return 0;
    }

    if (bootInfo->MemoryMapEntrySize < sizeof(OrynBootMemoryEntry))
    {
        return 0;
    }

    sourceCount = bootInfo->MemoryMapEntryCount;
    memoryMap->SourceEntryCount = (unsigned int)sourceCount;
    if (sourceCount > ORYN_KERNEL_MEMORY_MAX_ENTRIES)
    {
        sourceCount = ORYN_KERNEL_MEMORY_MAX_ENTRIES;
        memoryMap->Truncated = 1U;
    }

    sourceBytes = (const unsigned char*)(unsigned long long)bootInfo->MemoryMap;
    for (unsigned long long index = 0; index < sourceCount; ++index)
    {
        const OrynBootMemoryEntry* source = (const OrynBootMemoryEntry*)(sourceBytes + (index * bootInfo->MemoryMapEntrySize));
        OrynKernelMemoryEntry* target = &memoryMap->Entries[memoryMap->EntryCount];
        OrynKernelMemoryType type = ClassifyEntry(bootInfo, source);
        unsigned long long bytes = PagesToBytes(source->PageCount);

        target->Type = type;
        target->SourceType = source->Type;
        target->Reserved = 0U;
        target->PhysicalStart = source->PhysicalStart;
        target->PageCount = source->PageCount;
        target->Attribute = source->Attribute;
        memoryMap->EntryCount += 1U;
        AddBytes(memoryMap, type, bytes);
    }

    return 1;
}
