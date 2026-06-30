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
    unsigned int FastScrollPresentCount;
    unsigned int FullPresentCount;
    unsigned int StateOnlyScrollProofCount;
    unsigned int AtomicPresentCount;
    unsigned int PresentSuppressed;
    unsigned int DirtyLineActive;
    unsigned int DirtyScreenRow;
    unsigned int DirtyMinColumn;
    unsigned int DirtyMaxColumn;
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
    gConsole.FastScrollPresentCount = 0U;
    gConsole.FullPresentCount = 0U;
    gConsole.StateOnlyScrollProofCount = 0U;
    gConsole.AtomicPresentCount = 0U;
    gConsole.PresentSuppressed = 0U;
    gConsole.DirtyLineActive = 0U;
    gConsole.DirtyScreenRow = 0U;
    gConsole.DirtyMinColumn = 0U;
    gConsole.DirtyMaxColumn = 0U;
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

static unsigned long long KConsoleSaveFlagsAndDisableInterrupts(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    unsigned long long flags;
    __asm__ volatile ("pushfq; popq %0; cli" : "=r" (flags) :: "memory", "cc");
    return flags;
#else
    return 0ULL;
#endif
}

static void KConsoleRestoreFlags(unsigned long long flags)
{
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile ("pushq %0; popfq" :: "r" (flags) : "memory", "cc");
#else
    (void)flags;
#endif
}

static void KConsolePresentFramebuffer(void)
{
    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER ||
        gConsole.Framebuffer == 0 || gConsole.FramebufferBackBuffer == 0 ||
        gConsole.PresentSuppressed)
    {
        return;
    }

    unsigned long long flags = KConsoleSaveFlagsAndDisableInterrupts();

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
    gConsole.AtomicPresentCount += 1U;
    KConsoleRestoreFlags(flags);
}

static void KConsolePresentFramebufferRect(
    unsigned int left,
    unsigned int top,
    unsigned int width,
    unsigned int height)
{
    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_FRAMEBUFFER ||
        gConsole.Framebuffer == 0 || gConsole.FramebufferBackBuffer == 0 ||
        width == 0U || height == 0U || left >= gConsole.Width || top >= gConsole.Height ||
        gConsole.PresentSuppressed)
    {
        return;
    }

    if (left + width > gConsole.Width)
    {
        width = gConsole.Width - left;
    }

    if (top + height > gConsole.Height)
    {
        height = gConsole.Height - top;
    }

    unsigned long long flags = KConsoleSaveFlagsAndDisableInterrupts();

    for (unsigned int y = top; y < top + height; ++y)
    {
        for (unsigned int x = left; x < left + width; ++x)
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
    gConsole.AtomicPresentCount += 1U;
    KConsoleRestoreFlags(flags);
}

static void KConsolePresentVga(void)
{
    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_VGA_TEXT || gConsole.PresentSuppressed)
    {
        return;
    }

    unsigned long long flags = KConsoleSaveFlagsAndDisableInterrupts();

    for (unsigned int row = 0U; row < KCONSOLE_VGA_HEIGHT; ++row)
    {
        for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
        {
            unsigned int index = (row * KCONSOLE_VGA_WIDTH) + col;
            gVgaText[index] = gVgaShadowBuffer[index];
        }
    }

    gConsole.PresentCount += 1U;
    gConsole.AtomicPresentCount += 1U;
    KConsoleRestoreFlags(flags);
}

static void KConsolePresentVgaRow(unsigned int row)
{
    if (!gConsole.Available || gConsole.Mode != KCONSOLE_MODE_VGA_TEXT ||
        row >= KCONSOLE_VGA_HEIGHT || gConsole.PresentSuppressed)
    {
        return;
    }

    unsigned long long flags = KConsoleSaveFlagsAndDisableInterrupts();

    for (unsigned int col = 0U; col < KCONSOLE_VGA_WIDTH; ++col)
    {
        unsigned int index = (row * KCONSOLE_VGA_WIDTH) + col;
        gVgaText[index] = gVgaShadowBuffer[index];
    }

    gConsole.PresentCount += 1U;
    gConsole.AtomicPresentCount += 1U;
    KConsoleRestoreFlags(flags);
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

static void KConsolePresentDirtyLine(void);

