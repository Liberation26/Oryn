#ifndef ORYN_KERNEL_PAGE_FAULT_POLICY_H
#define ORYN_KERNEL_PAGE_FAULT_POLICY_H

#include "KernelIdt.h"

#define ORYN_PAGE_FAULT_PRESENT 0x01ULL
#define ORYN_PAGE_FAULT_WRITE 0x02ULL
#define ORYN_PAGE_FAULT_USER 0x04ULL
#define ORYN_PAGE_FAULT_RESERVED 0x08ULL
#define ORYN_PAGE_FAULT_INSTRUCTION 0x10ULL

typedef enum OrynKernelPageFaultAction
{
    OrynKernelPageFaultActionRecover = 0,
    OrynKernelPageFaultActionKillProcess = 1,
    OrynKernelPageFaultActionKernelPanic = 2
} OrynKernelPageFaultAction;

typedef struct OrynKernelPageFaultPolicyState
{
    unsigned int Initialized;
    unsigned long long TotalFaults;
    unsigned long long KernelFaults;
    unsigned long long UserFaults;
    unsigned long long GuardPageFaults;
    unsigned long long ProtectionFaults;
    unsigned long long NonPresentFaults;
    unsigned long long FatalFaults;
    unsigned long long NonFatalFaults;
    unsigned long long LastAddress;
    unsigned long long LastErrorCode;
    unsigned long long LastRip;
    unsigned int LastAction;
} OrynKernelPageFaultPolicyState;

void OrynKernelPageFaultPolicyInit(void);
OrynKernelPageFaultAction OrynKernelPageFaultPolicyHandle(
    OrynIdtInterruptFrame* frame,
    unsigned long long faultAddress);
const OrynKernelPageFaultPolicyState* OrynKernelPageFaultPolicyGetState(void);
void OrynKernelPageFaultPolicyPrintProof(void);
int OrynKernelPageFaultPolicyRunSelfTest(void);

#endif
