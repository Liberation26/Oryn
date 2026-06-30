#include "KernelDiagnostics.h"
#include "KernelDiagnosticsProofsInternal.h"

static void OrynKernelDiagnosticsPrintBootInfoOwnership(const OrynBootInfo* kernelBootInfo)
{
    KernelIoWriteString("[KERNEL] PASS: Kernel entry received one plain OrynBootInfo pointer.\n");
    if (KernelBootInfoIsKernelOwned(kernelBootInfo))
    {
        KernelIoWriteString("[KERNEL] PASS: OrynBootInfo copied into kernel-owned storage.\n");
        KernelIoWriteString("[KERNEL] Loader BootInfo pointer: ");
        KernelIoWriteHex64(KernelBootInfoSourceAddress());
        KernelIoWriteString("\n");
        KernelIoWriteString("[KERNEL] Kernel BootInfo copy: ");
        KernelIoWriteHex64((unsigned long long)kernelBootInfo);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel could not adopt OrynBootInfo.\n");
    }
}

void OrynKernelDiagnosticsPrintEntryProofs(const OrynBootInfo* kernelBootInfo)
{
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 entered.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel entered successfully.\n");
    KernelIoWriteString("[KERNEL] PASS: Serial/debug output path is working.\n");
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

void OrynKernelDiagnosticsRunBootProofs(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelDiagnosticsPrintEntryProofs(kernelBootInfo);
    OrynKernelBootInfoStatus bootStatus = KernelBootInfoValidate(kernelBootInfo);

    OrynKernelDiagnosticsRunDescriptorProofs();
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleDescriptorsReady);

    OrynKernelDiagnosticsRunInterruptTimerProofs(kernelBootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleInterruptsReady);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleTimersReady);

    OrynKernelDiagnosticsRunPciProof(kernelBootInfo);
    if (bootStatus.IsValid)
    {
        OrynKernelDiagnosticsRunConsoleProofs(kernelBootInfo);
        KernelBootInfoPrintSummary(kernelBootInfo);
        OrynKernelDiagnosticsRunFat32VfsProof();
        OrynKernelDiagnosticsRunMemoryProofs(kernelBootInfo);
    }
    else
    {
        OrynKernelDiagnosticsRunBootInfoFailureProof();
    }
}
