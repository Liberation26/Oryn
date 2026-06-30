#include "KernelDiagnosticsProofsInternal.h"

static int OrynKernelDiagnosticsRunKeyboardScrollProof(void)
{
    if (OrynKernelKeyboardInitForConsoleScroll())
    {
        OrynKernelKeyboardPrintProof();
        return 1;
    }

    OrynKernelKeyboardPrintProof();
    return 0;
}

static void OrynKernelDiagnosticsRunScreenScrollProof(void)
{
    KernelIoWriteString("[KERNEL] Kernel screen scrolling: starting proof.\n");
    KernelIoWriteString("[KERNEL] Kernel screen visible rows: ");
    KernelIoWriteDec64(KConsole.VisibleRows());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Kernel screen visible columns: ");
    KernelIoWriteDec64(KConsole.VisibleColumns());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Kernel screen scrollback rows: ");
    KernelIoWriteDec64(KConsole.ScrollbackRows());
    KernelIoWriteString("\n");

    if (KConsoleRunScrollProof())
    {
        KernelIoWriteString("[KERNEL] PASS: Kernel screen scrollback buffer initialized.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen scrollback stores coloured cells.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen scroll up/down works.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen page up/down works.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen scroll-to-bottom works.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen proof keeps visible output stable.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen scrolling implemented.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel screen scrolling proof failed.\n");
    }
}

static void OrynKernelDiagnosticsRunScreenDoubleBufferProof(void)
{
    KernelIoWriteString("[KERNEL] Kernel screen double buffer: starting proof.\n");
    KernelIoWriteString("[KERNEL] Kernel screen back buffer bytes: ");
    KernelIoWriteDec64(KConsole.BackBufferBytes());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Kernel screen present count: ");
    KernelIoWriteDec64(KConsole.PresentCount());
    KernelIoWriteString("\n");

    if (KConsole.IsDoubleBuffered() && KConsoleRunDoubleBufferProof() &&
        KConsoleRunLineBufferedFlipProof() && KConsoleRunFastRefreshProof())
    {
        KernelIoWriteString("[KERNEL] PASS: Kernel screen back buffer allocated.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen renders into back buffer first.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen defers visible flip while line is being written.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen flips after completed line.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen presents dirty completed line only.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen uses fast scroll path after visible area is full.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen presents completed frame to visible output.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen visible presents are atomic.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen refresh is line/scroll optimized.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen line-buffered double buffering implemented.\n");
        KernelIoWriteString("[KERNEL] PASS: Kernel screen double buffering implemented.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel screen double buffering proof failed.\n");
    }
}

void OrynKernelDiagnosticsRunConsoleProofs(const OrynBootInfo* kernelBootInfo)
{
    KConsoleInit(kernelBootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleConsoleReady);
    KConsole.ClearScreen();
    KernelIoWriteString("[KERNEL] Oryn Kernel-5 booted.\n");
    KernelIoWriteString("[KERNEL] Target: uefi-x64\n");
    KernelIoWriteString("[KERNEL] Toolchain: clang + lld\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel console initialized.\n");
    KernelIoWriteString(KConsole.IsTtfActive() ?
        "[KERNEL] TTF renderer: active\n" :
        "[KERNEL] TTF renderer: fallback bitmap glyphs\n");
    OrynKernelDiagnosticsRunScreenScrollProof();
    OrynKernelDiagnosticsRunScreenDoubleBufferProof();
    (void)OrynKernelDiagnosticsRunKeyboardScrollProof();
}
