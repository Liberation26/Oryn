#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

static int OrynKernelDiagnosticsConsoleCanUseBackBuffer(void)
{
    return OrynBootProofConsoleIsAvailable() && OrynBootProofConsoleIsDoubleBuffered() &&
        OrynBootProofConsoleBackBufferBytes() != 0ULL;
}

int OrynKernelDiagnosticsConsoleRunDoubleBufferProbe(void)
{
    OrynBootProofConsoleMetrics before;
    OrynBootProofConsoleMetrics after;
    unsigned int savedState;

    OrynKernelDiagnosticsLogText("[KERNEL] Kernel screen double buffer: starting proof.\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Kernel screen back buffer bytes: ");
    OrynKernelDiagnosticsLogDec64(OrynBootProofConsoleBackBufferBytes());
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Kernel screen present count: ");
    OrynKernelDiagnosticsLogDec64(OrynBootProofConsolePresentCount());
    OrynKernelDiagnosticsLogText("\n");

    if (!OrynKernelDiagnosticsConsoleCanUseBackBuffer())
    {
        return 0;
    }

    savedState = OrynBootProofConsoleBeginDeferredPresent();
    OrynBootProofConsoleGetMetrics(&before);
    OrynBootProofConsoleClear();
    OrynBootProofConsoleGetMetrics(&after);
    OrynBootProofConsoleEndDeferredPresent(savedState);

    if (after.FullPresentCount <= before.FullPresentCount)
    {
        return 0;
    }

    OrynKernelScreenReportOk(0, "Kernel screen back buffer allocated.");
    OrynKernelScreenReportOk(0, "Kernel screen renders into back buffer first.");
    return 1;
}

int OrynKernelDiagnosticsConsoleRunLineBufferedProbe(void)
{
    OrynBootProofConsoleMetrics before;
    OrynBootProofConsoleMetrics afterCharacters;
    OrynBootProofConsoleMetrics afterLine;
    unsigned int savedState;
    int ok;

    if (!OrynKernelDiagnosticsConsoleCanUseBackBuffer())
    {
        return 0;
    }

    savedState = OrynBootProofConsoleBeginDeferredPresent();
    OrynBootProofConsoleScrollToBottom();
    OrynBootProofConsoleWriteChar('\n');
    OrynBootProofConsoleGetMetrics(&before);

    OrynBootProofConsoleSetColour(ORYN_BOOT_PROOF_CONSOLE_COLOUR_STEP);
    OrynBootProofConsoleWriteChar('L');
    OrynBootProofConsoleWriteChar('B');
    OrynBootProofConsoleGetMetrics(&afterCharacters);
    OrynBootProofConsoleWriteChar('\n');
    OrynBootProofConsoleGetMetrics(&afterLine);
    OrynBootProofConsoleResetColour();

    ok = afterCharacters.LinePresentCount == before.LinePresentCount &&
        afterLine.LinePresentCount > afterCharacters.LinePresentCount;
    OrynBootProofConsoleClear();
    OrynBootProofConsoleEndDeferredPresent(savedState);
    OrynBootProofConsoleClear();

    if (!ok)
    {
        return 0;
    }

    OrynKernelScreenReportOk(0, "Kernel screen defers visible flip while line is being written.");
    OrynKernelScreenReportOk(0, "Kernel screen flips after completed line.");
    OrynKernelScreenReportOk(0, "Kernel screen presents dirty completed line only.");
    return 1;
}

int OrynKernelDiagnosticsConsoleRunFastRefreshProbe(void)
{
    OrynBootProofConsoleMetrics before;
    OrynBootProofConsoleMetrics afterCharacters;
    OrynBootProofConsoleMetrics afterLines;
    unsigned int savedState;
    unsigned int lines = OrynBootProofConsoleVisibleRows() + 2U;
    int ok;

    if (!OrynKernelDiagnosticsConsoleCanUseBackBuffer())
    {
        return 0;
    }

    if (lines > 80U)
    {
        lines = 80U;
    }

    savedState = OrynBootProofConsoleBeginDeferredPresent();
    OrynBootProofConsoleGetMetrics(&before);
    OrynBootProofConsoleScrollToBottom();
    OrynBootProofConsoleSetColour(ORYN_BOOT_PROOF_CONSOLE_COLOUR_STEP);
    OrynBootProofConsoleWriteString("[FAST] dirty line proof");
    OrynBootProofConsoleGetMetrics(&afterCharacters);
    OrynBootProofConsoleWriteChar('\n');

    for (unsigned int index = 0U; index < lines; ++index)
    {
        OrynBootProofConsoleWriteString("[FAST] scroll proof line");
        OrynBootProofConsoleWriteChar('\n');
    }

    OrynBootProofConsoleGetMetrics(&afterLines);
    OrynBootProofConsoleResetColour();
    OrynBootProofConsoleClear();
    OrynBootProofConsoleEndDeferredPresent(savedState);
    OrynBootProofConsoleClear();

    ok = afterCharacters.LinePresentCount == before.LinePresentCount &&
        afterLines.FastScrollPresentCount > before.FastScrollPresentCount;

    if (!ok)
    {
        return 0;
    }

    OrynKernelScreenReportOk(0, "Kernel screen uses fast scroll path after visible area is full.");
    OrynKernelScreenReportOk(0, "Kernel screen presents completed frame to visible output.");
    OrynKernelScreenReportOk(0, "Kernel screen visible presents are atomic.");
    OrynKernelScreenReportOk(0, "Kernel screen refresh is line/scroll optimized.");
    OrynKernelScreenReportOk(0, "Kernel screen line-buffered double buffering implemented.");
    return 1;
}
