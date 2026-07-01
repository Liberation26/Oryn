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
        OrynKernelDiagnosticsLogText("[KERNEL] Loader BootInfo pointer: ");
        OrynKernelDiagnosticsLogHex64(KernelBootInfoSourceAddress());
        OrynKernelDiagnosticsLogText("\n");
        OrynKernelDiagnosticsLogText("[KERNEL] Kernel BootInfo copy: ");
        OrynKernelDiagnosticsLogHex64((unsigned long long)kernelBootInfo);
        OrynKernelDiagnosticsLogText("\n");
    }
    else
    {
        OrynKernelScreenReportFail(0, "Kernel could not adopt OrynBootInfo.");
    }
}

void OrynKernelDiagnosticsPrintEntryProofs(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Oryn Kernel-5 entered.\n");
    OrynKernelScreenReportOk(0, "Kernel entered successfully.");
    OrynKernelScreenReportOk(0, "Serial/debug output path is working.");
    OrynKernelDiagnosticsPrintBootInfoOwnership(kernelBootInfo);
    KernelBootInfoPrintSelection();
}

void OrynKernelDiagnosticsRunBootInfoFailureProof(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] BootInfo invalid. Memory services are disabled.\n");
    OrynKernelPanicBegin(
        "BootInfo validation failed",
        "Kernel-owned panic report path handles invalid handoff data",
        0xB0070001ULL);
}
