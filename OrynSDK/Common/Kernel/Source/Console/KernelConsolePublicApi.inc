unsigned int KConsolePresentCount(void)
{
    return gConsole.PresentCount;
}

static unsigned int KConsoleBeginSilentProof(void)
{
    unsigned int savedSuppress = gConsole.PresentSuppressed;
    gConsole.PresentSuppressed = 1U;
    return savedSuppress;
}

static void KConsoleEndSilentProof(unsigned int savedSuppress)
{
    gConsole.PresentSuppressed = savedSuppress;
}

int KConsoleRunDoubleBufferProof(void)
{
    unsigned int savedSuppress;
    unsigned int beforeFull;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    savedSuppress = KConsoleBeginSilentProof();
    beforeFull = gConsole.FullPresentCount;
    KConsoleRenderVisible();
    KConsoleEndSilentProof(savedSuppress);
    return gConsole.FullPresentCount > beforeFull;
}

int KConsoleRunLineBufferedFlipProof(void)
{
    unsigned int savedSuppress;
    unsigned int beforeLine;
    unsigned int afterCharacters;
    unsigned int afterLine;
    unsigned int originalColour = gConsole.ForegroundColour;
    unsigned char originalVga = gConsole.VgaAttribute;
    int ok;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    savedSuppress = KConsoleBeginSilentProof();
    KConsoleScrollToBottom();
    KConsoleWriteChar('\n');
    beforeLine = gConsole.LinePresentCount;

    KConsoleSetForegroundColour(KCONSOLE_COLOUR_STEP);
    KConsoleWriteChar('L');
    KConsoleWriteChar('B');
    afterCharacters = gConsole.LinePresentCount;
    KConsoleWriteChar('\n');
    afterLine = gConsole.LinePresentCount;

    gConsole.ForegroundColour = originalColour;
    gConsole.VgaAttribute = originalVga;
    ok = afterCharacters == beforeLine && afterLine > afterCharacters;
    KConsoleClearScreen();
    KConsoleEndSilentProof(savedSuppress);
    return ok;
}

int KConsoleRunFastRefreshProof(void)
{
    unsigned int savedSuppress;
    unsigned int beforeFast;
    unsigned int beforeLine;
    unsigned int afterCharacters;
    unsigned int lines;
    unsigned int originalColour = gConsole.ForegroundColour;
    unsigned char originalVga = gConsole.VgaAttribute;
    int ok;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    savedSuppress = KConsoleBeginSilentProof();
    beforeFast = gConsole.FastScrollPresentCount;
    KConsoleScrollToBottom();
    beforeLine = gConsole.LinePresentCount;

    KConsoleSetForegroundColour(KCONSOLE_COLOUR_STEP);
    KConsoleWriteProofText("[FAST] dirty line proof");
    afterCharacters = gConsole.LinePresentCount;
    KConsoleWriteChar('\n');

    lines = gConsole.VisibleRows + 2U;
    if (lines > 80U)
    {
        lines = 80U;
    }

    for (unsigned int index = 0U; index < lines; ++index)
    {
        KConsoleWriteProofText("[FAST] scroll proof line");
        KConsoleWriteChar('\n');
    }

    gConsole.ForegroundColour = originalColour;
    gConsole.VgaAttribute = originalVga;
    ok = afterCharacters == beforeLine && gConsole.FastScrollPresentCount > beforeFast;
    KConsoleClearScreen();
    KConsoleEndSilentProof(savedSuppress);
    return ok;
}

int KConsoleRunScrollProof(void)
{
    unsigned int savedSuppress;
    unsigned int originalColour;
    unsigned char originalVga;
    unsigned int proofLines;
    int ok;

    if (!gConsole.Available || gConsole.VisibleRows == 0U || gConsole.VisibleColumns == 0U)
    {
        return 0;
    }

    savedSuppress = KConsoleBeginSilentProof();
    originalColour = gConsole.ForegroundColour;
    originalVga = gConsole.VgaAttribute;
    proofLines = gConsole.VisibleRows + 4U;

    if (proofLines > 80U)
    {
        proofLines = 80U;
    }

    KConsoleSetForegroundColour(KCONSOLE_COLOUR_STEP);
    for (unsigned int index = 0U; index < proofLines; ++index)
    {
        KConsoleWriteProofText("[SCROLL] proof line ");
        KConsoleWriteProofDecimal(index + 1U);
        KConsoleWriteChar('\n');
    }

    if (gConsole.TotalLines > gConsole.VisibleRows)
    {
        ok = KConsoleScrollUpLines(1U) &&
            KConsoleScrollDownLines(1U) &&
            KConsolePageUp() &&
            KConsolePageDown();
        gConsole.StateOnlyScrollProofCount += ok ? 1U : 0U;
    }
    else
    {
        ok = 0;
    }

    KConsoleScrollToBottom();
    ok = ok && gConsole.ViewFollowsTail && gConsole.ViewTopLine == KConsoleMaximumViewTop();

    gConsole.ForegroundColour = originalColour;
    gConsole.VgaAttribute = originalVga;
    KConsoleClearScreen();
    KConsoleEndSilentProof(savedSuppress);
    return ok;
}

static unsigned char KConsoleVgaAttributeForColour(unsigned int colour)
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
    KConsolePresentCount
};
