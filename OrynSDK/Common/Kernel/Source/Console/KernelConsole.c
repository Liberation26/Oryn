#include "KernelConsole.h"
#include "KernelTtf.h"

#define KCONSOLE_SCALE 2U
#define KCONSOLE_GLYPH_WIDTH 5U
#define KCONSOLE_GLYPH_HEIGHT 7U
#define KCONSOLE_CHAR_SPACING_X 2U
#define KCONSOLE_LINE_SPACING_Y 4U
#define KCONSOLE_CELL_WIDTH ((KCONSOLE_GLYPH_WIDTH * KCONSOLE_SCALE) + KCONSOLE_CHAR_SPACING_X)
#define KCONSOLE_CELL_HEIGHT ((KCONSOLE_GLYPH_HEIGHT * KCONSOLE_SCALE) + KCONSOLE_LINE_SPACING_Y)
#define KCONSOLE_TTF_PIXEL_HEIGHT 16U
#define KCONSOLE_TTF_CELL_HEIGHT (KCONSOLE_TTF_PIXEL_HEIGHT + KCONSOLE_LINE_SPACING_Y)
#define KCONSOLE_MARGIN_X 8U
#define KCONSOLE_MARGIN_Y 8U
#define KCONSOLE_SCROLLBAR_WIDTH 4U
#define KCONSOLE_SCROLLBACK_ROWS 512U
#define KCONSOLE_SCROLLBACK_COLS 160U
#define KCONSOLE_BACKBUFFER_MAX_WIDTH 1920U
#define KCONSOLE_BACKBUFFER_MAX_HEIGHT 1080U
#define KCONSOLE_BACKBUFFER_PIXELS (KCONSOLE_BACKBUFFER_MAX_WIDTH * KCONSOLE_BACKBUFFER_MAX_HEIGHT)

#define KCONSOLE_WHITE KCONSOLE_COLOUR_DEFAULT
#define KCONSOLE_BLACK 0x00000000U

#define KCONSOLE_MODE_NONE 0U
#define KCONSOLE_MODE_FRAMEBUFFER 1U
#define KCONSOLE_MODE_VGA_TEXT 2U

#define KCONSOLE_VGA_WIDTH 80U
#define KCONSOLE_VGA_HEIGHT 25U
#define KCONSOLE_VGA_ATTRIBUTE_DEFAULT 0x0FU
#define KCONSOLE_VGA_ATTRIBUTE_INFO 0x0BU
#define KCONSOLE_VGA_ATTRIBUTE_PASS 0x0AU
#define KCONSOLE_VGA_ATTRIBUTE_WARN 0x0EU
#define KCONSOLE_VGA_ATTRIBUTE_FAIL 0x0CU
#define KCONSOLE_VGA_ATTRIBUTE_STEP 0x0DU
#define KCONSOLE_VGA_ATTRIBUTE_PCI 0x0BU

static const unsigned char gDigits[10][7] =
{
    { 14, 17, 19, 21, 25, 17, 14 },
    {  4, 12,  4,  4,  4,  4, 14 },
    { 14, 17,  1,  2,  4,  8, 31 },
    { 30,  1,  1, 14,  1,  1, 30 },
    {  2,  6, 10, 18, 31,  2,  2 },
    { 31, 16, 16, 30,  1,  1, 30 },
    { 14, 16, 16, 30, 17, 17, 14 },
    { 31,  1,  2,  4,  8,  8,  8 },
    { 14, 17, 17, 14, 17, 17, 14 },
    { 14, 17, 17, 15,  1,  1, 14 }
};

static const unsigned char gLetters[26][7] =
{
    { 14, 17, 17, 31, 17, 17, 17 },
    { 30, 17, 17, 30, 17, 17, 30 },
    { 14, 17, 16, 16, 16, 17, 14 },
    { 30, 17, 17, 17, 17, 17, 30 },
    { 31, 16, 16, 30, 16, 16, 31 },
    { 31, 16, 16, 30, 16, 16, 16 },
    { 14, 17, 16, 23, 17, 17, 15 },
    { 17, 17, 17, 31, 17, 17, 17 },
    { 14,  4,  4,  4,  4,  4, 14 },
    {  7,  2,  2,  2,  2, 18, 12 },
    { 17, 18, 20, 24, 20, 18, 17 },
    { 16, 16, 16, 16, 16, 16, 31 },
    { 17, 27, 21, 21, 17, 17, 17 },
    { 17, 25, 21, 19, 17, 17, 17 },
    { 14, 17, 17, 17, 17, 17, 14 },
    { 30, 17, 17, 30, 16, 16, 16 },
    { 14, 17, 17, 17, 21, 18, 13 },
    { 30, 17, 17, 30, 20, 18, 17 },
    { 15, 16, 16, 14,  1,  1, 30 },
    { 31,  4,  4,  4,  4,  4,  4 },
    { 17, 17, 17, 17, 17, 17, 14 },
    { 17, 17, 17, 17, 17, 10,  4 },
    { 17, 17, 17, 21, 21, 21, 10 },
    { 17, 17, 10,  4, 10, 17, 17 },
    { 17, 17, 10,  4,  4,  4,  4 },
    { 31,  1,  2,  4,  8, 16, 31 }
};

static const unsigned char gBlank[7] = { 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char gUnknown[7] = { 31, 17,  5,  2,  4,  0,  4 };
static const unsigned char gColon[7] = { 0,  4,  4,  0,  4,  4,  0 };
static const unsigned char gDot[7] = { 0,  0,  0,  0,  0, 12, 12 };
static const unsigned char gComma[7] = { 0,  0,  0,  0, 12,  4,  8 };
static const unsigned char gDash[7] = { 0,  0,  0, 31,  0,  0,  0 };
static const unsigned char gPlus[7] = { 0,  4,  4, 31,  4,  4,  0 };
static const unsigned char gSlash[7] = { 1,  2,  2,  4,  8,  8, 16 };
static const unsigned char gBackslash[7] = { 16,  8,  8,  4,  2,  2,  1 };
static const unsigned char gLeftBracket[7] = { 14,  8,  8,  8,  8,  8, 14 };
static const unsigned char gRightBracket[7] = { 14,  2,  2,  2,  2,  2, 14 };
static const unsigned char gLeftParen[7] = { 2,  4,  8,  8,  8,  4,  2 };
static const unsigned char gRightParen[7] = { 8,  4,  2,  2,  2,  4,  8 };
static const unsigned char gEquals[7] = { 0,  0, 31,  0, 31,  0,  0 };
static const unsigned char gUnderscore[7] = { 0,  0,  0,  0,  0,  0, 31 };

typedef struct KConsoleCell
{
    char Value;
    unsigned int Colour;
    unsigned char VgaAttribute;
} KConsoleCell;

typedef struct KConsoleState
{
    volatile unsigned int* Framebuffer;
    unsigned long long FramebufferSize;
    unsigned int* FramebufferBackBuffer;
    unsigned long long FramebufferBackBufferPixels;
    unsigned long long FramebufferBackBufferSize;
    int DoubleBuffered;
    unsigned int PresentCount;
    unsigned int LinePresentCount;
    unsigned int Width;
    unsigned int Height;
    unsigned int Pitch;
    unsigned int CursorX;
    unsigned int CursorY;
    unsigned int Mode;
    unsigned int ForegroundColour;
    unsigned char VgaAttribute;
    unsigned int VisibleColumns;
    unsigned int VisibleRows;
    unsigned int CellWidth;
    unsigned int CellHeight;
    unsigned int CurrentLine;
    unsigned int CurrentColumn;
    unsigned int TotalLines;
    unsigned int ViewTopLine;
    int ViewFollowsTail;
    int Available;
    int TtfReady;
    OrynTtfFont Font;
    KConsoleCell Cells[KCONSOLE_SCROLLBACK_ROWS][KCONSOLE_SCROLLBACK_COLS];
} KConsoleState;

static KConsoleState gConsole;
static unsigned int gFramebufferBackBuffer[KCONSOLE_BACKBUFFER_PIXELS];
static unsigned short gVgaShadowBuffer[KCONSOLE_VGA_WIDTH * KCONSOLE_VGA_HEIGHT];
static volatile unsigned short* const gVgaText = (volatile unsigned short*)0xB8000ULL;

static unsigned int KConsoleActiveCellHeight(void)
{
    return gConsole.TtfReady ? KCONSOLE_TTF_CELL_HEIGHT : KCONSOLE_CELL_HEIGHT;
}

static void KConsoleUseVgaTextFallback(void)
{
    gConsole.Framebuffer = 0;
    gConsole.FramebufferSize = 0ULL;
    gConsole.FramebufferBackBuffer = 0;
    gConsole.FramebufferBackBufferPixels = 0ULL;
    gConsole.FramebufferBackBufferSize = 0ULL;
    gConsole.DoubleBuffered = 1;
    gConsole.PresentCount = 0U;
    gConsole.LinePresentCount = 0U;
    gConsole.Width = KCONSOLE_VGA_WIDTH;
    gConsole.Height = KCONSOLE_VGA_HEIGHT;
    gConsole.Pitch = KCONSOLE_VGA_WIDTH;
    gConsole.CursorX = 0U;
    gConsole.CursorY = 0U;
    gConsole.Mode = KCONSOLE_MODE_VGA_TEXT;
    gConsole.ForegroundColour = KCONSOLE_COLOUR_DEFAULT;
    gConsole.VgaAttribute = KCONSOLE_VGA_ATTRIBUTE_DEFAULT;
    gConsole.TtfReady = 0;
    gConsole.Available = 1;
}

static int KConsoleBootInfoHasUsableFramebuffer(const OrynBootInfo* bootInfo)
{
    if (!OrynBootInfoAbiIsCompatible(bootInfo))
    {
        return 0;
    }

    if (bootInfo->Framebuffer.Base == 0ULL || bootInfo->Framebuffer.Size == 0ULL ||
        bootInfo->Framebuffer.Width == 0U || bootInfo->Framebuffer.Height == 0U ||
        bootInfo->Framebuffer.PixelsPerScanLine < bootInfo->Framebuffer.Width ||
        bootInfo->Framebuffer.BytesPerPixel != 4U)
    {
        return 0;
    }

    return 1;
}

static unsigned int KConsoleVisibleHeight(const OrynBootInfo* bootInfo)
{
    unsigned long long bytesPerLine = (unsigned long long)bootInfo->Framebuffer.PixelsPerScanLine *
        (unsigned long long)bootInfo->Framebuffer.BytesPerPixel;
    unsigned long long safeHeight;

    if (bytesPerLine == 0ULL)
    {
        return 0U;
    }

    safeHeight = bootInfo->Framebuffer.Size / bytesPerLine;
    if (safeHeight > (unsigned long long)bootInfo->Framebuffer.Height)
    {
        safeHeight = (unsigned long long)bootInfo->Framebuffer.Height;
    }

    return (unsigned int)safeHeight;
}

static const unsigned char* KConsoleGlyph(char value)
{
    if (value >= 'a' && value <= 'z')
    {
        value = (char)(value - 'a' + 'A');
    }

    if (value >= '0' && value <= '9') { return gDigits[value - '0']; }
    if (value >= 'A' && value <= 'Z') { return gLetters[value - 'A']; }
    if (value == ' ') { return gBlank; }
    if (value == ':') { return gColon; }
    if (value == '.') { return gDot; }
    if (value == ',') { return gComma; }
    if (value == '-') { return gDash; }
    if (value == '+') { return gPlus; }
    if (value == '/') { return gSlash; }
    if (value == '\\') { return gBackslash; }
    if (value == '[') { return gLeftBracket; }
    if (value == ']') { return gRightBracket; }
    if (value == '(') { return gLeftParen; }
    if (value == ')') { return gRightParen; }
    if (value == '=') { return gEquals; }
    if (value == '_') { return gUnderscore; }

    return gUnknown;
}

static void KConsolePutPixel(unsigned int x, unsigned int y, unsigned int colour)
{
    unsigned long long index;

    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER ||
        gConsole.FramebufferBackBuffer == 0 || x >= gConsole.Width || y >= gConsole.Height)
    {
        return;
    }

    index = ((unsigned long long)y * (unsigned long long)gConsole.Width) + (unsigned long long)x;
    if (index >= gConsole.FramebufferBackBufferPixels)
    {
        return;
    }

    gConsole.FramebufferBackBuffer[index] = colour;
}

static void KConsolePresentFramebuffer(void)
{
    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER ||
        gConsole.Framebuffer == 0 || gConsole.FramebufferBackBuffer == 0)
    {
        return;
    }

    for (unsigned int y = 0U; y < gConsole.Height; ++y)
    {
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            unsigned long long sourceIndex = ((unsigned long long)y * (unsigned long long)gConsole.Width) + (unsigned long long)x;
            unsigned long long targetIndex = ((unsigned long long)y * (unsigned long long)gConsole.Pitch) + (unsigned long long)x;
            unsigned long long targetByteOffset = targetIndex * 4ULL;

            if (sourceIndex < gConsole.FramebufferBackBufferPixels &&
                targetByteOffset + 3ULL < gConsole.FramebufferSize)
            {
                gConsole.Framebuffer[targetIndex] = gConsole.FramebufferBackBuffer[sourceIndex];
            }
        }
    }

    gConsole.PresentCount += 1U;
}

static void KConsolePresentVga(void)
{
    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_VGA_TEXT)
    {
        return;
    }

    for (unsigned int row = 0U; row < KCONSOLE_VGA_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            unsigned int index = (row * KCONSOLE_VGA_WIDTH) + col;
            gVgaText[index] = gVgaShadowBuffer[index];
        }
    }

    gConsole.PresentCount += 1U;
}

static void KConsolePresent(void)
{
    if (gConsole.Mode == KCONSOLE_MODE_FRAMEBUFFER)
    {
        KConsolePresentFramebuffer();
    }
    else if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsolePresentVga();
    }
}

static void KConsoleClearCell(unsigned int x, unsigned int y)
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

static unsigned int KConsoleDrawGlyph(char value)
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


static unsigned int KConsoleActiveCellWidth(void)
{
    return gConsole.TtfReady ? (KCONSOLE_TTF_PIXEL_HEIGHT + KCONSOLE_CHAR_SPACING_X) : KCONSOLE_CELL_WIDTH;
}

static KConsoleCell KConsoleBlankCell(void)
{
    KConsoleCell cell;
    cell.Value = ' ';
    cell.Colour = KCONSOLE_COLOUR_DEFAULT;
    cell.VgaAttribute = KCONSOLE_VGA_ATTRIBUTE_DEFAULT;
    return cell;
}

static void KConsoleClearLogicalLine(unsigned int line)
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

static void KConsoleClearScrollback(void)
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

static void KConsoleConfigureGeometry(void)
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

static unsigned int KConsoleMaximumViewTop(void)
{
    if (gConsole.TotalLines <= gConsole.VisibleRows)
    {
        return 0U;
    }

    return gConsole.TotalLines - gConsole.VisibleRows;
}

static void KConsoleFramebufferClearPixels(void)
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

static void KConsoleVgaClearPixels(void)
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

static void KConsoleDrawScrollbar(void)
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

static void KConsoleRenderCell(unsigned int screenRow, unsigned int screenColumn)
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
    }

    KConsoleClearLogicalLine(gConsole.CurrentLine);
    if (gConsole.ViewFollowsTail)
    {
        gConsole.ViewTopLine = KConsoleMaximumViewTop();
        KConsoleRenderVisible();
        gConsole.LinePresentCount += 1U;
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
        KConsoleRenderCell(gConsole.CurrentLine - gConsole.ViewTopLine, gConsole.CurrentColumn);
        KConsoleDrawScrollbar();
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

unsigned int KConsolePresentCount(void)
{
    return gConsole.PresentCount;
}

int KConsoleRunDoubleBufferProof(void)
{
    unsigned int before;

    if (!gConsole.Available || !gConsole.DoubleBuffered || KConsoleBackBufferBytes() == 0ULL)
    {
        return 0;
    }

    before = gConsole.PresentCount;
    KConsoleRenderVisible();
    return gConsole.PresentCount > before;
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

    ok = gConsole.TotalLines > gConsole.VisibleRows &&
        KConsoleScrollUpLines(1U) &&
        KConsoleScrollDownLines(1U) &&
        KConsolePageUp() &&
        KConsolePageDown();

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
