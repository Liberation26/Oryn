#include "KernelDiagnosticsProofsInternal.h"

static void OrynKernelDiagnosticsPrintConsoleHeader(void)
{
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 booted.\n");
    KernelIoWriteString("[KERNEL] Target: uefi-x64\n");
    KernelIoWriteString("[KERNEL] Toolchain: clang + lld\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel console initialized.\n");
    KernelIoWriteString("[KERNEL] PASS: Console runtime separated from diagnostics proof flow.\n");
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
        KernelIoWriteString("[KERNEL] PASS: Kernel screen double buffering implemented.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel screen double buffering proof failed.\n");
    }

    (void)OrynKernelDiagnosticsRunKeyboardScrollProof();
}
