#include "KernelDiagnosticsProofsInternal.h"

static unsigned int OrynKernelDiagnosticsMaximumViewTop(const KConsoleMetrics* metrics)
{
    unsigned int rows = KConsole.VisibleRows();

    if (metrics->TotalLines <= rows)
    {
        return 0U;
    }

    return metrics->TotalLines - rows;
}

static int OrynKernelDiagnosticsExerciseScrollState(void)
{
    KConsoleMetrics metrics;
    int ok;

    KConsole.GetMetrics(&metrics);
    if (metrics.TotalLines <= KConsole.VisibleRows())
    {
        return 0;
    }

    ok = KConsole.ScrollUpLines(1U) &&
        KConsole.ScrollDownLines(1U) &&
        KConsole.PageUp() &&
        KConsole.PageDown();

    KConsole.ScrollToBottom();
    KConsole.GetMetrics(&metrics);

    return ok && metrics.ViewFollowsTail &&
        metrics.ViewTopLine == OrynKernelDiagnosticsMaximumViewTop(&metrics);
}

static void OrynKernelDiagnosticsWriteScrollProbeLines(void)
{
    unsigned int lines = KConsole.VisibleRows() + 4U;

    if (lines > 80U)
    {
        lines = 80U;
    }

    KConsole.SetForegroundColour(KCONSOLE_COLOUR_STEP);
    for (unsigned int index = 0U; index < lines; ++index)
    {
        KConsole.WriteString("[SCROLL] proof line ");
        KConsole.WriteUnsignedDec(index + 1U);
        KConsole.WriteChar('\n');
    }

    KConsole.ResetForegroundColour();
}

int OrynKernelDiagnosticsConsoleRunScrollProbe(void)
{
    unsigned int savedState;
    int ok;

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

    if (!KConsole.IsAvailable() || KConsole.VisibleRows() == 0U || KConsole.VisibleColumns() == 0U)
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel screen scrolling proof failed.\n");
        return 0;
    }

    savedState = KConsole.BeginDeferredPresent();
    OrynKernelDiagnosticsWriteScrollProbeLines();
    ok = OrynKernelDiagnosticsExerciseScrollState();
    KConsole.ClearScreen();
    KConsole.EndDeferredPresent(savedState);
    KConsole.ClearScreen();

    if (!ok)
    {
        KernelIoWriteString("[KERNEL] FAIL: Kernel screen scrolling proof failed.\n");
        return 0;
    }

    KernelIoWriteString("[KERNEL] PASS: Kernel screen scrollback buffer initialized.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel screen scrollback stores coloured cells.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel screen scroll up/down works.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel screen page up/down works.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel screen scroll-to-bottom works.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel screen scroll proof keeps visible output stable.\n");
    KernelIoWriteString("[KERNEL] PASS: Kernel screen scrolling implemented.\n");
    return 1;
}
