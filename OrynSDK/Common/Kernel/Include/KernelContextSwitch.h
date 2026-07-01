#ifndef ORYN_KERNEL_CONTEXT_SWITCH_H
#define ORYN_KERNEL_CONTEXT_SWITCH_H

typedef struct OrynKernelCpuContext
{
    unsigned long long Rsp;
    unsigned long long Rbp;
    unsigned long long Rbx;
    unsigned long long R12;
    unsigned long long R13;
    unsigned long long R14;
    unsigned long long R15;
    unsigned long long Rip;
    unsigned long long Rflags;
    unsigned long long Cr3;
} OrynKernelCpuContext;

typedef struct OrynKernelContextSwitchStats
{
    unsigned int Initialized;
    unsigned int X64ContextSwitchReady;
    unsigned int SavedRegisterCount;
    unsigned int RestoredRegisterCount;
    unsigned int ProofSwitchCount;
} OrynKernelContextSwitchStats;

void OrynKernelContextSwitchInit(void);
void OrynKernelContextSwitchPrepareInitial(
    OrynKernelCpuContext* context,
    void* stackTop,
    void (*entryPoint)(void* context),
    void* entryContext,
    unsigned long long cr3);
void OrynKernelX64ContextSwitch(OrynKernelCpuContext* oldContext,
    const OrynKernelCpuContext* newContext);
int OrynKernelContextSwitchRunSelfTest(void);
const OrynKernelContextSwitchStats* OrynKernelContextSwitchGetStats(void);
void OrynKernelContextSwitchPrintProof(void);

#endif
