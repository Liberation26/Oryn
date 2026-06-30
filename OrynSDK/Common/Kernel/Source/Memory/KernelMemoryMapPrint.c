#include "KernelMemoryMap.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static void WriteBytesAsMb(const char* label, unsigned long long bytes)
{
    KernelIoWriteString(label);
    KernelIoWriteDec64(bytes / (1024ULL * 1024ULL));
    KernelIoWriteString(" MB\n");
}

static void WriteEntryLine(const OrynKernelMemoryEntry* entry, unsigned int index)
{
    KernelIoWriteString("  [");
    KernelIoWriteDec64(index);
    KernelIoWriteString("] ");
    KernelIoWriteString(OrynMemoryMapTypeName(entry->Type));
    KernelIoWriteString(" start ");
    KernelIoWriteHex64(entry->PhysicalStart);
    KernelIoWriteString(" pages ");
    KernelIoWriteDec64(entry->PageCount);
    KernelIoWriteString(" source ");
    KernelIoWriteDec64(entry->SourceType);
    KernelIoWriteString("\n");
}

void OrynMemoryMapPrintSummary(const OrynKernelMemoryMap* memoryMap)
{
    if (memoryMap == 0)
    {
        KernelIoWriteString("[KERNEL] Memory map parser: unavailable.\n");
        return;
    }

    KernelIoWriteString("[KERNEL] Memory map selected: yes\n");
    KernelIoWriteString("[KERNEL] Kernel memory map entries copied: ");
    KernelIoWriteDec64(memoryMap->EntryCount);
    KernelIoWriteString(" of ");
    KernelIoWriteDec64(memoryMap->SourceEntryCount);
    KernelIoWriteString("\n");

    if (memoryMap->Truncated)
    {
        OrynKernelScreenReportWarn(0, "Memory map was truncated to kernel static capacity.");
    }

    KernelIoWriteString("[KERNEL] Memory summary:\n");
    WriteBytesAsMb("  Usable: ", memoryMap->UsableBytes);
    WriteBytesAsMb("  Reserved: ", memoryMap->ReservedBytes);
    WriteBytesAsMb("  Bootloader reclaim: ", memoryMap->BootloaderReclaimBytes);
    WriteBytesAsMb("  ACPI reclaim: ", memoryMap->AcpiReclaimBytes);
    WriteBytesAsMb("  ACPI NVS: ", memoryMap->AcpiNvsBytes);
    WriteBytesAsMb("  MMIO: ", memoryMap->MmioBytes);
    WriteBytesAsMb("  Runtime: ", memoryMap->RuntimeBytes);
    WriteBytesAsMb("  Bad: ", memoryMap->BadBytes);
    WriteBytesAsMb("  Kernel reserved: ", memoryMap->KernelReservedBytes);
    WriteBytesAsMb("  Total described: ", memoryMap->TotalBytes);

    KernelIoWriteString("[KERNEL] First memory map entries:\n");
    unsigned int limit = memoryMap->EntryCount < 8U ? memoryMap->EntryCount : 8U;
    for (unsigned int index = 0; index < limit; ++index)
    {
        WriteEntryLine(&memoryMap->Entries[index], index);
    }

    if (memoryMap->EntryCount > limit)
    {
        KernelIoWriteString("  ...\n");
    }

}
