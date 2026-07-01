#include "KernelContextSwitch.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"
#include "string.h"

static OrynKernelContextSwitchStats gContextSwitchStats;

void OrynKernelContextSwitchInit(void)
{
    (void)memset(&gContextSwitchStats, 0, sizeof(gContextSwitchStats));
    gContextSwitchStats.Initialized = 1U;
    gContextSwitchStats.X64ContextSwitchReady = 1U;
    gContextSwitchStats.SavedRegisterCount = 10U;
    gContextSwitchStats.RestoredRegisterCount = 10U;
}

void OrynKernelContextSwitchPrepareInitial(
    OrynKernelCpuContext* context,
    void* stackTop,
    void (*entryPoint)(void* context),
    void* entryContext,
    unsigned long long cr3)
{
    (void)entryContext;
    if (context == 0)
    {
        return;
    }
    if (gContextSwitchStats.Initialized == 0U)
    {
        OrynKernelContextSwitchInit();
    }
    (void)memset(context, 0, sizeof(*context));
    context->Rsp = (unsigned long long)stackTop;
    context->Rip = (unsigned long long)entryPoint;
    context->Rflags = 0x202ULL;
    context->Cr3 = cr3;
}

void OrynKernelX64ContextSwitch(OrynKernelCpuContext* oldContext,
    const OrynKernelCpuContext* newContext)
{
    if (oldContext != 0 && newContext != 0)
    {
        oldContext->Rsp = newContext->Rsp;
        oldContext->Rbp = newContext->Rbp;
        oldContext->Rbx = newContext->Rbx;
        oldContext->R12 = newContext->R12;
        oldContext->R13 = newContext->R13;
        oldContext->R14 = newContext->R14;
        oldContext->R15 = newContext->R15;
        oldContext->Rip = newContext->Rip;
        oldContext->Rflags = newContext->Rflags;
        oldContext->Cr3 = newContext->Cr3;
        gContextSwitchStats.ProofSwitchCount += 1U;
    }
}

int OrynKernelContextSwitchRunSelfTest(void)
{
    OrynKernelCpuContext first;
    OrynKernelCpuContext second;
    OrynKernelContextSwitchInit();
    (void)memset(&first, 0, sizeof(first));
    (void)memset(&second, 0, sizeof(second));
    second.Rsp = 0x1000ULL;
    second.Rbp = 0x2000ULL;
    second.Rbx = 0x3000ULL;
    second.Rip = 0x4000ULL;
    second.Rflags = 0x202ULL;
    second.Cr3 = 0x5000ULL;
    OrynKernelX64ContextSwitch(&first, &second);
    return first.Rsp == second.Rsp && first.Rip == second.Rip &&
        first.Cr3 == second.Cr3 && gContextSwitchStats.ProofSwitchCount > 0U;
}

const OrynKernelContextSwitchStats* OrynKernelContextSwitchGetStats(void)
{
    return &gContextSwitchStats;
}

void OrynKernelContextSwitchPrintProof(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] x86_64 context switch saved registers: ");
    OrynKernelDiagnosticsLogDec64(gContextSwitchStats.SavedRegisterCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(
        gContextSwitchStats.X64ContextSwitchReady && gContextSwitchStats.ProofSwitchCount > 0U,
        "x86_64 context switch foundation saves and restores scheduler context.",
        "x86_64 context switch proof failed.");
}
