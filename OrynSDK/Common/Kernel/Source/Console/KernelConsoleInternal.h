#ifndef ORYN_KERNEL_CONSOLE_INTERNAL_H
#define ORYN_KERNEL_CONSOLE_INTERNAL_H

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


extern KConsoleState gConsole;
extern unsigned int gFramebufferBackBuffer[KCONSOLE_BACKBUFFER_PIXELS];
extern unsigned short gVgaShadowBuffer[KCONSOLE_VGA_WIDTH * KCONSOLE_VGA_HEIGHT];
extern volatile unsigned short* const gVgaText;

void KConsoleResetDirtyLine(void);
void KConsolePresentScreenRowRange( unsigned int screenRow, unsigned int minColumn, unsigned int maxColumn);
void KConsoleMarkDirtyCell(unsigned int screenRow, unsigned int screenColumn);
void KConsolePresentDirtyLine(void);
void KConsoleClearCell(unsigned int x, unsigned int y);
unsigned int KConsoleDrawGlyph(char value);
unsigned int KConsoleActiveCellWidth(void);
KConsoleCell KConsoleBlankCell(void);
void KConsoleClearLogicalLine(unsigned int line);
void KConsoleClearScrollback(void);
void KConsoleConfigureGeometry(void);
unsigned int KConsoleMaximumViewTop(void);
void KConsoleFramebufferClearPixels(void);
void KConsoleVgaClearPixels(void);
void KConsoleDrawScrollbar(void);
void KConsoleRenderCell(unsigned int screenRow, unsigned int screenColumn);
void KConsoleFramebufferMoveBackBufferUpOneLine(void);
unsigned int KConsolePresentCount(void);
unsigned int KConsoleBeginSilentProof(void);
void KConsoleEndSilentProof(unsigned int savedSuppress);
int KConsoleRunDoubleBufferProof(void);
int KConsoleRunLineBufferedFlipProof(void);
int KConsoleRunFastRefreshProof(void);
int KConsoleRunScrollProof(void);
unsigned char KConsoleVgaAttributeForColour(unsigned int colour);
void KConsoleSetForegroundColour(unsigned int colour);
void KConsoleResetForegroundColour(void);
int KConsoleIsAvailable(void);
int KConsoleIsTtfActive(void);
void KConsoleVgaMoveShadowUpOneLine(void);
void KConsoleFastTailScrollPresent(void);
void KConsoleRenderVisible(void);
int KConsoleCurrentLineIsVisible(void);
void KConsoleShiftScrollbackUp(void);
void KConsoleAppendNewLine(void);
void KConsoleStorePrintable(char value);
void KConsoleWriteProofDecimal(unsigned int value);
void KConsoleWriteProofText(const char* text);
void KConsoleInit(const OrynBootInfo* bootInfo);
void KConsoleClearScreen(void);
void KConsoleWriteChar(char value);
int KConsoleScrollUpLines(unsigned int lines);
int KConsoleScrollDownLines(unsigned int lines);
int KConsolePageUp(void);
int KConsolePageDown(void);
void KConsoleScrollToBottom(void);
unsigned int KConsoleVisibleRows(void);
unsigned int KConsoleVisibleColumns(void);
unsigned int KConsoleScrollbackRows(void);
int KConsoleIsDoubleBuffered(void);
unsigned long long KConsoleBackBufferBytes(void);
unsigned int KConsoleActiveCellHeight(void);
void KConsoleUseVgaTextFallback(void);
int KConsoleBootInfoHasUsableFramebuffer(const OrynBootInfo* bootInfo);
unsigned int KConsoleVisibleHeight(const OrynBootInfo* bootInfo);
const unsigned char* KConsoleGlyph(char value);
void KConsolePutPixel(unsigned int x, unsigned int y, unsigned int colour);
unsigned long long KConsoleSaveFlagsAndDisableInterrupts(void);
void KConsoleRestoreFlags(unsigned long long flags);
void KConsolePresentFramebuffer(void);
void KConsolePresentFramebufferRect( unsigned int left, unsigned int top, unsigned int width, unsigned int height);
void KConsolePresentVga(void);
void KConsolePresentVgaRow(unsigned int row);
void KConsolePresent(void);

#endif
