#include "KernelPageFaultPolicy.h"
#include "KernelScreenReport.h"
#include "KernelVirtualMemory.h"

static OrynKernelPageFaultPolicyState gPageFaultPolicy;

static void ClearPolicy(void)
{
    unsigned char* bytes = (unsigned char*)&gPageFaultPolicy;
    for (unsigned long long index = 0; index < sizeof(gPageFaultPolicy); ++index)
    {
        bytes[index] = 0U;
    }
}

void OrynKernelPageFaultPolicyInit(void)
{
    ClearPolicy();
    gPageFaultPolicy.Initialized = 1U;
}

const OrynKernelPageFaultPolicyState* OrynKernelPageFaultPolicyGetState(void)
{
    return &gPageFaultPolicy;
}

static int IsGuardFault(unsigned long long address)
{
    if ((address & (ORYN_VIRTUAL_PAGE_SIZE - 1ULL)) == 0ULL)
    {
        return 1;
    }
    return 0;
}

OrynKernelPageFaultAction OrynKernelPageFaultPolicyHandle(
    OrynIdtInterruptFrame* frame,
    unsigned long long faultAddress)
{
    unsigned long long errorCode = frame != 0 ? frame->ErrorCode : 0ULL;
    OrynKernelPageFaultAction action;

    if (!gPageFaultPolicy.Initialized)
    {
        OrynKernelPageFaultPolicyInit();
    }

    gPageFaultPolicy.TotalFaults += 1ULL;
    gPageFaultPolicy.LastAddress = faultAddress;
    gPageFaultPolicy.LastErrorCode = errorCode;
    gPageFaultPolicy.LastRip = frame != 0 ? frame->Rip : 0ULL;

    if ((errorCode & ORYN_PAGE_FAULT_USER) != 0ULL || OrynVirtualMemoryIsUserAddress(faultAddress))
    {
        gPageFaultPolicy.UserFaults += 1ULL;
        action = OrynKernelPageFaultActionKillProcess;
    }
    else
    {
        gPageFaultPolicy.KernelFaults += 1ULL;
        action = OrynKernelPageFaultActionKernelPanic;
    }

    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) != 0ULL)
    {
        gPageFaultPolicy.ProtectionFaults += 1ULL;
    }
    else
    {
        gPageFaultPolicy.NonPresentFaults += 1ULL;
    }

    if (IsGuardFault(faultAddress))
    {
        gPageFaultPolicy.GuardPageFaults += 1ULL;
        action = OrynKernelPageFaultActionKernelPanic;
    }

    if (action == OrynKernelPageFaultActionKernelPanic)
    {
        gPageFaultPolicy.FatalFaults += 1ULL;
    }
    else
    {
        gPageFaultPolicy.NonFatalFaults += 1ULL;
    }

    gPageFaultPolicy.LastAction = (unsigned int)action;
    return action;
}

int OrynKernelPageFaultPolicyRunSelfTest(void)
{
    OrynIdtInterruptFrame userFrame;
    OrynIdtInterruptFrame kernelFrame;
    unsigned char* userBytes = (unsigned char*)&userFrame;
    unsigned char* kernelBytes = (unsigned char*)&kernelFrame;
    for (unsigned long long index = 0; index < sizeof(userFrame); ++index)
    {
        userBytes[index] = 0U;
    }
    for (unsigned long long index = 0; index < sizeof(kernelFrame); ++index)
    {
        kernelBytes[index] = 0U;
    }
    OrynKernelPageFaultPolicyInit();
    userFrame.ErrorCode = ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;
    kernelFrame.ErrorCode = ORYN_PAGE_FAULT_PRESENT | ORYN_PAGE_FAULT_WRITE;
    return OrynKernelPageFaultPolicyHandle(&userFrame, ORYN_VIRTUAL_USER_BASE + 0x1000ULL) ==
            OrynKernelPageFaultActionKillProcess &&
        OrynKernelPageFaultPolicyHandle(&kernelFrame, ORYN_VIRTUAL_KERNEL_BASE) ==
            OrynKernelPageFaultActionKernelPanic;
}

void OrynKernelPageFaultPolicyPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.Initialized,
        "Page-fault policy handler is initialized.",
        "Page-fault policy handler is not initialized.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.FatalFaults != 0ULL,
        "Kernel page faults are fatal by policy.",
        "Kernel page-fault fatal policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.NonFatalFaults != 0ULL,
        "User page faults use non-fatal process policy.",
        "User page-fault process policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.GuardPageFaults != 0ULL,
        "Guard-page page faults are classified as fatal.",
        "Guard-page page-fault policy did not classify faults.");
}
