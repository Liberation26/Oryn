#include "KernelBootProofConsole.h"
#include "KernelConsole.h"
#include "KernelLifecycle.h"

static void OrynBootProofConsoleCopyMetrics(
    OrynBootProofConsoleMetrics* target,
    const KConsoleMetrics* source)
{
    target->PresentCount = source->PresentCount;
    target->LinePresentCount = source->LinePresentCount;
    target->FastScrollPresentCount = source->FastScrollPresentCount;
    target->FullPresentCount = source->FullPresentCount;
    target->AtomicPresentCount = source->AtomicPresentCount;
    target->TotalLines = source->TotalLines;
    target->ViewTopLine = source->ViewTopLine;
    target->ViewFollowsTail = source->ViewFollowsTail;
    target->CurrentLine = source->CurrentLine;
    target->CurrentColumn = source->CurrentColumn;
}

void OrynBootProofConsoleInitialize(const OrynBootInfo* bootInfo)
{
    KConsoleInit(bootInfo);
}

void OrynBootProofConsoleMarkRuntimeReady(void)
{
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleConsoleReady);
}

void OrynBootProofConsoleClear(void)
{
    KConsole.ClearScreen();
}

void OrynBootProofConsoleWriteChar(char value)
{
    KConsole.WriteChar(value);
}

void OrynBootProofConsoleWriteString(const char* text)
{
    KConsole.WriteString(text);
}

void OrynBootProofConsoleWriteUnsignedDec(unsigned int value)
{
    KConsole.WriteUnsignedDec(value);
}

void OrynBootProofConsoleSetColour(unsigned int colour)
{
    KConsole.SetForegroundColour(colour);
}

void OrynBootProofConsoleResetColour(void)
{
    KConsole.ResetForegroundColour();
}

int OrynBootProofConsoleIsAvailable(void)
{
    return KConsole.IsAvailable();
}

int OrynBootProofConsoleIsTtfActive(void)
{
    return KConsole.IsTtfActive();
}

int OrynBootProofConsoleIsDoubleBuffered(void)
{
    return KConsole.IsDoubleBuffered();
}

unsigned int OrynBootProofConsoleVisibleRows(void)
{
    return KConsole.VisibleRows();
}

unsigned int OrynBootProofConsoleVisibleColumns(void)
{
    return KConsole.VisibleColumns();
}

unsigned int OrynBootProofConsoleScrollbackRows(void)
{
    return KConsole.ScrollbackRows();
}

unsigned long long OrynBootProofConsoleBackBufferBytes(void)
{
    return KConsole.BackBufferBytes();
}

unsigned int OrynBootProofConsolePresentCount(void)
{
    return KConsole.PresentCount();
}

unsigned int OrynBootProofConsoleBeginDeferredPresent(void)
{
    return KConsole.BeginDeferredPresent();
}

void OrynBootProofConsoleEndDeferredPresent(unsigned int savedState)
{
    KConsole.EndDeferredPresent(savedState);
}

void OrynBootProofConsoleGetMetrics(OrynBootProofConsoleMetrics* metrics)
{
    KConsoleMetrics consoleMetrics;

    if (metrics == 0)
    {
        return;
    }

    KConsole.GetMetrics(&consoleMetrics);
    OrynBootProofConsoleCopyMetrics(metrics, &consoleMetrics);
}

void OrynBootProofConsoleScrollToBottom(void)
{
    KConsole.ScrollToBottom();
}

int OrynBootProofConsoleScrollUpLines(unsigned int lines)
{
    return KConsole.ScrollUpLines(lines);
}

int OrynBootProofConsoleScrollDownLines(unsigned int lines)
{
    return KConsole.ScrollDownLines(lines);
}

int OrynBootProofConsolePageUp(void)
{
    return KConsole.PageUp();
}

int OrynBootProofConsolePageDown(void)
{
    return KConsole.PageDown();
}
