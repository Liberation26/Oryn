#include "KernelConsoleInternal.h"
void KConsoleResetDirtyLine(void)
{
    gConsole.DirtyLineActive = 0U;
    gConsole.DirtyScreenRow = 0U;
    gConsole.DirtyMinColumn = 0U;
    gConsole.DirtyMaxColumn = 0U;
}

void KConsolePresentScreenRowRange(
    unsigned int screenRow,
    unsigned int minColumn,
    unsigned int maxColumn)
{
    if (!gConsole.Available || screenRow >= gConsole.VisibleRows || minColumn > maxColumn)
    {
        return;
    }

    if (maxColumn >= gConsole.VisibleColumns)
    {
        maxColumn = gConsole.VisibleColumns - 1U;
    }

    if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsolePresentVgaRow(screenRow);
        return;
    }

    unsigned int left = KCONSOLE_MARGIN_X + (minColumn * gConsole.CellWidth);
    unsigned int top = KCONSOLE_MARGIN_Y + (screenRow * gConsole.CellHeight);
    unsigned int width = ((maxColumn - minColumn) + 1U) * gConsole.CellWidth;
    KConsolePresentFramebufferRect(left, top, width, gConsole.CellHeight);
}

void KConsoleMarkDirtyCell(unsigned int screenRow, unsigned int screenColumn)
{
    if (screenRow >= gConsole.VisibleRows || screenColumn >= gConsole.VisibleColumns)
    {
        return;
    }

    if (gConsole.DirtyLineActive && gConsole.DirtyScreenRow != screenRow)
    {
        KConsolePresentDirtyLine();
    }

    if (!gConsole.DirtyLineActive)
    {
        gConsole.DirtyLineActive = 1U;
        gConsole.DirtyScreenRow = screenRow;
        gConsole.DirtyMinColumn = screenColumn;
        gConsole.DirtyMaxColumn = screenColumn;
        return;
    }

    if (screenColumn < gConsole.DirtyMinColumn)
    {
        gConsole.DirtyMinColumn = screenColumn;
    }

    if (screenColumn > gConsole.DirtyMaxColumn)
    {
        gConsole.DirtyMaxColumn = screenColumn;
    }
}

void KConsolePresentDirtyLine(void)
{
    if (!gConsole.DirtyLineActive)
    {
        return;
    }

    KConsolePresentScreenRowRange(
        gConsole.DirtyScreenRow,
        gConsole.DirtyMinColumn,
        gConsole.DirtyMaxColumn);
    gConsole.LinePresentCount += 1U;
    KConsoleResetDirtyLine();
}

void KConsoleClearCell(unsigned int x, unsigned int y)
{
    unsigned int width = gConsole.CellWidth != 0U ? gConsole.CellWidth : KCONSOLE_CELL_WIDTH;
    unsigned int height = gConsole.CellHeight != 0U ? gConsole.CellHeight : KCONSOLE_CELL_HEIGHT;

    for (unsigned int py = 0U; py < height; ++py)
    {
        for (unsigned int px = 0U; px < width; ++px)
        {
            KConsolePutPixel(x + px, y + py, KCONSOLE_BLACK);
        }
    }
}

unsigned int KConsoleDrawGlyph(char value)
{
    if (gConsole.TtfReady)
    {
        unsigned int advance = OrynTtfRenderAsciiGlyph(
            &gConsole.Font,
            value,
            gConsole.FramebufferBackBuffer,
            gConsole.FramebufferBackBufferSize,
            gConsole.Width,
            gConsole.Height,
            gConsole.Width,
            gConsole.CursorX,
            gConsole.CursorY,
            KCONSOLE_TTF_PIXEL_HEIGHT,
            gConsole.ForegroundColour);
        if (advance != 0U)
        {
            return advance;
        }
    }

    const unsigned char* glyph = KConsoleGlyph(value);

    KConsoleClearCell(gConsole.CursorX, gConsole.CursorY);
    for (unsigned int row = 0U; row < KCONSOLE_GLYPH_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_GLYPH_WIDTH; ++col)
        {
            if ((glyph[row] & (1U << (KCONSOLE_GLYPH_WIDTH - 1U - col))) != 0U)
            {
                for (unsigned int sy = 0U; sy < KCONSOLE_SCALE; ++sy)
                {
                    for (unsigned int sx = 0U; sx < KCONSOLE_SCALE; ++sx)
                    {
                        KConsolePutPixel(
                            gConsole.CursorX + (col * KCONSOLE_SCALE) + sx,
                            gConsole.CursorY + (row * KCONSOLE_SCALE) + sy,
                            gConsole.ForegroundColour);
                    }
                }
            }
        }
    }

    return KCONSOLE_GLYPH_WIDTH * KCONSOLE_SCALE;
}


unsigned int KConsoleActiveCellWidth(void)
{
    return gConsole.TtfReady ? (KCONSOLE_TTF_PIXEL_HEIGHT + KCONSOLE_CHAR_SPACING_X) : KCONSOLE_CELL_WIDTH;
}

KConsoleCell KConsoleBlankCell(void)
{
    KConsoleCell cell;
    cell.Value = ' ';
    cell.Colour = KCONSOLE_COLOUR_DEFAULT;
    cell.VgaAttribute = KCONSOLE_VGA_ATTRIBUTE_DEFAULT;
    return cell;
}

void KConsoleClearLogicalLine(unsigned int line)
{
    if (line >= KCONSOLE_SCROLLBACK_ROWS)
    {
        return;
    }

    KConsoleCell blank = KConsoleBlankCell();
    for (unsigned int col = 0U; col < KCONSOLE_SCROLLBACK_COLS; ++col)
    {
        gConsole.Cells[line][col] = blank;
    }
}

void KConsoleClearScrollback(void)
{
    for (unsigned int row = 0U; row < KCONSOLE_SCROLLBACK_ROWS; ++row)
    {
        KConsoleClearLogicalLine(row);
    }

    gConsole.CurrentLine = 0U;
    gConsole.CurrentColumn = 0U;
    gConsole.TotalLines = 1U;
    gConsole.ViewTopLine = 0U;
    gConsole.ViewFollowsTail = 1;
}

void KConsoleConfigureGeometry(void)
{
    unsigned int usableWidth;
    unsigned int usableHeight;

    gConsole.CellWidth = KConsoleActiveCellWidth();
    gConsole.CellHeight = KConsoleActiveCellHeight();

    if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        gConsole.VisibleColumns = KCONSOLE_VGA_WIDTH;
        gConsole.VisibleRows = KCONSOLE_VGA_HEIGHT;
    }
    else
    {
        usableWidth = gConsole.Width > ((KCONSOLE_MARGIN_X * 2U) + KCONSOLE_SCROLLBAR_WIDTH) ?
            gConsole.Width - (KCONSOLE_MARGIN_X * 2U) - KCONSOLE_SCROLLBAR_WIDTH : 0U;
        usableHeight = gConsole.Height > (KCONSOLE_MARGIN_Y * 2U) ?
            gConsole.Height - (KCONSOLE_MARGIN_Y * 2U) : 0U;

        gConsole.VisibleColumns = gConsole.CellWidth != 0U ? usableWidth / gConsole.CellWidth : 0U;
        gConsole.VisibleRows = gConsole.CellHeight != 0U ? usableHeight / gConsole.CellHeight : 0U;
    }

    if (gConsole.VisibleColumns > KCONSOLE_SCROLLBACK_COLS)
    {
        gConsole.VisibleColumns = KCONSOLE_SCROLLBACK_COLS;
    }

    if (gConsole.VisibleRows > KCONSOLE_SCROLLBACK_ROWS)
    {
        gConsole.VisibleRows = KCONSOLE_SCROLLBACK_ROWS;
    }

    if (gConsole.VisibleColumns == 0U)
    {
        gConsole.VisibleColumns = 1U;
    }

    if (gConsole.VisibleRows == 0U)
    {
        gConsole.VisibleRows = 1U;
    }
}

unsigned int KConsoleMaximumViewTop(void)
{
    if (gConsole.TotalLines <= gConsole.VisibleRows)
    {
        return 0U;
    }

    return gConsole.TotalLines - gConsole.VisibleRows;
}

void KConsoleFramebufferClearPixels(void)
{
    if (gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER)
    {
        return;
    }

    for (unsigned int y = 0U; y < gConsole.Height; ++y)
    {
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            KConsolePutPixel(x, y, KCONSOLE_BLACK);
        }
    }
}

void KConsoleVgaClearPixels(void)
{
    for (unsigned int row = 0U; row < KCONSOLE_VGA_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            gVgaShadowBuffer[(row * KCONSOLE_VGA_WIDTH) + col] =
                (unsigned short)(((unsigned short)KCONSOLE_VGA_ATTRIBUTE_DEFAULT << 8) | (unsigned char)' ');
        }
    }
}

void KConsoleDrawScrollbar(void)
{
    if (gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER || gConsole.TotalLines <= gConsole.VisibleRows)
    {
        return;
    }

    unsigned int xStart = gConsole.Width > KCONSOLE_SCROLLBAR_WIDTH ?
        gConsole.Width - KCONSOLE_SCROLLBAR_WIDTH : 0U;
    unsigned int yStart = KCONSOLE_MARGIN_Y;
    unsigned int yEnd = gConsole.Height > KCONSOLE_MARGIN_Y ?
        gConsole.Height - KCONSOLE_MARGIN_Y : gConsole.Height;
    unsigned int trackHeight = yEnd > yStart ? yEnd - yStart : 0U;
    unsigned int thumbHeight = (trackHeight * gConsole.VisibleRows) / gConsole.TotalLines;
    unsigned int maxTop = KConsoleMaximumViewTop();
    unsigned int thumbTop;

    if (trackHeight == 0U)
    {
        return;
    }

    if (thumbHeight < 4U)
    {
        thumbHeight = 4U;
    }

    if (thumbHeight > trackHeight)
    {
        thumbHeight = trackHeight;
    }

    thumbTop = maxTop == 0U ? 0U : ((trackHeight - thumbHeight) * gConsole.ViewTopLine) / maxTop;

    for (unsigned int y = yStart; y < yEnd; ++y)
    {
        for (unsigned int x = xStart; x < gConsole.Width; ++x)
        {
            KConsolePutPixel(x, y, 0x00202020U);
        }
    }

    for (unsigned int y = yStart + thumbTop; y < yStart + thumbTop + thumbHeight && y < yEnd; ++y)
    {
        for (unsigned int x = xStart; x < gConsole.Width; ++x)
        {
            KConsolePutPixel(x, y, 0x00808080U);
        }
    }
}

void KConsoleRenderCell(unsigned int screenRow, unsigned int screenColumn)
{
    if (screenRow >= gConsole.VisibleRows || screenColumn >= gConsole.VisibleColumns)
    {
        return;
    }

    unsigned int logicalRow = gConsole.ViewTopLine + screenRow;
    KConsoleCell cell = KConsoleBlankCell();

    if (logicalRow < gConsole.TotalLines && logicalRow < KCONSOLE_SCROLLBACK_ROWS &&
        screenColumn < KCONSOLE_SCROLLBACK_COLS)
    {
        cell = gConsole.Cells[logicalRow][screenColumn];
    }

    if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        if (screenRow < KCONSOLE_VGA_HEIGHT && screenColumn < KCONSOLE_VGA_WIDTH)
        {
            gVgaShadowBuffer[(screenRow * KCONSOLE_VGA_WIDTH) + screenColumn] =
                (unsigned short)(((unsigned short)cell.VgaAttribute << 8) | (unsigned char)cell.Value);
        }
        return;
    }

    unsigned int savedX = gConsole.CursorX;
    unsigned int savedY = gConsole.CursorY;
    unsigned int savedColour = gConsole.ForegroundColour;
    unsigned char savedVga = gConsole.VgaAttribute;

    gConsole.CursorX = KCONSOLE_MARGIN_X + (screenColumn * gConsole.CellWidth);
    gConsole.CursorY = KCONSOLE_MARGIN_Y + (screenRow * gConsole.CellHeight);
    gConsole.ForegroundColour = cell.Colour;
    gConsole.VgaAttribute = cell.VgaAttribute;
    KConsoleClearCell(gConsole.CursorX, gConsole.CursorY);
    if (cell.Value != ' ')
    {
        (void)KConsoleDrawGlyph(cell.Value);
    }

    gConsole.CursorX = savedX;
    gConsole.CursorY = savedY;
    gConsole.ForegroundColour = savedColour;
    gConsole.VgaAttribute = savedVga;
}

void KConsoleFramebufferMoveBackBufferUpOneLine(void)
{
    if (gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER || gConsole.FramebufferBackBuffer == 0 ||
        gConsole.VisibleRows == 0U || gConsole.CellHeight == 0U)
    {
        return;
    }

    unsigned int textTop = KCONSOLE_MARGIN_Y;
    unsigned int textHeight = gConsole.VisibleRows * gConsole.CellHeight;
    if (textTop >= gConsole.Height)
    {
        return;
    }

    if (textTop + textHeight > gConsole.Height)
    {
        textHeight = gConsole.Height - textTop;
    }

    if (textHeight <= gConsole.CellHeight)
    {
        return;
    }

    unsigned int moveHeight = textHeight - gConsole.CellHeight;
    for (unsigned int y = 0U; y < moveHeight; ++y)
    {
        unsigned int targetY = textTop + y;
        unsigned int sourceY = targetY + gConsole.CellHeight;
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            unsigned long long targetIndex = ((unsigned long long)targetY * (unsigned long long)gConsole.Width) + x;
            unsigned long long sourceIndex = ((unsigned long long)sourceY * (unsigned long long)gConsole.Width) + x;
            if (targetIndex < gConsole.FramebufferBackBufferPixels && sourceIndex < gConsole.FramebufferBackBufferPixels)
            {
                gConsole.FramebufferBackBuffer[targetIndex] = gConsole.FramebufferBackBuffer[sourceIndex];
            }
        }
    }

    unsigned int bottomTop = textTop + moveHeight;
    for (unsigned int y = bottomTop; y < textTop + textHeight; ++y)
    {
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            unsigned long long index = ((unsigned long long)y * (unsigned long long)gConsole.Width) + x;
            if (index < gConsole.FramebufferBackBufferPixels)
            {
                gConsole.FramebufferBackBuffer[index] = KCONSOLE_BLACK;
            }
        }
    }
}

