#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

static void OrynKernelDiagnosticsPrintConsoleHeader(void)
{
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 booted.\n");
    KernelIoWriteString("[KERNEL] Target: uefi-x64\n");
    KernelIoWriteString("[KERNEL] Toolchain: clang + lld\n");
    OrynKernelScreenReportOk(0, "Kernel console initialized.");
    OrynKernelScreenReportOk(0, "Console runtime separated from diagnostics proof flow.");
    KernelIoWriteString(KConsole.IsTtfActive() ?
        "[KERNEL] TTF renderer: active\n" :
        "[KERNEL] TTF renderer: fallback bitmap glyphs\n");
}

void OrynKernelDiagnosticsRunConsoleProofs(const OrynBootInfo* kernelBootInfo)
{
    KConsoleInit(kernelBootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleConsoleReady);
    KConsole.ClearScreen();
    OrynKernelDiagnosticsPrintConsoleHeader();

    (void)OrynKernelDiagnosticsConsoleRunScrollProbe();

    if (KConsole.IsDoubleBuffered() &&
        OrynKernelDiagnosticsConsoleRunDoubleBufferProbe() &&
        OrynKernelDiagnosticsConsoleRunLineBufferedProbe() &&
        OrynKernelDiagnosticsConsoleRunFastRefreshProbe())
    {
        OrynKernelScreenReportOk(0, "Kernel screen double buffering implemented.");
    }
    else
    {
        OrynKernelScreenReportFail(0, "Kernel screen double buffering proof failed.");
    }

    (void)OrynKernelDiagnosticsRunKeyboardScrollProof();
}
