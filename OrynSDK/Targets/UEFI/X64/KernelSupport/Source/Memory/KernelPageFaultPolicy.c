#include "KernelPageFaultPolicy.h"
#include "KernelScreenReport.h"
#include "KernelVirtualMemory.h"

static OrynKernelPageFaultPolicyState gPageFaultPolicy;
static const OrynKernelAddressSpace* gFaultProcessAddressSpace;
static OrynKernelAddressSpace* gDemandAddressSpace;
static OrynKernelPhysicalMemory* gDemandPhysicalMemory;

static void ClearPolicy(void)
{
    unsigned char* bytes = (unsigned char*)&gPageFaultPolicy;
    for (unsigned long long index = 0ULL; index < sizeof(gPageFaultPolicy); ++index)
    {
        bytes[index] = 0U;
    }
    gFaultProcessAddressSpace = 0;
    gDemandAddressSpace = 0;
    gDemandPhysicalMemory = 0;
}

void OrynKernelPageFaultPolicyInit(void)
{
    ClearPolicy();
    gPageFaultPolicy.Initialized = 1U;
}

static int IsValidProcessAddressSpace(const OrynKernelAddressSpace* addressSpace)
{
    return addressSpace != 0 &&
        addressSpace->Initialized != 0U &&
        addressSpace->ProcessOwned != 0U &&
        addressSpace->UserBase == ORYN_VIRTUAL_USER_BASE &&
        addressSpace->UserLimit == ORYN_VIRTUAL_USER_LIMIT &&
        addressSpace->KernelBase == ORYN_VIRTUAL_KERNEL_BASE &&
        addressSpace->KernelLimit == ORYN_VIRTUAL_KERNEL_LIMIT;
}

void OrynKernelPageFaultPolicySetProcessContext(const OrynKernelAddressSpace* addressSpace)
{
    if (!gPageFaultPolicy.Initialized)
    {
        OrynKernelPageFaultPolicyInit();
    }

    if (IsValidProcessAddressSpace(addressSpace))
    {
        gFaultProcessAddressSpace = addressSpace;
    }
    else
    {
        gFaultProcessAddressSpace = 0;
        gDemandAddressSpace = 0;
        gDemandPhysicalMemory = 0;
    }
}

void OrynKernelPageFaultPolicySetDemandAllocator(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory)
{
    if (!gPageFaultPolicy.Initialized)
    {
        OrynKernelPageFaultPolicyInit();
    }

    if (IsValidProcessAddressSpace(addressSpace) &&
        physicalMemory != 0 &&
        physicalMemory->Initialized != 0U)
    {
        gDemandAddressSpace = addressSpace;
        gDemandPhysicalMemory = physicalMemory;
    }
    else
    {
        gDemandAddressSpace = 0;
        gDemandPhysicalMemory = 0;
    }
}

const OrynKernelPageFaultPolicyState* OrynKernelPageFaultPolicyGetState(void)
{
    return &gPageFaultPolicy;
}

static int IsGuardFault(unsigned long long address, unsigned long long errorCode)
{
    if ((errorCode & ORYN_PAGE_FAULT_USER) != 0ULL)
    {
        return 0;
    }

    if (!OrynVirtualMemoryIsKernelAddress(address))
    {
        return 0;
    }

    return (address & (ORYN_VIRTUAL_PAGE_SIZE - 1ULL)) == 0ULL;
}

static int FaultIsUserOwned(unsigned long long address, unsigned long long errorCode)
{
    return ((errorCode & ORYN_PAGE_FAULT_USER) != 0ULL) ||
        OrynVirtualMemoryIsUserAddress(address);
}

static void AccountFaultShape(unsigned long long address, unsigned long long errorCode)
{
    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) != 0ULL)
    {
        gPageFaultPolicy.ProtectionFaults += 1ULL;
    }
    else
    {
        gPageFaultPolicy.NonPresentFaults += 1ULL;
    }

    if ((errorCode & ORYN_PAGE_FAULT_RESERVED) != 0ULL)
    {
        gPageFaultPolicy.ReservedBitFaults += 1ULL;
    }

    if ((errorCode & ORYN_PAGE_FAULT_INSTRUCTION) != 0ULL)
    {
        gPageFaultPolicy.InstructionFetchFaults += 1ULL;
    }

    if (address == 0ULL)
    {
        gPageFaultPolicy.NullAddressFaults += 1ULL;
    }

    if ((errorCode & ORYN_PAGE_FAULT_USER) != 0ULL &&
        !OrynVirtualMemoryIsUserAddress(address))
    {
        gPageFaultPolicy.UserRangeViolations += 1ULL;
    }
}

static OrynKernelPageFaultAction FinishAction(OrynKernelPageFaultAction action)
{
    if (action == OrynKernelPageFaultActionRecover)
    {
        gPageFaultPolicy.NonFatalFaults += 1ULL;
        gPageFaultPolicy.RecoverActions += 1ULL;
    }
    else if (action == OrynKernelPageFaultActionKillProcess)
    {
        gPageFaultPolicy.NonFatalFaults += 1ULL;
        gPageFaultPolicy.KillProcessActions += 1ULL;
    }
    else
    {
        gPageFaultPolicy.FatalFaults += 1ULL;
        gPageFaultPolicy.KernelPanicActions += 1ULL;
    }

    gPageFaultPolicy.LastAction = (unsigned int)action;
    return action;
}

static int TryCopyOnWriteRecover(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long errorCode,
    unsigned long long faultAddress)
{
    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) == 0ULL)
    {
        return 0;
    }

    if ((errorCode & ORYN_PAGE_FAULT_WRITE) == 0ULL)
    {
        return 0;
    }

    if (addressSpace == 0 || physicalMemory == 0)
    {
        return 0;
    }

    return OrynVirtualMemoryResolveCopyOnWriteFault(
        addressSpace,
        physicalMemory,
        faultAddress);
}

static int TryDemandRecover(
    OrynKernelAddressSpace* addressSpace,
    OrynKernelPhysicalMemory* physicalMemory,
    unsigned long long errorCode,
    unsigned long long faultAddress)
{
    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) != 0ULL)
    {
        return 0;
    }

    if (addressSpace == 0 || physicalMemory == 0)
    {
        return 0;
    }

    return OrynVirtualMemoryDemandAllocateUserPage(
        addressSpace,
        physicalMemory,
        faultAddress,
        ORYN_VIRTUAL_FLAG_READ | ORYN_VIRTUAL_FLAG_WRITE | ORYN_VIRTUAL_FLAG_USER);
}

static OrynKernelPageFaultAction HandleUserFault(
    unsigned long long faultAddress,
    unsigned long long errorCode)
{
    gPageFaultPolicy.UserFaults += 1ULL;

    if (gFaultProcessAddressSpace == 0)
    {
        gPageFaultPolicy.InvalidUserFaultContexts += 1ULL;
        return OrynKernelPageFaultActionKernelPanic;
    }

    if (!OrynVirtualMemoryIsUserAddress(faultAddress))
    {
        gPageFaultPolicy.UserProcessFaults += 1ULL;
        return OrynKernelPageFaultActionKillProcess;
    }

    if (TryCopyOnWriteRecover(gDemandAddressSpace, gDemandPhysicalMemory, errorCode, faultAddress))
    {
        gPageFaultPolicy.CopyOnWriteFaults += 1ULL;
        return OrynKernelPageFaultActionRecover;
    }

    if (TryDemandRecover(gDemandAddressSpace, gDemandPhysicalMemory, errorCode, faultAddress))
    {
        gPageFaultPolicy.DemandAllocatedFaults += 1ULL;
        return OrynKernelPageFaultActionRecover;
    }

    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) == 0ULL && gDemandAddressSpace != 0)
    {
        gPageFaultPolicy.DemandAllocationFailures += 1ULL;
    }

    if ((errorCode & ORYN_PAGE_FAULT_PRESENT) != 0ULL &&
        (errorCode & ORYN_PAGE_FAULT_WRITE) != 0ULL &&
        gDemandAddressSpace != 0)
    {
        gPageFaultPolicy.CopyOnWriteFailures += 1ULL;
    }

    gPageFaultPolicy.UserProcessFaults += 1ULL;
    return OrynKernelPageFaultActionKillProcess;
}

OrynKernelPageFaultAction OrynKernelPageFaultPolicyHandle(
    OrynIdtInterruptFrame* frame,
    unsigned long long faultAddress)
{
    unsigned long long errorCode;
    OrynKernelPageFaultAction action;

    if (!gPageFaultPolicy.Initialized)
    {
        OrynKernelPageFaultPolicyInit();
    }

    errorCode = frame != 0 ? frame->ErrorCode : 0ULL;
    gPageFaultPolicy.TotalFaults += 1ULL;
    gPageFaultPolicy.PolicyDecisionChecks += 1ULL;
    gPageFaultPolicy.LastAddress = faultAddress;
    gPageFaultPolicy.LastErrorCode = errorCode;
    gPageFaultPolicy.LastRip = frame != 0 ? frame->Rip : 0ULL;

    if (frame == 0)
    {
        gPageFaultPolicy.FaultsWithoutFrame += 1ULL;
        gPageFaultPolicy.KernelFaults += 1ULL;
        return FinishAction(OrynKernelPageFaultActionKernelPanic);
    }

    AccountFaultShape(faultAddress, errorCode);

    if ((errorCode & ORYN_PAGE_FAULT_RESERVED) != 0ULL)
    {
        gPageFaultPolicy.KernelFaults += 1ULL;
        return FinishAction(OrynKernelPageFaultActionKernelPanic);
    }

    if (IsGuardFault(faultAddress, errorCode))
    {
        gPageFaultPolicy.GuardPageFaults += 1ULL;
        gPageFaultPolicy.KernelFaults += 1ULL;
        return FinishAction(OrynKernelPageFaultActionKernelPanic);
    }

    if (FaultIsUserOwned(faultAddress, errorCode))
    {
        action = HandleUserFault(faultAddress, errorCode);
        return FinishAction(action);
    }

    gPageFaultPolicy.KernelFaults += 1ULL;
    return FinishAction(OrynKernelPageFaultActionKernelPanic);
}

static void ClearFrame(OrynIdtInterruptFrame* frame)
{
    unsigned char* bytes = (unsigned char*)frame;
    for (unsigned long long index = 0ULL; index < sizeof(*frame); ++index)
    {
        bytes[index] = 0U;
    }
}

static void InitProofAddressSpace(OrynKernelAddressSpace* addressSpace)
{
    unsigned char* bytes = (unsigned char*)addressSpace;
    for (unsigned long long index = 0ULL; index < sizeof(*addressSpace); ++index)
    {
        bytes[index] = 0U;
    }

    addressSpace->Initialized = 1U;
    addressSpace->ProcessOwned = 1U;
    addressSpace->AddressSpaceId = 1U;
    addressSpace->UserBase = ORYN_VIRTUAL_USER_BASE;
    addressSpace->UserLimit = ORYN_VIRTUAL_USER_LIMIT;
    addressSpace->KernelBase = ORYN_VIRTUAL_KERNEL_BASE;
    addressSpace->KernelLimit = ORYN_VIRTUAL_KERNEL_LIMIT;
}

int OrynKernelPageFaultPolicyRunSelfTest(void)
{
    OrynIdtInterruptFrame userFrame;
    OrynIdtInterruptFrame userOutsideFrame;
    OrynIdtInterruptFrame userExecFrame;
    OrynIdtInterruptFrame kernelFrame;
    OrynIdtInterruptFrame reservedFrame;
    OrynKernelAddressSpace processSpace;
    int invalidUserIsFatal;
    int validUserIsProcessFault;
    int userOutsideIsKilled;
    int userExecIsKilled;
    int kernelIsFatal;
    int reservedIsFatal;
    int nullNoFrameIsFatal;

    ClearFrame(&userFrame);
    ClearFrame(&userOutsideFrame);
    ClearFrame(&userExecFrame);
    ClearFrame(&kernelFrame);
    ClearFrame(&reservedFrame);
    InitProofAddressSpace(&processSpace);

    OrynKernelPageFaultPolicyInit();
    userFrame.ErrorCode = ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;
    userOutsideFrame.ErrorCode = ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_WRITE;
    userExecFrame.ErrorCode = ORYN_PAGE_FAULT_PRESENT |
        ORYN_PAGE_FAULT_USER | ORYN_PAGE_FAULT_INSTRUCTION;
    kernelFrame.ErrorCode = ORYN_PAGE_FAULT_PRESENT | ORYN_PAGE_FAULT_WRITE;
    reservedFrame.ErrorCode = ORYN_PAGE_FAULT_RESERVED | ORYN_PAGE_FAULT_USER;

    invalidUserIsFatal = OrynKernelPageFaultPolicyHandle(
        &userFrame,
        ORYN_VIRTUAL_USER_BASE + 0x1234ULL) == OrynKernelPageFaultActionKernelPanic;

    OrynKernelPageFaultPolicySetProcessContext(&processSpace);
    validUserIsProcessFault = OrynKernelPageFaultPolicyHandle(
        &userFrame,
        ORYN_VIRTUAL_USER_BASE + 0x2234ULL) == OrynKernelPageFaultActionKillProcess;
    userOutsideIsKilled = OrynKernelPageFaultPolicyHandle(
        &userOutsideFrame,
        ORYN_VIRTUAL_KERNEL_BASE) == OrynKernelPageFaultActionKillProcess;
    userExecIsKilled = OrynKernelPageFaultPolicyHandle(
        &userExecFrame,
        ORYN_VIRTUAL_USER_BASE + 0x3234ULL) == OrynKernelPageFaultActionKillProcess;

    OrynKernelPageFaultPolicySetProcessContext(0);
    kernelIsFatal = OrynKernelPageFaultPolicyHandle(
        &kernelFrame,
        ORYN_VIRTUAL_KERNEL_BASE) == OrynKernelPageFaultActionKernelPanic;
    reservedIsFatal = OrynKernelPageFaultPolicyHandle(
        &reservedFrame,
        ORYN_VIRTUAL_USER_BASE + 0x4234ULL) == OrynKernelPageFaultActionKernelPanic;
    nullNoFrameIsFatal = OrynKernelPageFaultPolicyHandle(0, 0ULL) ==
        OrynKernelPageFaultActionKernelPanic;

    return invalidUserIsFatal && validUserIsProcessFault &&
        userOutsideIsKilled && userExecIsKilled && kernelIsFatal &&
        reservedIsFatal && nullNoFrameIsFatal;
}

void OrynKernelPageFaultPolicyPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.Initialized,
        "Page-fault policy handler is initialized.",
        "Page-fault policy handler is not initialized.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.PolicyDecisionChecks >= 7ULL,
        "Page-fault policy decision table is exercised.",
        "Page-fault policy decision table did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.FatalFaults != 0ULL &&
        gPageFaultPolicy.KernelPanicActions != 0ULL,
        "Kernel page faults are fatal by policy.",
        "Kernel page-fault fatal policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.InvalidUserFaultContexts != 0ULL,
        "User page faults without process context are fatal by policy.",
        "Invalid user page-fault context policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.UserProcessFaults != 0ULL &&
        gPageFaultPolicy.KillProcessActions != 0ULL,
        "User page faults with process context kill only that process.",
        "User page-fault process policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.UserRangeViolations != 0ULL,
        "User page faults outside user range are rejected by policy.",
        "User out-of-range page-fault policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.ReservedBitFaults != 0ULL,
        "Reserved-bit page faults are fatal by policy.",
        "Reserved-bit page-fault policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.InstructionFetchFaults != 0ULL,
        "Instruction-fetch page faults are classified by policy.",
        "Instruction-fetch page-fault policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.FaultsWithoutFrame != 0ULL,
        "Malformed page-fault frames are fatal by policy.",
        "Malformed page-fault frame policy did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.DemandAllocatedFaults != 0ULL,
        "Demand allocation handles non-present user pages.",
        "Demand allocation did not handle a user page fault.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.CopyOnWriteFaults != 0ULL,
        "Copy-on-write write faults resolve into private user pages.",
        "Copy-on-write page-fault resolution did not run.");
    OrynKernelScreenReportOkOrFail(gPageFaultPolicy.GuardPageFaults != 0ULL,
        "Guard-page page faults are classified as fatal.",
        "Guard-page page-fault policy did not classify faults.");
}
