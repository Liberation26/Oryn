#include "KernelVirtualMemory.h"
#include "KernelIo.h"

static void WriteRange(const char* label, unsigned long long start, unsigned long long end)
{
    KernelIoWriteString(label);
    KernelIoWriteHex64(start);
    KernelIoWriteString(" - ");
    KernelIoWriteHex64(end);
    KernelIoWriteString("\n");
}

void OrynVirtualMemoryPrintProof(const OrynKernelVirtualMemory* virtualMemory)
{
    if (virtualMemory == 0)
    {
        KernelIoWriteString("[KERNEL] Virtual memory: unavailable\n");
        return;
    }

    KernelIoWriteString("[KERNEL] Current CR3: ");
    KernelIoWriteHex64(virtualMemory->CurrentCr3);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] New PML4: ");
    KernelIoWriteHex64(virtualMemory->NewPml4);
    KernelIoWriteString("\n");

    WriteRange("[KERNEL] Identity mapped kernel range: ",
        virtualMemory->KernelMapStart,
        virtualMemory->KernelMapEnd);

    WriteRange("[KERNEL] Identity mapped BootInfo: ",
        virtualMemory->BootInfoMapStart,
        virtualMemory->BootInfoMapEnd);

    WriteRange("[KERNEL] Identity mapped memory map: ",
        virtualMemory->MemoryMapMapStart,
        virtualMemory->MemoryMapMapEnd);

    KernelIoWriteString("[KERNEL] Framebuffer mapping: ");
    if (virtualMemory->FramebufferSelected)
    {
        KernelIoWriteString("selected ");
        KernelIoWriteHex64(virtualMemory->FramebufferMapStart);
        KernelIoWriteString(" - ");
        KernelIoWriteHex64(virtualMemory->FramebufferMapEnd);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("not selected\n");
    }

    KernelIoWriteString("[KERNEL] Default screen mapping: ");
    if (virtualMemory->DefaultScreenMapped)
    {
        KernelIoWriteHex64(virtualMemory->DefaultScreenMapStart);
        KernelIoWriteString(" - ");
        KernelIoWriteHex64(virtualMemory->DefaultScreenMapEnd);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("not available\n");
    }

    KernelIoWriteString("[KERNEL] VGA text mapping: ");
    if (virtualMemory->VgaTextMapped)
    {
        KernelIoWriteHex64(virtualMemory->VgaTextMapStart);
        KernelIoWriteString(" - ");
        KernelIoWriteHex64(virtualMemory->VgaTextMapEnd);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("not mapped\n");
    }

    KernelIoWriteString("[KERNEL] TTF font mapping: ");
    if (virtualMemory->FontMapped)
    {
        KernelIoWriteHex64(virtualMemory->FontMapStart);
        KernelIoWriteString(" - ");
        KernelIoWriteHex64(virtualMemory->FontMapEnd);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("not supplied\n");
    }

    KernelIoWriteString("[KERNEL] Page table pages allocated: ");
    KernelIoWriteDec64(virtualMemory->TablesAllocated);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Identity mapped pages: ");
    KernelIoWriteDec64(virtualMemory->IdentityMappedPages);
    KernelIoWriteString("\n");

    if (virtualMemory->Active)
    {
        KernelIoWriteString("[KERNEL] Virtual memory: active\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] Virtual memory: inactive\n");
    }
}
