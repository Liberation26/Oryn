#include "KernelVirtualMemory.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

static void WriteRange(const char* label, unsigned long long start, unsigned long long end)
{
    OrynKernelDiagnosticsLogText(label);
    OrynKernelDiagnosticsLogHex64(start);
    OrynKernelDiagnosticsLogText(" - ");
    OrynKernelDiagnosticsLogHex64(end);
    OrynKernelDiagnosticsLogText("\n");
}

void OrynVirtualMemoryPrintProof(const OrynKernelVirtualMemory* virtualMemory)
{
    if (virtualMemory == 0)
    {
        OrynKernelDiagnosticsLogText("[KERNEL] Virtual memory: unavailable\n");
        return;
    }

    OrynKernelDiagnosticsLogText("[KERNEL] Current CR3: ");
    OrynKernelDiagnosticsLogHex64(virtualMemory->CurrentCr3);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Current stack pointer: ");
    OrynKernelDiagnosticsLogHex64(virtualMemory->CurrentStackPointer);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] New PML4: ");
    OrynKernelDiagnosticsLogHex64(virtualMemory->NewPml4);
    OrynKernelDiagnosticsLogText("\n");

    WriteRange("[KERNEL] Identity mapped kernel range: ",
        virtualMemory->KernelMapStart,
        virtualMemory->KernelMapEnd);

    WriteRange("[KERNEL] Higher-half/chosen kernel virtual range: ",
        virtualMemory->KernelVirtualMapStart,
        virtualMemory->KernelVirtualMapEnd);

    OrynKernelDiagnosticsLogText("[KERNEL] Kernel physical entry: ");
    OrynKernelDiagnosticsLogHex64(virtualMemory->KernelEntryPhysical);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Kernel virtual entry: ");
    OrynKernelDiagnosticsLogHex64(virtualMemory->KernelEntryVirtual);
    OrynKernelDiagnosticsLogText("\n");

    WriteRange("[KERNEL] Mapped BootInfo range: ",
        virtualMemory->BootInfoMapStart,
        virtualMemory->BootInfoMapEnd);

    WriteRange("[KERNEL] Identity mapped memory map: ",
        virtualMemory->MemoryMapMapStart,
        virtualMemory->MemoryMapMapEnd);

    WriteRange("[KERNEL] Mapped current stack range: ",
        virtualMemory->StackMapStart,
        virtualMemory->StackMapEnd);

    OrynKernelDiagnosticsLogText("[KERNEL] Framebuffer mapping: ");
    if (virtualMemory->FramebufferSelected)
    {
        OrynKernelDiagnosticsLogText("selected ");
        OrynKernelDiagnosticsLogHex64(virtualMemory->FramebufferMapStart);
        OrynKernelDiagnosticsLogText(" - ");
        OrynKernelDiagnosticsLogHex64(virtualMemory->FramebufferMapEnd);
        OrynKernelDiagnosticsLogText("\n");
    }
    else
    {
        OrynKernelDiagnosticsLogText("not selected\n");
    }

    OrynKernelDiagnosticsLogText("[KERNEL] Default screen mapping: ");
    if (virtualMemory->DefaultScreenMapped)
    {
        OrynKernelDiagnosticsLogHex64(virtualMemory->DefaultScreenMapStart);
        OrynKernelDiagnosticsLogText(" - ");
        OrynKernelDiagnosticsLogHex64(virtualMemory->DefaultScreenMapEnd);
        OrynKernelDiagnosticsLogText("\n");
    }
    else
    {
        OrynKernelDiagnosticsLogText("not available\n");
    }

    OrynKernelDiagnosticsLogText("[KERNEL] VGA text mapping: ");
    if (virtualMemory->VgaTextMapped)
    {
        OrynKernelDiagnosticsLogHex64(virtualMemory->VgaTextMapStart);
        OrynKernelDiagnosticsLogText(" - ");
        OrynKernelDiagnosticsLogHex64(virtualMemory->VgaTextMapEnd);
        OrynKernelDiagnosticsLogText("\n");
    }
    else
    {
        OrynKernelDiagnosticsLogText("not mapped\n");
    }

    OrynKernelDiagnosticsLogText("[KERNEL] TTF font mapping: ");
    if (virtualMemory->FontMapped)
    {
        OrynKernelDiagnosticsLogHex64(virtualMemory->FontMapStart);
        OrynKernelDiagnosticsLogText(" - ");
        OrynKernelDiagnosticsLogHex64(virtualMemory->FontMapEnd);
        OrynKernelDiagnosticsLogText("\n");
    }
    else
    {
        OrynKernelDiagnosticsLogText("not supplied\n");
    }

    OrynKernelDiagnosticsLogText("[KERNEL] Page table pages allocated: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->TablesAllocated);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Identity mapped pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->IdentityMappedPages);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Kernel virtual mapped pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->KernelVirtualMappedPages);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Higher-half validation pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->HigherHalfMappedPageProofCount);
    OrynKernelDiagnosticsLogText(" failures=");
    OrynKernelDiagnosticsLogDec64(virtualMemory->HigherHalfValidationFailures);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelScreenReportOkOrFail(virtualMemory->HigherHalfReady != 0U &&
        virtualMemory->HigherHalfCanonical != 0U &&
        virtualMemory->HigherHalfAligned != 0U &&
        virtualMemory->HigherHalfNoUserOverlap != 0U &&
        virtualMemory->HigherHalfEntryMapped != 0U &&
        virtualMemory->HigherHalfPml4SlotReady != 0U &&
        virtualMemory->HigherHalfPhysicalWindowValid != 0U &&
        virtualMemory->HigherHalfValidationFailures == 0ULL,
        "Higher-half kernel mapping is validated page-for-page.",
        "Higher-half kernel mapping validation failed.");

    WriteRange("[KERNEL] User address split: ",
        virtualMemory->UserBase,
        virtualMemory->UserLimit);

    WriteRange("[KERNEL] Kernel address split: ",
        virtualMemory->KernelBase,
        virtualMemory->KernelLimit);

    OrynKernelDiagnosticsLogText("[KERNEL] VM API mapped pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->ApiMappedPages);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] VM API protected pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->ApiProtectedPages);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] VM API unmapped pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->ApiUnmappedPages);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Process address spaces created: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->ProcessAddressSpacesCreated);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Anonymous user regions created: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->AnonymousRegionsCreated);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] mmap-style regions created: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->MmapRegionsCreated);
    OrynKernelDiagnosticsLogText(" file=");
    OrynKernelDiagnosticsLogDec64(virtualMemory->FileMmapRegionsCreated);
    OrynKernelDiagnosticsLogText(" device=");
    OrynKernelDiagnosticsLogDec64(virtualMemory->DeviceMmapRegionsCreated);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] W^X policy checks: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->WriteExecutePolicyChecks);
    OrynKernelDiagnosticsLogText(" denied=");
    OrynKernelDiagnosticsLogDec64(virtualMemory->WriteExecuteDeniedCount);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Demand-allocated user pages: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->DemandAllocatedUserPages);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] User copy bytes from user: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->UserCopyBytesIn);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] User copy bytes to user: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->UserCopyBytesOut);
    OrynKernelDiagnosticsLogText("\n");

    OrynKernelDiagnosticsLogText("[KERNEL] Copy-on-write clones: ");
    OrynKernelDiagnosticsLogDec64(virtualMemory->CopyOnWriteCloneCount);
    OrynKernelDiagnosticsLogText(" shared pages=");
    OrynKernelDiagnosticsLogDec64(virtualMemory->CopyOnWriteSharedPages);
    OrynKernelDiagnosticsLogText(" resolved pages=");
    OrynKernelDiagnosticsLogDec64(virtualMemory->CopyOnWriteResolvedPages);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(virtualMemory->CopyOnWriteCloneCount != 0ULL &&
        virtualMemory->CopyOnWriteSharedPages != 0ULL &&
        virtualMemory->CopyOnWriteResolvedPages != 0ULL,
        "Copy-on-write foundation can clone and privatize user pages.",
        "Copy-on-write foundation proof failed.");
    OrynKernelScreenReportOkOrFail(virtualMemory->MmapRegionsCreated >= 3ULL &&
        virtualMemory->FileMmapRegionsCreated != 0ULL &&
        virtualMemory->DeviceMmapRegionsCreated != 0ULL,
        "mmap-style region tracking covers anonymous, file, and device memory.",
        "mmap-style region tracking proof failed.");
    OrynKernelScreenReportOkOrFail(virtualMemory->WriteExecutePolicyChecks != 0ULL &&
        virtualMemory->WriteExecuteDeniedCount != 0ULL,
        "W^X page policy rejects writable executable mappings.",
        "W^X page policy proof failed.");

    OrynKernelDiagnosticsLogText("[KERNEL] Page-fault policy: ");
    OrynKernelDiagnosticsLogText(virtualMemory->PageFaultPolicyReady ? "ready\n" : "not proven\n");

    if (virtualMemory->Active)
    {
        OrynKernelDiagnosticsLogText("[KERNEL] Virtual memory: active\n");
    }
    else
    {
        OrynKernelDiagnosticsLogText("[KERNEL] Virtual memory: inactive\n");
    }
}
