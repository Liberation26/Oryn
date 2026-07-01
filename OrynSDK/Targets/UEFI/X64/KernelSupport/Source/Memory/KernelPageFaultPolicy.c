#include "KernelPageFaultPolicy.h"
#include "KernelScreenReport.h"
#include "KernelVirtualMemory.h"

static OrynKernelPageFaultPolicyState gPageFaultPolicy;
static const OrynKernelAddressSpace* gFaultProcessAddressSpace;

static void ClearPolicy(void)
{
    unsigned char* bytes = (unsigned char*)&gPageFaultPolicy;
    for (unsigned long long index = 0; index < sizeof(gPageFaultPolicy); ++index)
    {
        bytes[index] = 0U;
    }
    gFaultProcessAddressSpace = 0;
}

void OrynKernelPageFaultPolicyInit(void)
{
    ClearPolicy();
    gPageFaultPolicy.Initialized = 1U;
}

void OrynKernelPageFaultPolicySetProcessContext(const OrynKernelAddressSpace* addressSpace)
{
    if (!gPageFaultPolicy.Initialized)
    {
        OrynKernelPageFaultPolicyInit();
    }

    if (addressSpace != 0 && addressSpace->Initialized != 0U && addressSpace->ProcessOwned != 0U)
    {
        gFaultProcessAddressSpace = addressSpace;
    }
    else
    {
        gFaultProcessAddressSpace = 0;
    }
}

const OrynKernelPageFaultPolicyState* OrynKernelPageFaultPolicyGetState(void)
{
    return &gPageFaultPolicy;
}

static int IsGuardFault(unsigned long long address, unsigned long long errorCode)
{
    if (OrynVirtualMemoryIsUserAddress(address))
    {
        return 0;
    }

    if ((errorCode & ORYN_PAGE_FAULT_USER) != 0ULL)
    {
        return 0;
    }

    if (OrynVirtualMemoryIsKernelAddress(address) &&
        (address & (ORYN_VIRTUAL_PAGE_SIZE - 1ULL)) == 0ULL)
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
    int userFault;
    int guardFault;
    OrynKernelPageFaultAction action;

    if (!gPageFaultPolicy.Initialized)
    {
        OrynKernelPageFaultPolicyInit();
    }

    gPageFaultPolicy.TotalFaults += 1ULL;
    gPageFaultPolicy.LastAddress = faultAddress;
    gPageFaultPolicy.LastErrorCode = errorCode;
    gPageFaultPolicy.LastRip = frame != 0 ? frame->Rip : 0ULL;

    userFault = ((errorCode & ORYN_PAGE_FAULT_USER) != 0ULL) ||
        OrynVirtualMemoryIsUserAddress(faultAddress);
    guardFault = IsGuardFault(faultAddress, errorCode);

    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) != 0ULL)
    {
        gPageFaultPolicy.ProtectionFaults += 1ULL;
    }
    else
    {
        gPageFaultPolicy.NonPresentFaults += 1ULL;
    }

    if (guardFault)
    {
        gPageFaultPolicy.GuardPageFaults += 1ULL;
        gPageFaultPolicy.KernelFaults += 1ULL;
        action = OrynKernelPageFaultActionKernelPanic;
    }
    else if (userFault)
    {
        gPageFaultPolicy.UserFaults += 1ULL;
        if (gFaultProcessAddressSpace != 0)
        {
            gPageFaultPolicy.UserProcessFaults += 1ULL;
            action = OrynKernelPageFaultActionKillProcess;
        }
        else
        {
            gPageFaultPolicy.InvalidUserFaultContexts += 1ULL;
            action = OrynKernelPageFaultActionKernelPanic;
        }
    }
    else
    {
        gPageFaultPolicy.KernelFaults += 1ULL;
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

static void ClearFrame(OrynIdtInterruptFrame* frame)
{
    unsigned char* bytes = (unsigned char*)frame;
    for (unsigned long long index = 0; index < sizeof(*frame); ++index)
    {
        bytes[index] = 0U;
    }
}

int OrynKernelPageFaultPolicyRunSelfTest(void)
{
    OrynIdtInterruptFrame userFrame;
    OrynIdtInterruptFrame kernelFrame;
    OrynKernelAddressSpace processSpace;
    unsigned char* processBytes = (unsigned char*)&processSpace;
    int invalidUserIsFatal;
    int validUserIsProcessFault;
    int kernelIsFatal;

    ClearFrame(&userFrame);
    ClearFrame(&kernelFrame);
    for (unsigned long long index = 0; index < sizeof(processSpace); ++index)
    {
        processBytes[index] = 0U;
    }

    OrynKernelPageFaultPolicyInit();
    userFrame.ErrorCode = ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;
    kernelFrame.ErrorCode = ORYN_PAGE_FAULT_PRESENT | ORYN_PAGE_FAULT_WRITE;

    invalidUserIsFatal = OrynKernelPageFaultPolicyHandle(
        &userFrame,
        ORYN_VIRTUAL_USER_BASE + 0x1234ULL) == OrynKernelPageFaultActionKernelPanic;

    processSpace.Initialized = 1U;
    processSpace.ProcessOwned = 1U;
    processSpace.AddressSpaceId = 1U;
    processSpace.UserBase = ORYN_VIRTUAL_USER_BASE;
    processSpace.UserLimit = ORYN_VIRTUAL_USER_LIMIT;
    processSpace.KernelBase = ORYN_VIRTUAL_KERNEL_BASE;
    processSpace.KernelLimit = ORYN_VIRTUAL_KERNEL_LIMIT;
    OrynKernelPageFaultPolicySetProcessContext(&processSpace);

    validUserIsProcessFault = OrynKernelPageFaultPolicyHandle(
        &userFrame,
        ORYN_VIRTUAL_USER_BASE + 0x2234ULL) == OrynKernelPageFaultActionKillProcess;

    OrynKernelPageFaultPolicySetProcessContext(0);
    kernelIsFatal = OrynKernelPageFaultPolicyHandle(
        &kernelFrame,
        ORYN_VIRTUAL_KERNEL_BASE) == OrynKernelPageFaultActionKernelPanic;

    return invalidUserIsFatal && validUserIsProcessFault && kernelIsFatal;
}

void OrynKernelPageFaultPolicyPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.Initialized,
        "Page-fault policy handler is initialized.",
        "Page-fault policy handler is not initialized.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.FatalFaults != 0ULL,
        "Kernel page faults are fatal by policy.",
        "Kernel page-fault fatal policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.InvalidUserFaultContexts != 0ULL,
        "User page faults without process context are fatal by policy.",
        "Invalid user page-fault context policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.UserProcessFaults != 0ULL && gPageFaultPolicy.NonFatalFaults != 0ULL,
        "User page faults with process context use non-fatal process policy.",
        "User page-fault process policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.GuardPageFaults != 0ULL,
        "Guard-page page faults are classified as fatal.",
        "Guard-page page-fault policy did not classify faults.");
}
