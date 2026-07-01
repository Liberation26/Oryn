#ifndef ORYN_KERNEL_INTERRUPTS_H
#define ORYN_KERNEL_INTERRUPTS_H

#include "KernelIdt.h"

#define ORYN_INTERRUPT_VECTOR_COUNT 256U
#define ORYN_INTERRUPT_EXCEPTION_COUNT 32U
#define ORYN_INTERRUPT_IRQ_BASE 0x20U
#define ORYN_INTERRUPT_IRQ_LIMIT 0x30U
#define ORYN_INTERRUPT_APIC_TIMER_VECTOR 0xEEU
#define ORYN_INTERRUPT_CPU_ACCOUNT_LIMIT 32U

typedef void (*OrynKernelInterruptHandler)(OrynIdtInterruptFrame* frame, void* context);

typedef struct OrynKernelInterruptCpuAccount
{
    unsigned int Used;
    unsigned int CpuIndex;
    unsigned int LocalApicId;
    unsigned int LastVector;
    unsigned long long TotalDispatches;
    unsigned long long ExceptionDispatches;
    unsigned long long HardwareDispatches;
    unsigned long long EoiCount;
} OrynKernelInterruptCpuAccount;

typedef struct OrynKernelInterruptState
{
    unsigned int Initialized;
    unsigned int HandlerSlots;
    unsigned int RegisteredHandlers;
    unsigned int RegisteredDeviceHandlers;
    unsigned int DeviceRouteSlots;
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
    unsigned int CpuAccountSlots;
    unsigned int CpuAccountsReady;
    unsigned int CurrentCpuIndex;
    unsigned int InterruptNesting;
    unsigned int IrqMaskApiReady;
    unsigned int IrqMaskOperations;
    unsigned int IrqUnmaskOperations;
    unsigned int IrqMaskRejected;
    unsigned int LastMaskedIrq;
    OrynKernelInterruptCpuAccount CpuAccounts[ORYN_INTERRUPT_CPU_ACCOUNT_LIMIT];
} OrynKernelInterruptState;

int OrynKernelInterruptsInit(void);
const OrynKernelInterruptState* OrynKernelInterruptsGetState(void);
int OrynKernelInterruptsRegisterHandler(
    unsigned int vector,
    OrynKernelInterruptHandler handler,
    void* context,
    const char* name);
int OrynKernelInterruptsRegisterDeviceHandler(
    unsigned int vector,
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    OrynKernelInterruptHandler handler,
    void* context,
    const char* name);
int OrynKernelInterruptsFindDeviceHandler(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int* vectorOut);
void OrynKernelInterruptsPrintDeviceProof(void);
void OrynKernelInterruptsPrintCpuAccountingProof(void);
unsigned long long OrynKernelInterruptsGetVectorCount(unsigned int vector);
unsigned int OrynKernelInterruptsGetCurrentCpuIndex(void);
unsigned int OrynKernelInterruptsAreInInterrupt(void);
void OrynKernelInterruptsEnable(void);
void OrynKernelInterruptsDisable(void);
unsigned int OrynKernelInterruptsAreEnabled(void);
int OrynKernelInterruptsMaskIrq(unsigned int irq);
int OrynKernelInterruptsUnmaskIrq(unsigned int irq);
int OrynKernelInterruptsSetIrqMasked(unsigned int irq, unsigned int masked);
void OrynKernelInterruptsPrintIrqMaskProof(void);
void OrynKernelInterruptsDispatch(OrynIdtInterruptFrame* frame);
int OrynKernelInterruptsRunPicTimerProof(void);
int OrynKernelInterruptsRunApicTimerProof(void);
void OrynKernelInterruptsPrintProof(void);
void OrynKernelInterruptsPrintPicRuntimeProof(void);
void OrynKernelInterruptsPrintApicRuntimeProof(void);
void OrynKernelInterruptsPrintRuntimeProof(void);

#endif
