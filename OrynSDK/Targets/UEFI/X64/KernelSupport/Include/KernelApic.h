#ifndef ORYN_KERNEL_APIC_H
#define ORYN_KERNEL_APIC_H

typedef struct OrynKernelApicState
{
    unsigned int Initialized;
    unsigned int CpuHasApic;
    unsigned int CpuHasApic2;
    unsigned int XApicEnabled;
    unsigned int Apic2Enabled;
    unsigned int SoftwareEnabled;
    unsigned int TimerProbeRan;
    unsigned int TimerCountMoved;
    unsigned int LocalApicId;
    unsigned int Version;
    unsigned int MaxLvt;
    unsigned long long BaseMsrBefore;
    unsigned long long BaseMsrAfter;
    unsigned long long LocalApicPhysicalBase;
    unsigned long long TimerCountBefore;
    unsigned long long TimerCountAfter;
    unsigned int TimerInterruptVector;
    unsigned int TimerInterruptArmed;
} OrynKernelApicState;

int OrynKernelApicInit(int preferApic2);
const OrynKernelApicState* OrynKernelApicGetState(void);
void OrynKernelApicSendEoi(void);
int OrynKernelApicStartOneShotTimer(unsigned int vector, unsigned int initialCount, unsigned int divideMode);
void OrynKernelApicMaskTimer(void);
unsigned long long OrynKernelApicReadTimerCurrent(void);
int OrynKernelApicCanSendIpi(void);
int OrynKernelApicSendInitIpi(unsigned int targetApicId);
int OrynKernelApicSendStartupIpi(unsigned int targetApicId, unsigned int startupVector);
void OrynKernelApicPrintProof(void);

#endif
