#include "KernelConsoleInternal.h"

unsigned int KConsolePresentCount(void)
{
    return gConsole.PresentCount;
}

unsigned int KConsoleBeginDeferredPresent(void)
{
    unsigned int savedState = gConsole.PresentSuppressed;
    gConsole.PresentSuppressed = 1U;
    return savedState;
}

void KConsoleEndDeferredPresent(unsigned int saved_state)
{
    gConsole.PresentSuppressed = saved_state;
}

void KConsoleGetMetrics(KConsoleMetrics* metrics)
{
    if (metrics == 0)
    {
        return;
    }

    metrics->PresentCount = gConsole.PresentCount;
    metrics->LinePresentCount = gConsole.LinePresentCount;
    metrics->FastScrollPresentCount = gConsole.FastScrollPresentCount;
    metrics->FullPresentCount = gConsole.FullPresentCount;
    metrics->AtomicPresentCount = gConsole.AtomicPresentCount;
    metrics->TotalLines = gConsole.TotalLines;
    metrics->ViewTopLine = gConsole.ViewTopLine;
    metrics->ViewFollowsTail = (unsigned int)(gConsole.ViewFollowsTail ? 1 : 0);
    metrics->CurrentLine = gConsole.CurrentLine;
    metrics->CurrentColumn = gConsole.CurrentColumn;
}

unsigned char KConsoleVgaAttributeForColour(unsigned int colour)
{
    if (colour == KCONSOLE_COLOUR_PASS || colour == KCONSOLE_COLOUR_OK)
    {
        return KCONSOLE_VGA_ATTRIBUTE_PASS;
    }

    if (colour == KCONSOLE_COLOUR_WARN)
    {
        return KCONSOLE_VGA_ATTRIBUTE_WARN;
    }

    if (colour == KCONSOLE_COLOUR_FAIL)
    {
        return KCONSOLE_VGA_ATTRIBUTE_FAIL;
    }

    if (colour == KCONSOLE_COLOUR_STEP)
    {
        return KCONSOLE_VGA_ATTRIBUTE_STEP;
    }

    if (colour == KCONSOLE_COLOUR_PCI || colour == KCONSOLE_COLOUR_INFO)
    {
        return KCONSOLE_VGA_ATTRIBUTE_INFO;
    }

    return KCONSOLE_VGA_ATTRIBUTE_DEFAULT;
}

void KConsoleSetForegroundColour(unsigned int colour)
{
    gConsole.ForegroundColour = colour;
    gConsole.VgaAttribute = KConsoleVgaAttributeForColour(colour);
}

void KConsoleResetForegroundColour(void)
{
    KConsoleSetForegroundColour(KCONSOLE_COLOUR_DEFAULT);
}

int KConsoleIsAvailable(void)
{
    return gConsole.Available;
}

int KConsoleIsTtfActive(void)
{
    return gConsole.Available && gConsole.Mode == KCONSOLE_MODE_FRAMEBUFFER && gConsole.TtfReady;
}

const KConsoleApi KConsole =
{
    KConsoleClearScreen,
    KConsoleWriteChar,
    KConsoleWriteString,
    KConsoleWriteUnsignedDec,
    KConsoleSetForegroundColour,
    KConsoleResetForegroundColour,
    KConsoleIsAvailable,
    KConsoleIsTtfActive,
    KConsoleScrollUpLines,
    KConsoleScrollDownLines,
    KConsolePageUp,
    KConsolePageDown,
    KConsoleScrollToBottom,
    KConsoleVisibleRows,
    KConsoleVisibleColumns,
    KConsoleScrollbackRows,
    KConsoleIsDoubleBuffered,
    KConsoleBackBufferBytes,
    KConsolePresentCount,
    KConsoleGetMetrics,
    KConsoleBeginDeferredPresent,
    KConsoleEndDeferredPresent
};
