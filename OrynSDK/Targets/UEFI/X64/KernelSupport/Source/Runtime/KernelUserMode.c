#include "KernelUserMode.h"
#include "KernelCpu.h"
#include "KernelGdt.h"
#include "KernelScreenReport.h"

static OrynKernelUserModeState gUserModeState;

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

const OrynKernelUserModeState* OrynKernelUserModeGetState(void)
{
    return &gUserModeState;
}

int OrynKernelUserModeInit(void)
{
    const OrynKernelCpuFeatures* features;
    ClearBytes(&gUserModeState, sizeof(gUserModeState));
    features = OrynKernelCpuGetFeatures();
    gUserModeState.Initialized = 1U;
    gUserModeState.UserCodeSelector = OrynKernelGdtUserCodeSelector();
    gUserModeState.UserDataSelector = OrynKernelGdtUserDataSelector();
    gUserModeState.SyscallEntrySelected = 1U;
    if (features->HasSyscallSysret != 0U)
    {
        gUserModeState.UsesSyscallSysret = 1U;
    }
    else
    {
        gUserModeState.UsesInterruptFallback = 1U;
    }
    return 1;
}

int OrynKernelUserModePrepareControlledTest(OrynKernelThread* thread)
{
    unsigned long long kernelStackTop;
    if (gUserModeState.Initialized == 0U)
    {
        (void)OrynKernelUserModeInit();
    }
    if (thread == 0 || thread->IsUserThread == 0U || thread->StackTop == 0)
    {
        return 0;
    }
    kernelStackTop = (unsigned long long)thread->StackTop;
    OrynKernelGdtSetKernelStack(kernelStackTop);
    gUserModeState.KernelStackTop = kernelStackTop;
    gUserModeState.UserEntry = thread->CpuContext.Rip;
    gUserModeState.UserStackTop = thread->CpuContext.Rsp;
    gUserModeState.IretFrame.Rip = thread->CpuContext.Rip;
    gUserModeState.IretFrame.Cs = gUserModeState.UserCodeSelector;
    gUserModeState.IretFrame.Rflags = 0x202ULL;
    gUserModeState.IretFrame.Rsp = thread->CpuContext.Rsp;
    gUserModeState.IretFrame.Ss = gUserModeState.UserDataSelector;
    gUserModeState.UserThreadReady = 1U;
    gUserModeState.TssStackReady = OrynKernelGdtGetState()->UserStackSwitchReady;
    gUserModeState.IretFrameReady =
        (gUserModeState.IretFrame.Cs == ORYN_GDT_USER_CODE_SELECTOR &&
         gUserModeState.IretFrame.Ss == ORYN_GDT_USER_DATA_SELECTOR &&
         gUserModeState.IretFrame.Rip != 0ULL &&
         gUserModeState.IretFrame.Rsp != 0ULL) ? 1U : 0U;
    gUserModeState.ControlledProcessReady =
        (gUserModeState.UserThreadReady && gUserModeState.IretFrameReady &&
         gUserModeState.TssStackReady) ? 1U : 0U;
    return gUserModeState.ControlledProcessReady ? 1 : 0;
}

int OrynKernelUserModeRunProof(OrynKernelThread* thread)
{
    if (!OrynKernelUserModePrepareControlledTest(thread))
    {
        return 0;
    }
    return gUserModeState.ControlledProcessReady &&
        gUserModeState.SyscallEntrySelected ? 1 : 0;
}

void OrynKernelUserModePrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gUserModeState.ControlledProcessReady,
        "Ring 3 controlled test process entry frame is ready.",
        "Ring 3 controlled test process entry frame proof failed.");
    OrynKernelScreenReportOkOrFail(gUserModeState.TssStackReady,
        "TSS/user stack handling is ready for syscalls and interrupts.",
        "TSS/user stack handling proof failed.");
    OrynKernelScreenReportOkOrFail(gUserModeState.SyscallEntrySelected,
        "Syscall entry mechanism selected with interrupt fallback policy.",
        "Syscall entry mechanism was not selected.");
}
