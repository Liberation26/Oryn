#ifndef ORYN_KERNEL_BOOT_PROOF_CONSOLE_H
#define ORYN_KERNEL_BOOT_PROOF_CONSOLE_H

#include "KernelBootInfo.h"

#define ORYN_BOOT_PROOF_CONSOLE_COLOUR_STEP 0x00FF00FFU

typedef struct OrynBootProofConsoleMetrics
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
} OrynBootProofConsoleMetrics;

void OrynBootProofConsoleInitialize(const OrynBootInfo* bootInfo);
void OrynBootProofConsoleMarkRuntimeReady(void);
void OrynBootProofConsoleClear(void);
void OrynBootProofConsoleWriteChar(char value);
void OrynBootProofConsoleWriteString(const char* text);
void OrynBootProofConsoleWriteUnsignedDec(unsigned int value);
void OrynBootProofConsoleSetColour(unsigned int colour);
void OrynBootProofConsoleResetColour(void);
int OrynBootProofConsoleIsAvailable(void);
int OrynBootProofConsoleIsTtfActive(void);
int OrynBootProofConsoleIsDoubleBuffered(void);
unsigned int OrynBootProofConsoleVisibleRows(void);
unsigned int OrynBootProofConsoleVisibleColumns(void);
unsigned int OrynBootProofConsoleScrollbackRows(void);
unsigned long long OrynBootProofConsoleBackBufferBytes(void);
unsigned int OrynBootProofConsolePresentCount(void);
unsigned int OrynBootProofConsoleBeginDeferredPresent(void);
void OrynBootProofConsoleEndDeferredPresent(unsigned int savedState);
void OrynBootProofConsoleGetMetrics(OrynBootProofConsoleMetrics* metrics);
void OrynBootProofConsoleScrollToBottom(void);
int OrynBootProofConsoleScrollUpLines(unsigned int lines);
int OrynBootProofConsoleScrollDownLines(unsigned int lines);
int OrynBootProofConsolePageUp(void);
int OrynBootProofConsolePageDown(void);

#endif
