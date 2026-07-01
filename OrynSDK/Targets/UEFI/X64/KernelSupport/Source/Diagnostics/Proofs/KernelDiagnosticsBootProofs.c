#include "KernelDiagnostics.h"
#include "KernelDiagnosticsProofsInternal.h"
#include "KernelModuleManifest.h"
#include "KernelScreenReport.h"

static void OrynKernelDiagnosticsPrintBootInfoOwnership(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelScreenReportOk(0, "Kernel entry received one plain OrynBootInfo pointer.");
    if (KernelBootInfoIsKernelOwned(kernelBootInfo))
    {
        OrynKernelScreenReportOk(0, "OrynBootInfo copied into kernel-owned storage.");
        KernelIoWriteString("[KERNEL] Loader BootInfo pointer: ");
        KernelIoWriteHex64(KernelBootInfoSourceAddress());
        KernelIoWriteString("\n");
        KernelIoWriteString("[KERNEL] Kernel BootInfo copy: ");
        KernelIoWriteHex64((unsigned long long)kernelBootInfo);
        KernelIoWriteString("\n");
    }
    else
    {
        OrynKernelScreenReportFail(0, "Kernel could not adopt OrynBootInfo.");
    }
}

void OrynKernelDiagnosticsPrintEntryProofs(const OrynBootInfo* kernelBootInfo)
{
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 entered.\n");
    OrynKernelScreenReportOk(0, "Kernel entered successfully.");
    OrynKernelScreenReportOk(0, "Serial/debug output path is working.");
    OrynKernelDiagnosticsPrintBootInfoOwnership(kernelBootInfo);
    KernelBootInfoPrintSelection();
}

void OrynKernelDiagnosticsRunBootInfoFailureProof(void)
{
    KernelIoWriteString("[KERNEL] BootInfo invalid. Memory services are disabled.\n");
    OrynKernelPanicBegin(
        "BootInfo validation failed",
        "Kernel-owned panic report path handles invalid handoff data",
        0xB0070001ULL);
}
