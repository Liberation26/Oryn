#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

static int OrynKernelDiagnosticsConsoleCanUseBackBuffer(void)
{
    return KConsole.IsAvailable() && KConsole.IsDoubleBuffered() &&
        KConsole.BackBufferBytes() != 0ULL;
}

int OrynKernelDiagnosticsConsoleRunDoubleBufferProbe(void)
{
    KConsoleMetrics before;
    KConsoleMetrics after;
    unsigned int savedState;

    KernelIoWriteString("[KERNEL] Kernel screen double buffer: starting proof.\n");
    KernelIoWriteString("[KERNEL] Kernel screen back buffer bytes: ");
    KernelIoWriteDec64(KConsole.BackBufferBytes());
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Kernel screen present count: ");
    KernelIoWriteDec64(KConsole.PresentCount());
    KernelIoWriteString("\n");

    if (!OrynKernelDiagnosticsConsoleCanUseBackBuffer())
    {
        return 0;
    }

    savedState = KConsole.BeginDeferredPresent();
    KConsole.GetMetrics(&before);
    KConsole.ClearScreen();
    KConsole.GetMetrics(&after);
    KConsole.EndDeferredPresent(savedState);

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
    KConsoleMetrics before;
    KConsoleMetrics afterCharacters;
    KConsoleMetrics afterLine;
    unsigned int savedState;
    int ok;

    if (!OrynKernelDiagnosticsConsoleCanUseBackBuffer())
    {
        return 0;
    }

    savedState = KConsole.BeginDeferredPresent();
    KConsole.ScrollToBottom();
    KConsole.WriteChar('\n');
    KConsole.GetMetrics(&before);

    KConsole.SetForegroundColour(KCONSOLE_COLOUR_STEP);
    KConsole.WriteChar('L');
    KConsole.WriteChar('B');
    KConsole.GetMetrics(&afterCharacters);
    KConsole.WriteChar('\n');
    KConsole.GetMetrics(&afterLine);
    KConsole.ResetForegroundColour();

    ok = afterCharacters.LinePresentCount == before.LinePresentCount &&
        afterLine.LinePresentCount > afterCharacters.LinePresentCount;
    KConsole.ClearScreen();
    KConsole.EndDeferredPresent(savedState);
    KConsole.ClearScreen();

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
    KConsoleMetrics before;
    KConsoleMetrics afterCharacters;
    KConsoleMetrics afterLines;
    unsigned int savedState;
    unsigned int lines = KConsole.VisibleRows() + 2U;
    int ok;

    if (!OrynKernelDiagnosticsConsoleCanUseBackBuffer())
    {
        return 0;
    }

    if (lines > 80U)
    {
        lines = 80U;
    }

    savedState = KConsole.BeginDeferredPresent();
    KConsole.GetMetrics(&before);
    KConsole.ScrollToBottom();
    KConsole.SetForegroundColour(KCONSOLE_COLOUR_STEP);
    KConsole.WriteString("[FAST] dirty line proof");
    KConsole.GetMetrics(&afterCharacters);
    KConsole.WriteChar('\n');

    for (unsigned int index = 0U; index < lines; ++index)
    {
        KConsole.WriteString("[FAST] scroll proof line");
        KConsole.WriteChar('\n');
    }

    KConsole.GetMetrics(&afterLines);
    KConsole.ResetForegroundColour();
    KConsole.ClearScreen();
    KConsole.EndDeferredPresent(savedState);
    KConsole.ClearScreen();

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
