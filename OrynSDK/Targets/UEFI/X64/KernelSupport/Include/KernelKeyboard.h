#ifndef ORYN_KERNEL_KEYBOARD_H
#define ORYN_KERNEL_KEYBOARD_H

typedef struct OrynKernelKeyboardState
{
    unsigned int Initialized;
    unsigned int HandlerRegistered;
    unsigned int UsesInterrupts;
    unsigned int PicIrq1Unmasked;
    unsigned int ApicLegacyBridgeReady;
    unsigned int DecoderReady;
    unsigned int MakeBreakStateReady;
    unsigned int ReleaseStopsScrolling;
    unsigned int UpHeld;
    unsigned int DownHeld;
    unsigned int PageUpHeld;
    unsigned int PageDownHeld;
    unsigned int InterruptsEnabledForInteractiveMode;
    unsigned long long InterruptCount;
    unsigned long long ScanCodesRead;
    unsigned long long ScrollLineUpEvents;
    unsigned long long ScrollLineDownEvents;
    unsigned long long PageUpEvents;
    unsigned long long PageDownEvents;
    unsigned long long ReleaseEvents;
    unsigned long long StopOnReleaseEvents;
} OrynKernelKeyboardState;

int OrynKernelKeyboardInitForConsoleScroll(void);
void OrynKernelKeyboardEnableInteractiveInterrupts(void);
const OrynKernelKeyboardState* OrynKernelKeyboardGetState(void);
void OrynKernelKeyboardPrintProof(void);

#endif
