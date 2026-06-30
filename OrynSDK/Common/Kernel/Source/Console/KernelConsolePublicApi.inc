unsigned int KConsolePresentCount(void)
{
    return gConsole.PresentCount;
}

int KConsoleRunDoubleBufferProof(void)
{
    unsigned int before;
    unsigned int beforeAtomic;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    before = gConsole.PresentCount;
    beforeAtomic = gConsole.AtomicPresentCount;
    KConsoleRenderVisible();
    return gConsole.PresentCount > before && gConsole.AtomicPresentCount > beforeAtomic;
}

int KConsoleRunLineBufferedFlipProof(void)
{
    unsigned int before;
    unsigned int afterCharacters;
    unsigned int afterLine;
    unsigned int originalColour = gConsole.ForegroundColour;
    unsigned char originalVga = gConsole.VgaAttribute;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    KConsoleScrollToBottom();
    KConsoleWriteChar('\n');
    before = gConsole.PresentCount;

    KConsoleSetForegroundColour(KCONSOLE_COLOUR_STEP);
    KConsoleWriteChar('L');
    KConsoleWriteChar('B');
    afterCharacters = gConsole.PresentCount;
    KConsoleWriteChar('\n');
    afterLine = gConsole.PresentCount;

    gConsole.ForegroundColour = originalColour;
    gConsole.VgaAttribute = originalVga;

    return afterCharacters == before && afterLine > afterCharacters;
}

int KConsoleRunFastRefreshProof(void)
{
    unsigned int beforeFast;
    unsigned int beforePresent;
    unsigned int afterCharacters;
    unsigned int lines;
    unsigned int originalColour = gConsole.ForegroundColour;
    unsigned char originalVga = gConsole.VgaAttribute;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    beforeFast = gConsole.FastScrollPresentCount;
    KConsoleScrollToBottom();
    beforePresent = gConsole.PresentCount;

    KConsoleSetForegroundColour(KCONSOLE_COLOUR_STEP);
    KConsoleWriteProofText("[FAST] dirty line proof");
    afterCharacters = gConsole.PresentCount;
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

    return afterCharacters == beforePresent && gConsole.FastScrollPresentCount > beforeFast;
}

int KConsoleRunScrollProof(void)
{
    if (!gConsole.Available || gConsole.VisibleRows == 0U || gConsole.VisibleColumns == 0U)
    {
        return 0;
    }

    unsigned int originalColour = gConsole.ForegroundColour;
    unsigned char originalVga = gConsole.VgaAttribute;
    unsigned int proofLines = gConsole.VisibleRows + 4U;
    int ok;

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
        unsigned int savedSuppress = gConsole.PresentSuppressed;
        gConsole.PresentSuppressed = 1U;

        ok = KConsoleScrollUpLines(1U) &&
            KConsoleScrollDownLines(1U) &&
            KConsolePageUp() &&
            KConsolePageDown();

        gConsole.PresentSuppressed = savedSuppress;
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
