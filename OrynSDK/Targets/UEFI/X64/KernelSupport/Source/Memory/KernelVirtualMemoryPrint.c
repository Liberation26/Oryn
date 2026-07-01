#include "KernelVirtualMemory.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

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

    KernelIoWriteString("[KERNEL] Current stack pointer: ");
    KernelIoWriteHex64(virtualMemory->CurrentStackPointer);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] New PML4: ");
    KernelIoWriteHex64(virtualMemory->NewPml4);
    KernelIoWriteString("\n");

    WriteRange("[KERNEL] Identity mapped kernel range: ",
        virtualMemory->KernelMapStart,
        virtualMemory->KernelMapEnd);

    WriteRange("[KERNEL] Higher-half/chosen kernel virtual range: ",
        virtualMemory->KernelVirtualMapStart,
        virtualMemory->KernelVirtualMapEnd);

    KernelIoWriteString("[KERNEL] Kernel physical entry: ");
    KernelIoWriteHex64(virtualMemory->KernelEntryPhysical);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Kernel virtual entry: ");
    KernelIoWriteHex64(virtualMemory->KernelEntryVirtual);
    KernelIoWriteString("\n");

    WriteRange("[KERNEL] Mapped BootInfo range: ",
        virtualMemory->BootInfoMapStart,
        virtualMemory->BootInfoMapEnd);

    WriteRange("[KERNEL] Identity mapped memory map: ",
        virtualMemory->MemoryMapMapStart,
        virtualMemory->MemoryMapMapEnd);

    WriteRange("[KERNEL] Mapped current stack range: ",
        virtualMemory->StackMapStart,
        virtualMemory->StackMapEnd);

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

    KernelIoWriteString("[KERNEL] Kernel virtual mapped pages: ");
    KernelIoWriteDec64(virtualMemory->KernelVirtualMappedPages);
    KernelIoWriteString("\n");

    WriteRange("[KERNEL] User address split: ",
        virtualMemory->UserBase,
        virtualMemory->UserLimit);

    WriteRange("[KERNEL] Kernel address split: ",
        virtualMemory->KernelBase,
        virtualMemory->KernelLimit);

    KernelIoWriteString("[KERNEL] VM API mapped pages: ");
    KernelIoWriteDec64(virtualMemory->ApiMappedPages);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] VM API protected pages: ");
    KernelIoWriteDec64(virtualMemory->ApiProtectedPages);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] VM API unmapped pages: ");
    KernelIoWriteDec64(virtualMemory->ApiUnmappedPages);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Process address spaces created: ");
    KernelIoWriteDec64(virtualMemory->ProcessAddressSpacesCreated);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Anonymous user regions created: ");
    KernelIoWriteDec64(virtualMemory->AnonymousRegionsCreated);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Demand-allocated user pages: ");
    KernelIoWriteDec64(virtualMemory->DemandAllocatedUserPages);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] User copy bytes from user: ");
    KernelIoWriteDec64(virtualMemory->UserCopyBytesIn);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] User copy bytes to user: ");
    KernelIoWriteDec64(virtualMemory->UserCopyBytesOut);
    KernelIoWriteString("\n");

    KernelIoWriteString("[KERNEL] Copy-on-write clones: ");
    KernelIoWriteDec64(virtualMemory->CopyOnWriteCloneCount);
    KernelIoWriteString(" shared pages=");
    KernelIoWriteDec64(virtualMemory->CopyOnWriteSharedPages);
    KernelIoWriteString(" resolved pages=");
    KernelIoWriteDec64(virtualMemory->CopyOnWriteResolvedPages);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrFail(virtualMemory->CopyOnWriteCloneCount != 0ULL &&
        virtualMemory->CopyOnWriteSharedPages != 0ULL &&
        virtualMemory->CopyOnWriteResolvedPages != 0ULL,
        "Copy-on-write foundation can clone and privatize user pages.",
        "Copy-on-write foundation proof failed.");

    KernelIoWriteString("[KERNEL] Page-fault policy: ");
    KernelIoWriteString(virtualMemory->PageFaultPolicyReady ? "ready\n" : "not proven\n");

    if (virtualMemory->Active)
    {
        KernelIoWriteString("[KERNEL] Virtual memory: active\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] Virtual memory: inactive\n");
    }
}
