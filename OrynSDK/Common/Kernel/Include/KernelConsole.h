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

typedef struct KConsoleApi
{
    void (*ClearScreen)(void);
    void (*WriteChar)(char value);
    void (*SetForegroundColour)(unsigned int colour);
    void (*ResetForegroundColour)(void);
    int (*IsAvailable)(void);
    int (*IsTtfActive)(void);
} KConsoleApi;

extern const KConsoleApi KConsole;

void KConsoleInit(const OrynBootInfo* bootInfo);
void KConsoleClearScreen(void);
void KConsoleWriteChar(char value);
void KConsoleSetForegroundColour(unsigned int colour);
void KConsoleResetForegroundColour(void);
int KConsoleIsAvailable(void);
int KConsoleIsTtfActive(void);

#endif
