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
    unsigned int InterruptsEnabledForInteractiveMode;
    unsigned long long InterruptCount;
    unsigned long long ScanCodesRead;
    unsigned long long ScrollLineUpEvents;
    unsigned long long ScrollLineDownEvents;
    unsigned long long PageUpEvents;
    unsigned long long PageDownEvents;
} OrynKernelKeyboardState;

int OrynKernelKeyboardInitForConsoleScroll(void);
void OrynKernelKeyboardEnableInteractiveInterrupts(void);
const OrynKernelKeyboardState* OrynKernelKeyboardGetState(void);
void OrynKernelKeyboardPrintProof(void);

#endif
