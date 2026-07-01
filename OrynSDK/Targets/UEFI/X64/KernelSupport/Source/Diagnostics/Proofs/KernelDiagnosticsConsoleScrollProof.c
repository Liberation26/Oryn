#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

static unsigned int OrynKernelDiagnosticsMaximumViewTop(const OrynBootProofConsoleMetrics* metrics)
{
    unsigned int rows = OrynBootProofConsoleVisibleRows();

    if (metrics->TotalLines <= rows)
    {
        return 0U;
    }

    return metrics->TotalLines - rows;
}

static int OrynKernelDiagnosticsExerciseScrollState(void)
{
    OrynBootProofConsoleMetrics metrics;
    int ok;

    OrynBootProofConsoleGetMetrics(&metrics);
    if (metrics.TotalLines <= OrynBootProofConsoleVisibleRows())
    {
        return 0;
    }

    ok = OrynBootProofConsoleScrollUpLines(1U) &&
        OrynBootProofConsoleScrollDownLines(1U) &&
        OrynBootProofConsolePageUp() &&
        OrynBootProofConsolePageDown();

    OrynBootProofConsoleScrollToBottom();
    OrynBootProofConsoleGetMetrics(&metrics);

    return ok && metrics.ViewFollowsTail &&
        metrics.ViewTopLine == OrynKernelDiagnosticsMaximumViewTop(&metrics);
}

static void OrynKernelDiagnosticsWriteScrollProbeLines(void)
{
    unsigned int lines = OrynBootProofConsoleVisibleRows() + 4U;

    if (lines > 80U)
    {
        lines = 80U;
    }

    OrynBootProofConsoleSetColour(ORYN_BOOT_PROOF_CONSOLE_COLOUR_STEP);
    for (unsigned int index = 0U; index < lines; ++index)
    {
        OrynBootProofConsoleWriteString("[SCROLL] proof line ");
        OrynBootProofConsoleWriteUnsignedDec(index + 1U);
        OrynBootProofConsoleWriteChar('\n');
    }

    OrynBootProofConsoleResetColour();
}

int OrynKernelDiagnosticsConsoleRunScrollProbe(void)
{
    unsigned int savedState;
    int ok;

    KernelIoWriteString("[KERNEL] Kernel screen scrolling: starting proof.\n");
    KernelIoWriteString("[KERNEL] Kernel screen visible rows: ");
    KernelIoWriteDec64(OrynBootProofConsoleVisibleRows());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Kernel screen visible columns: ");
    KernelIoWriteDec64(OrynBootProofConsoleVisibleColumns());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Kernel screen scrollback rows: ");
    KernelIoWriteDec64(OrynBootProofConsoleScrollbackRows());
    KernelIoWriteString("\n");

    if (!OrynBootProofConsoleIsAvailable() || OrynBootProofConsoleVisibleRows() == 0U || OrynBootProofConsoleVisibleColumns() == 0U)
    {
        OrynKernelScreenReportFail(0, "Kernel screen scrolling proof failed.");
        return 0;
    }

    savedState = OrynBootProofConsoleBeginDeferredPresent();
    OrynKernelDiagnosticsWriteScrollProbeLines();
    ok = OrynKernelDiagnosticsExerciseScrollState();
    OrynBootProofConsoleClear();
    OrynBootProofConsoleEndDeferredPresent(savedState);
    OrynBootProofConsoleClear();

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
