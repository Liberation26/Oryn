static void KConsoleVgaMoveShadowUpOneLine(void)
{
    if (gConsole.Mode != KCONSOLE_MODE_VGA_TEXT || gConsole.VisibleRows <= 1U)
    {
        return;
    }

    for (unsigned int row = 1U; row < gConsole.VisibleRows && row < KCONSOLE_VGA_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            gVgaShadowBuffer[((row - 1U) * KCONSOLE_VGA_WIDTH) + col] =
                gVgaShadowBuffer[(row * KCONSOLE_VGA_WIDTH) + col];
        }
    }

    unsigned int bottomRow = gConsole.VisibleRows - 1U;
    if (bottomRow < KCONSOLE_VGA_HEIGHT)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            gVgaShadowBuffer[(bottomRow * KCONSOLE_VGA_WIDTH) + col] =
                (unsigned short)(((unsigned short)KCONSOLE_VGA_ATTRIBUTE_DEFAULT << 8) | (unsigned char)' ');
        }
    }
}

static void KConsoleFastTailScrollPresent(void)
{
    if (!gConsole.Available || gConsole.VisibleRows == 0U)
    {
        return;
    }

    unsigned int bottomRow = gConsole.VisibleRows - 1U;

    if (gConsole.Mode == KCONSOLE_MODE_FRAMEBUFFER)
    {
        KConsoleFramebufferMoveBackBufferUpOneLine();
    }
    else if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsoleVgaMoveShadowUpOneLine();
    }

    for (unsigned int col = 0U; col < gConsole.VisibleColumns; ++col)
    {
        KConsoleRenderCell(bottomRow, col);
    }

    KConsoleDrawScrollbar();

    if (gConsole.Mode == KCONSOLE_MODE_FRAMEBUFFER)
    {
        unsigned int textTop = KCONSOLE_MARGIN_Y;
        unsigned int textHeight = gConsole.VisibleRows * gConsole.CellHeight;
        if (textTop + textHeight > gConsole.Height)
        {
            textHeight = gConsole.Height - textTop;
        }
        KConsolePresentFramebufferRect(0U, textTop, gConsole.Width, textHeight);
    }
    else if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsolePresentVga();
    }

    gConsole.FastScrollPresentCount += 1U;
    gConsole.LinePresentCount += 1U;
    KConsoleResetDirtyLine();
}

static void KConsoleRenderVisible(void)
{
    if (!gConsole.Available)
    {
        return;
    }

    if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsoleVgaClearPixels();
    }
    else
    {
        KConsoleFramebufferClearPixels();
    }

    for (unsigned int row = 0U; row < gConsole.VisibleRows; ++row)
    {
        for (unsigned int col = 0U; col < gConsole.VisibleColumns; ++col)
        {
            KConsoleRenderCell(row, col);
        }
    }

    KConsoleDrawScrollbar();
    KConsolePresent();
    gConsole.FullPresentCount += 1U;
    KConsoleResetDirtyLine();
}

static int KConsoleCurrentLineIsVisible(void)
{
    return gConsole.CurrentLine >= gConsole.ViewTopLine &&
        gConsole.CurrentLine < (gConsole.ViewTopLine + gConsole.VisibleRows);
}

static void KConsoleShiftScrollbackUp(void)
{
    for (unsigned int row = 1U; row < KCONSOLE_SCROLLBACK_ROWS; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_SCROLLBACK_COLS; ++col)
        {
            gConsole.Cells[row - 1U][col] = gConsole.Cells[row][col];
        }
    }

    KConsoleClearLogicalLine(KCONSOLE_SCROLLBACK_ROWS - 1U);
    if (!gConsole.ViewFollowsTail && gConsole.ViewTopLine > 0U)
    {
        gConsole.ViewTopLine -= 1U;
    }
}

static void KConsoleAppendNewLine(void)
{
    unsigned int oldViewTop = gConsole.ViewTopLine;
    unsigned int oldTotalLines = gConsole.TotalLines;
    int shiftedScrollback = 0;

    gConsole.CurrentColumn = 0U;

    if ((gConsole.CurrentLine + 1U) < KCONSOLE_SCROLLBACK_ROWS)
    {
        gConsole.CurrentLine += 1U;
        if (gConsole.CurrentLine >= gConsole.TotalLines)
        {
            gConsole.TotalLines = gConsole.CurrentLine + 1U;
        }
    }
    else
    {
        KConsoleShiftScrollbackUp();
        gConsole.TotalLines = KCONSOLE_SCROLLBACK_ROWS;
        shiftedScrollback = 1;
    }

    KConsoleClearLogicalLine(gConsole.CurrentLine);
    if (gConsole.ViewFollowsTail)
    {
        unsigned int newViewTop = KConsoleMaximumViewTop();
        int viewShiftedByOne = (newViewTop == (oldViewTop + 1U));
        int viewChanged = newViewTop != oldViewTop;
        gConsole.ViewTopLine = newViewTop;

        if (shiftedScrollback || (viewShiftedByOne && oldTotalLines >= gConsole.VisibleRows))
        {
            KConsoleFastTailScrollPresent();
        }
        else if (viewChanged)
        {
            KConsoleRenderVisible();
            gConsole.LinePresentCount += 1U;
        }
        else
        {
            KConsolePresentDirtyLine();
        }
    }
}

static void KConsoleStorePrintable(char value)
{
    if (gConsole.CurrentColumn >= gConsole.VisibleColumns)
    {
        KConsoleAppendNewLine();
    }

    if (gConsole.CurrentLine >= KCONSOLE_SCROLLBACK_ROWS ||
        gConsole.CurrentColumn >= KCONSOLE_SCROLLBACK_COLS)
    {
        return;
    }

    gConsole.Cells[gConsole.CurrentLine][gConsole.CurrentColumn].Value = value;
    gConsole.Cells[gConsole.CurrentLine][gConsole.CurrentColumn].Colour = gConsole.ForegroundColour;
    gConsole.Cells[gConsole.CurrentLine][gConsole.CurrentColumn].VgaAttribute = gConsole.VgaAttribute;

    if (KConsoleCurrentLineIsVisible())
    {
        unsigned int screenRow = gConsole.CurrentLine - gConsole.ViewTopLine;
        KConsoleRenderCell(screenRow, gConsole.CurrentColumn);
        KConsoleDrawScrollbar();
        KConsoleMarkDirtyCell(screenRow, gConsole.CurrentColumn);
    }

    gConsole.CurrentColumn += 1U;
}

static void KConsoleWriteProofDecimal(unsigned int value)
{
    char buffer[16];
    unsigned int index = 0U;

    if (value == 0U)
    {
        KConsoleWriteChar('0');
        return;
    }

    while (value != 0U && index < sizeof(buffer))
    {
        buffer[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U)
    {
        KConsoleWriteChar(buffer[--index]);
    }
}

static void KConsoleWriteProofText(const char* text)
{
    while (*text != 0)
    {
        KConsoleWriteChar(*text);
        ++text;
    }
}

void KConsoleInit(const OrynBootInfo* bootInfo)
{
    KConsoleUseVgaTextFallback();

    if (KConsoleBootInfoHasUsableFramebuffer(bootInfo))
    {
        gConsole.Framebuffer = (volatile unsigned int*)bootInfo->Framebuffer.Base;
        gConsole.FramebufferSize = bootInfo->Framebuffer.Size;
        gConsole.Width = bootInfo->Framebuffer.Width;
        gConsole.Height = KConsoleVisibleHeight(bootInfo);
        gConsole.Pitch = bootInfo->Framebuffer.PixelsPerScanLine;
        gConsole.FramebufferBackBuffer = gFramebufferBackBuffer;
        gConsole.FramebufferBackBufferPixels = (unsigned long long)gConsole.Width * (unsigned long long)gConsole.Height;
        gConsole.FramebufferBackBufferSize = gConsole.FramebufferBackBufferPixels * 4ULL;
        gConsole.DoubleBuffered = gConsole.FramebufferBackBufferPixels <= (unsigned long long)KCONSOLE_BACKBUFFER_PIXELS ? 1 : 0;
        gConsole.PresentCount = 0U;
        gConsole.LinePresentCount = 0U;
        gConsole.FastScrollPresentCount = 0U;
        gConsole.FullPresentCount = 0U;
        gConsole.DirtyLineActive = 0U;
        gConsole.DirtyScreenRow = 0U;
        gConsole.DirtyMinColumn = 0U;
        gConsole.DirtyMaxColumn = 0U;
        gConsole.CursorX = KCONSOLE_MARGIN_X;
        gConsole.CursorY = KCONSOLE_MARGIN_Y;
        gConsole.Mode = KCONSOLE_MODE_FRAMEBUFFER;
        gConsole.ForegroundColour = KCONSOLE_COLOUR_DEFAULT;
        gConsole.VgaAttribute = KCONSOLE_VGA_ATTRIBUTE_DEFAULT;
        gConsole.TtfReady = OrynTtfLoadFromBootInfo(bootInfo, &gConsole.Font);
        gConsole.Available = (gConsole.Height >= (KCONSOLE_MARGIN_Y + KConsoleActiveCellHeight() + KCONSOLE_MARGIN_Y) &&
            gConsole.DoubleBuffered) ? 1 : 0;

        if (!gConsole.Available)
        {
            KConsoleUseVgaTextFallback();
        }
    }

    KConsoleConfigureGeometry();
    KConsoleClearScrollback();
    KConsoleRenderVisible();
}

void KConsoleClearScreen(void)
{
    if (!gConsole.Available)
    {
        return;
    }

    KConsoleClearScrollback();
    KConsoleResetDirtyLine();
    gConsole.CursorX = gConsole.Mode == KCONSOLE_MODE_VGA_TEXT ? 0U : KCONSOLE_MARGIN_X;
    gConsole.CursorY = gConsole.Mode == KCONSOLE_MODE_VGA_TEXT ? 0U : KCONSOLE_MARGIN_Y;
    KConsoleRenderVisible();
}

void KConsoleWriteChar(char value)
{
    if (!gConsole.Available)
    {
        return;
    }

    if (value == '\r')
    {
        gConsole.CurrentColumn = 0U;
        return;
    }

    if (value == '\n')
    {
        KConsoleAppendNewLine();
        return;
    }

    if (value == '\t')
    {
        for (unsigned int index = 0U; index < 4U; ++index)
        {
            KConsoleWriteChar(' ');
        }
        return;
    }

    KConsoleStorePrintable(value);
}


int KConsoleScrollUpLines(unsigned int lines)
{
    if (!gConsole.Available || lines == 0U || gConsole.ViewTopLine == 0U)
    {
        return 0;
    }

    if (lines > gConsole.ViewTopLine)
    {
        gConsole.ViewTopLine = 0U;
    }
    else
    {
        gConsole.ViewTopLine -= lines;
    }

    gConsole.ViewFollowsTail = 0;
    KConsoleRenderVisible();
    return 1;
}

int KConsoleScrollDownLines(unsigned int lines)
{
    unsigned int maxTop = KConsoleMaximumViewTop();

    if (!gConsole.Available || lines == 0U || gConsole.ViewTopLine >= maxTop)
    {
        if (gConsole.Available && gConsole.ViewTopLine >= maxTop)
        {
            gConsole.ViewFollowsTail = 1;
        }
        return 0;
    }

    if (lines > (maxTop - gConsole.ViewTopLine))
    {
        gConsole.ViewTopLine = maxTop;
    }
    else
    {
        gConsole.ViewTopLine += lines;
    }

    gConsole.ViewFollowsTail = gConsole.ViewTopLine >= maxTop ? 1 : 0;
    KConsoleRenderVisible();
    return 1;
}

int KConsolePageUp(void)
{
    unsigned int amount = gConsole.VisibleRows > 1U ? gConsole.VisibleRows - 1U : 1U;
    return KConsoleScrollUpLines(amount);
}

int KConsolePageDown(void)
{
    unsigned int amount = gConsole.VisibleRows > 1U ? gConsole.VisibleRows - 1U : 1U;
    return KConsoleScrollDownLines(amount);
}

void KConsoleScrollToBottom(void)
{
    if (!gConsole.Available)
    {
        return;
    }

    gConsole.ViewTopLine = KConsoleMaximumViewTop();
    gConsole.ViewFollowsTail = 1;
    KConsoleRenderVisible();
}

unsigned int KConsoleVisibleRows(void)
{
    return gConsole.VisibleRows;
}

unsigned int KConsoleVisibleColumns(void)
{
    return gConsole.VisibleColumns;
}

unsigned int KConsoleScrollbackRows(void)
{
    return KCONSOLE_SCROLLBACK_ROWS;
}

int KConsoleIsDoubleBuffered(void)
{
    return gConsole.Available && gConsole.DoubleBuffered;
}

unsigned long long KConsoleBackBufferBytes(void)
{
    if (!gConsole.Available)
    {
        return 0ULL;
    }

    if (gConsole.Mode == KCONSOLE_MODE_FRAMEBUFFER)
    {
        return gConsole.FramebufferBackBufferSize;
    }

    return (unsigned long long)(KCONSOLE_VGA_WIDTH * KCONSOLE_VGA_HEIGHT * sizeof(unsigned short));
}

