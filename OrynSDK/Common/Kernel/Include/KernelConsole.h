#ifndef ORYN_KERNEL_5_CONSOLE_H
#define ORYN_KERNEL_5_CONSOLE_H

#include "KernelBootInfo.h"

#define KCONSOLE_COLOUR_DEFAULT 0x00FFFFFFU
#define KCONSOLE_COLOUR_INFO 0x0000FFFFU
#define KCONSOLE_COLOUR_PASS 0x0000FF00U
#define KCONSOLE_COLOUR_OK 0x0000FF00U
#define KCONSOLE_COLOUR_WARN 0x00FFFF00U
#define KCONSOLE_COLOUR_FAIL 0x00FF3030U
#define KCONSOLE_COLOUR_STEP 0x00FF00FFU
#define KCONSOLE_COLOUR_PCI 0x0000B0FFU

typedef struct KConsoleMetrics
{
    unsigned int PresentCount;
    unsigned int LinePresentCount;
    unsigned int FastScrollPresentCount;
    unsigned int FullPresentCount;
    unsigned int AtomicPresentCount;
    unsigned int TotalLines;
    unsigned int ViewTopLine;
    unsigned int ViewFollowsTail;
    unsigned int CurrentLine;
    unsigned int CurrentColumn;
} KConsoleMetrics;

typedef struct KConsoleApi
{
    void (*ClearScreen)(void);
    void (*WriteChar)(char value);
    void (*WriteString)(const char* text);
    void (*WriteUnsignedDec)(unsigned int value);
    void (*SetForegroundColour)(unsigned int colour);
    void (*ResetForegroundColour)(void);
    int (*IsAvailable)(void);
    int (*IsTtfActive)(void);
    int (*ScrollUpLines)(unsigned int lines);
    int (*ScrollDownLines)(unsigned int lines);
    int (*PageUp)(void);
    int (*PageDown)(void);
    void (*ScrollToBottom)(void);
    unsigned int (*VisibleRows)(void);
    unsigned int (*VisibleColumns)(void);
    unsigned int (*ScrollbackRows)(void);
    int (*IsDoubleBuffered)(void);
    unsigned long long (*BackBufferBytes)(void);
    unsigned int (*PresentCount)(void);
    void (*GetMetrics)(KConsoleMetrics* metrics);
    unsigned int (*BeginDeferredPresent)(void);
    void (*EndDeferredPresent)(unsigned int saved_state);
} KConsoleApi;

extern const KConsoleApi KConsole;

void KConsoleInit(const OrynBootInfo* bootInfo);
void KConsoleClearScreen(void);
void KConsoleWriteChar(char value);
void KConsoleWriteString(const char* text);
void KConsoleWriteUnsignedDec(unsigned int value);
void KConsoleSetForegroundColour(unsigned int colour);
void KConsoleResetForegroundColour(void);
int KConsoleIsAvailable(void);
int KConsoleIsTtfActive(void);
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
unsigned int KConsolePresentCount(void);
void KConsoleGetMetrics(KConsoleMetrics* metrics);
unsigned int KConsoleBeginDeferredPresent(void);
void KConsoleEndDeferredPresent(unsigned int saved_state);

#endif
