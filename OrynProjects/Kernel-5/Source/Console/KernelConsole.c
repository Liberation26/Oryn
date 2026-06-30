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

#define KCONSOLE_WHITE 0x00FFFFFFU
#define KCONSOLE_BLACK 0x00000000U

#define KCONSOLE_MODE_NONE 0U
#define KCONSOLE_MODE_FRAMEBUFFER 1U
#define KCONSOLE_MODE_VGA_TEXT 2U

#define KCONSOLE_VGA_WIDTH 80U
#define KCONSOLE_VGA_HEIGHT 25U
#define KCONSOLE_VGA_ATTRIBUTE 0x0FU

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

typedef struct KConsoleState
{
    volatile unsigned int* Framebuffer;
    unsigned long long FramebufferSize;
    unsigned int Width;
    unsigned int Height;
    unsigned int Pitch;
    unsigned int CursorX;
    unsigned int CursorY;
    unsigned int Mode;
    int Available;
    int TtfReady;
    OrynTtfFont Font;
} KConsoleState;

static KConsoleState gConsole;
static volatile unsigned short* const gVgaText = (volatile unsigned short*)0xB8000ULL;

static unsigned int KConsoleActiveCellHeight(void)
{
    return gConsole.TtfReady ? KCONSOLE_TTF_CELL_HEIGHT : KCONSOLE_CELL_HEIGHT;
}

static unsigned int KConsoleActiveGlyphWidth(void)
{
    return gConsole.TtfReady ? KCONSOLE_TTF_PIXEL_HEIGHT : (KCONSOLE_GLYPH_WIDTH * KCONSOLE_SCALE);
}

static unsigned short KConsoleVgaEntry(char value)
{
    return (unsigned short)(((unsigned short)KCONSOLE_VGA_ATTRIBUTE << 8) | (unsigned char)value);
}

static void KConsoleUseVgaTextFallback(void)
{
    gConsole.Framebuffer = 0;
    gConsole.FramebufferSize = 0ULL;
    gConsole.Width = KCONSOLE_VGA_WIDTH;
    gConsole.Height = KCONSOLE_VGA_HEIGHT;
    gConsole.Pitch = KCONSOLE_VGA_WIDTH;
    gConsole.CursorX = 0U;
    gConsole.CursorY = 0U;
    gConsole.Mode = KCONSOLE_MODE_VGA_TEXT;
    gConsole.TtfReady = 0;
    gConsole.Available = 1;
}

static void KConsoleVgaClearScreen(void)
{
    for (unsigned int row = 0U; row < KCONSOLE_VGA_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            gVgaText[(row * KCONSOLE_VGA_WIDTH) + col] = KConsoleVgaEntry(' ');
        }
    }

    gConsole.CursorX = 0U;
    gConsole.CursorY = 0U;
}

static void KConsoleVgaScrollOneLine(void)
{
    for (unsigned int row = 1U; row < KCONSOLE_VGA_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            gVgaText[((row - 1U) * KCONSOLE_VGA_WIDTH) + col] =
                gVgaText[(row * KCONSOLE_VGA_WIDTH) + col];
        }
    }

    for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
    {
        gVgaText[((KCONSOLE_VGA_HEIGHT - 1U) * KCONSOLE_VGA_WIDTH) + col] = KConsoleVgaEntry(' ');
    }

    gConsole.CursorY = KCONSOLE_VGA_HEIGHT - 1U;
}

static void KConsoleVgaNewLine(void)
{
    gConsole.CursorX = 0U;
    gConsole.CursorY += 1U;

    if (gConsole.CursorY >= KCONSOLE_VGA_HEIGHT)
    {
        KConsoleVgaScrollOneLine();
    }
}

static void KConsoleVgaWriteChar(char value)
{
    if (value == '\r')
    {
        gConsole.CursorX = 0U;
        return;
    }

    if (value == '\n')
    {
        KConsoleVgaNewLine();
        return;
    }

    if (value == '\t')
    {
        for (unsigned int index = 0U; index < 4U; ++index)
        {
            KConsoleVgaWriteChar(' ');
        }
        return;
    }

    if (gConsole.CursorX >= KCONSOLE_VGA_WIDTH)
    {
        KConsoleVgaNewLine();
    }

    gVgaText[(gConsole.CursorY * KCONSOLE_VGA_WIDTH) + gConsole.CursorX] = KConsoleVgaEntry(value);
    gConsole.CursorX += 1U;
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
    unsigned long long byteOffset;

    if (!gConsole.Available || x >= gConsole.Width || y >= gConsole.Height)
    {
        return;
    }

    index = ((unsigned long long)y * (unsigned long long)gConsole.Pitch) + (unsigned long long)x;
    byteOffset = index * 4ULL;
    if (byteOffset + 3ULL >= gConsole.FramebufferSize)
    {
        return;
    }

    gConsole.Framebuffer[index] = colour;
}

static void KConsoleClearCell(unsigned int x, unsigned int y)
{
    for (unsigned int py = 0U; py < KCONSOLE_CELL_HEIGHT; ++py)
    {
        for (unsigned int px = 0U; px < KCONSOLE_CELL_WIDTH; ++px)
        {
            KConsolePutPixel(x + px, y + py, KCONSOLE_BLACK);
        }
    }
}

static void KConsoleScrollOneLine(void)
{
    unsigned int cellHeight = KConsoleActiveCellHeight();
    unsigned int fromY = KCONSOLE_MARGIN_Y + cellHeight;
    unsigned int toY = KCONSOLE_MARGIN_Y;
    unsigned int bottom = gConsole.Height - KCONSOLE_MARGIN_Y;

    if (bottom <= fromY)
    {
        gConsole.CursorX = KCONSOLE_MARGIN_X;
        gConsole.CursorY = KCONSOLE_MARGIN_Y;
        KConsoleClearScreen();
        return;
    }

    for (unsigned int y = fromY; y < bottom; ++y)
    {
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            unsigned long long source = ((unsigned long long)y * gConsole.Pitch) + x;
            unsigned long long target = ((unsigned long long)(toY + (y - fromY)) * gConsole.Pitch) + x;
            gConsole.Framebuffer[target] = gConsole.Framebuffer[source];
        }
    }

    for (unsigned int y = bottom - cellHeight; y < bottom; ++y)
    {
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            KConsolePutPixel(x, y, KCONSOLE_BLACK);
        }
    }

    gConsole.CursorY = bottom - cellHeight;
}

static void KConsoleNewLine(void)
{
    unsigned int cellHeight = KConsoleActiveCellHeight();
    gConsole.CursorX = KCONSOLE_MARGIN_X;
    gConsole.CursorY += cellHeight;

    if ((gConsole.CursorY + cellHeight) >= (gConsole.Height - KCONSOLE_MARGIN_Y))
    {
        KConsoleScrollOneLine();
    }
}

static unsigned int KConsoleDrawGlyph(char value)
{
    if (gConsole.TtfReady)
    {
        unsigned int advance = OrynTtfRenderAsciiGlyph(
            &gConsole.Font,
            value,
            gConsole.Framebuffer,
            gConsole.FramebufferSize,
            gConsole.Width,
            gConsole.Height,
            gConsole.Pitch,
            gConsole.CursorX,
            gConsole.CursorY,
            KCONSOLE_TTF_PIXEL_HEIGHT,
            KCONSOLE_WHITE);
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
                            KCONSOLE_WHITE);
                    }
                }
            }
        }
    }

    return KCONSOLE_GLYPH_WIDTH * KCONSOLE_SCALE;
}

void KConsoleInit(const OrynBootInfo* bootInfo)
{
    KConsoleUseVgaTextFallback();

    if (!KConsoleBootInfoHasUsableFramebuffer(bootInfo))
    {
        return;
    }

    gConsole.Framebuffer = (volatile unsigned int*)bootInfo->Framebuffer.Base;
    gConsole.FramebufferSize = bootInfo->Framebuffer.Size;
    gConsole.Width = bootInfo->Framebuffer.Width;
    gConsole.Height = KConsoleVisibleHeight(bootInfo);
    gConsole.Pitch = bootInfo->Framebuffer.PixelsPerScanLine;
    gConsole.CursorX = KCONSOLE_MARGIN_X;
    gConsole.CursorY = KCONSOLE_MARGIN_Y;
    gConsole.Mode = KCONSOLE_MODE_FRAMEBUFFER;
    gConsole.TtfReady = OrynTtfLoadFromBootInfo(bootInfo, &gConsole.Font);
    gConsole.Available = (gConsole.Height >= (KCONSOLE_MARGIN_Y + KConsoleActiveCellHeight() + KCONSOLE_MARGIN_Y)) ? 1 : 0;

    if (!gConsole.Available)
    {
        KConsoleUseVgaTextFallback();
    }
}

void KConsoleClearScreen(void)
{
    if (!gConsole.Available)
    {
        return;
    }

    if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsoleVgaClearScreen();
        return;
    }

    for (unsigned int y = 0U; y < gConsole.Height; ++y)
    {
        for (unsigned int x = 0U; x < gConsole.Width; ++x)
        {
            KConsolePutPixel(x, y, KCONSOLE_BLACK);
        }
    }

    gConsole.CursorX = KCONSOLE_MARGIN_X;
    gConsole.CursorY = KCONSOLE_MARGIN_Y;
}

void KConsoleWriteChar(char value)
{
    if (!gConsole.Available)
    {
        return;
    }

    if (gConsole.Mode == KCONSOLE_MODE_VGA_TEXT)
    {
        KConsoleVgaWriteChar(value);
        return;
    }

    if (value == '\r')
    {
        gConsole.CursorX = KCONSOLE_MARGIN_X;
        return;
    }

    if (value == '\n')
    {
        KConsoleNewLine();
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

    if ((gConsole.CursorX + KConsoleActiveGlyphWidth() + KCONSOLE_CHAR_SPACING_X) >= (gConsole.Width - KCONSOLE_MARGIN_X))
    {
        KConsoleNewLine();
    }

    unsigned int advance = KConsoleDrawGlyph(value);
    if (advance == 0U)
    {
        advance = KConsoleActiveGlyphWidth();
    }
    gConsole.CursorX += advance + KCONSOLE_CHAR_SPACING_X;
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
    KConsoleIsAvailable,
    KConsoleIsTtfActive
};
