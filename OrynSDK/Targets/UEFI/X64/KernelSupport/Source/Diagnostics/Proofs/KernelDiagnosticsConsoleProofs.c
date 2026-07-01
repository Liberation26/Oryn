#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

static void OrynKernelDiagnosticsPrintConsoleHeader(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Oryn Kernel-5 booted.\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Target: uefi-x64\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Toolchain: clang + lld\n");
    OrynKernelScreenReportOk(0, "Kernel console initialized.");
    OrynKernelScreenReportOk(0, "Console runtime separated from boot proof flow by BootProofConsole adapter.");
    OrynKernelDiagnosticsLogText(OrynBootProofConsoleIsTtfActive() ?
        "[KERNEL] TTF renderer: active\n" :
        "[KERNEL] TTF renderer: fallback bitmap glyphs\n");
}

void OrynKernelDiagnosticsRunConsoleProofs(const OrynBootInfo* kernelBootInfo)
{
    OrynBootProofConsoleInitialize(kernelBootInfo);
    OrynBootProofConsoleMarkRuntimeReady();
    OrynBootProofConsoleClear();
    OrynKernelDiagnosticsPrintConsoleHeader();

    (void)OrynKernelDiagnosticsConsoleRunScrollProbe();

    if (OrynBootProofConsoleIsDoubleBuffered() &&
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
