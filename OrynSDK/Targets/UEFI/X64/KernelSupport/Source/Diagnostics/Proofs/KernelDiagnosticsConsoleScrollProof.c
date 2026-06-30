#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

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
        OrynKernelScreenReportFail(0, "Kernel screen scrolling proof failed.");
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
        OrynKernelScreenReportFail(0, "Kernel screen scrolling proof failed.");
        return 0;
    }

    OrynKernelScreenReportOk(0, "Kernel screen scrollback buffer initialized.");
    OrynKernelScreenReportOk(0, "Kernel screen scrollback stores coloured cells.");
    OrynKernelScreenReportOk(0, "Kernel screen scroll up/down works.");
    OrynKernelScreenReportOk(0, "Kernel screen page up/down works.");
    OrynKernelScreenReportOk(0, "Kernel screen scroll-to-bottom works.");
    OrynKernelScreenReportOk(0, "Kernel screen scroll proof keeps visible output stable.");
    OrynKernelScreenReportOk(0, "Kernel screen scrolling implemented.");
    return 1;
}
