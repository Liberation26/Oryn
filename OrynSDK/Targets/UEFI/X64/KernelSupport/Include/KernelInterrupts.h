#ifndef ORYN_KERNEL_INTERRUPTS_H
#define ORYN_KERNEL_INTERRUPTS_H

#include "KernelIdt.h"

#define ORYN_INTERRUPT_VECTOR_COUNT 256U
#define ORYN_INTERRUPT_EXCEPTION_COUNT 32U
#define ORYN_INTERRUPT_IRQ_BASE 0x20U
#define ORYN_INTERRUPT_IRQ_LIMIT 0x30U
#define ORYN_INTERRUPT_APIC_TIMER_VECTOR 0xEEU

typedef void (*OrynKernelInterruptHandler)(OrynIdtInterruptFrame* frame, void* context);

typedef struct OrynKernelInterruptState
{
    unsigned int Initialized;
    unsigned int HandlerSlots;
    unsigned int RegisteredHandlers;
    unsigned int InterruptsEnabled;
    unsigned int LastVector;
    unsigned int LastErrorCodeLow;
    unsigned long long LastRip;
    unsigned long long LastCr2;
    unsigned long long TotalDispatches;
    unsigned long long ExceptionDispatches;
    unsigned long long HardwareDispatches;
    unsigned long long EoiCount;
    unsigned long long PicEoiCount;
    unsigned long long ApicEoiCount;
    unsigned long long PicTimerInterrupts;
    unsigned long long PicTimerBefore;
    unsigned long long PicTimerAfter;
    unsigned int PicTimerProofRan;
    unsigned int PicTimerProofPassed;
    unsigned long long ApicTimerInterrupts;
    unsigned long long ApicTimerBefore;
    unsigned long long ApicTimerAfter;
    unsigned int ApicTimerProofRan;
    unsigned int ApicTimerProofPassed;
} OrynKernelInterruptState;

int OrynKernelInterruptsInit(void);
const OrynKernelInterruptState* OrynKernelInterruptsGetState(void);
int OrynKernelInterruptsRegisterHandler(
    unsigned int vector,
    OrynKernelInterruptHandler handler,
    void* context,
    const char* name);
unsigned long long OrynKernelInterruptsGetVectorCount(unsigned int vector);
void OrynKernelInterruptsEnable(void);
void OrynKernelInterruptsDisable(void);
unsigned int OrynKernelInterruptsAreEnabled(void);
void OrynKernelInterruptsDispatch(OrynIdtInterruptFrame* frame);
int OrynKernelInterruptsRunPicTimerProof(void);
int OrynKernelInterruptsRunApicTimerProof(void);
void OrynKernelInterruptsPrintProof(void);
void OrynKernelInterruptsPrintPicRuntimeProof(void);
void OrynKernelInterruptsPrintApicRuntimeProof(void);
void OrynKernelInterruptsPrintRuntimeProof(void);

#endif
