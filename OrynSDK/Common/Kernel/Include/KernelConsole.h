#ifndef ORYN_KERNEL_5_CONSOLE_H
#define ORYN_KERNEL_5_CONSOLE_H

#include "KernelBootInfo.h"

typedef struct KConsoleApi
{
    void (*ClearScreen)(void);
    void (*WriteChar)(char value);
    int (*IsAvailable)(void);
    int (*IsTtfActive)(void);
} KConsoleApi;

extern const KConsoleApi KConsole;

void KConsoleInit(const OrynBootInfo* bootInfo);
void KConsoleClearScreen(void);
void KConsoleWriteChar(char value);
int KConsoleIsAvailable(void);
int KConsoleIsTtfActive(void);

#endif
