#include "KernelDiagnostics.h"
#include "KernelIo.h"
#include "KernelLifecycle.h"
#include "KernelPanic.h"
#include "KernelRuntime.h"
#include "KernelRuntimeInternal.h"

static void OrynKernelRuntimeHandleActivePanic(void)
{
    if (!OrynKernelPanicIsActive())
    {
        return;
    }

    KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
    OrynKernelPanicHalt();
}

static void OrynKernelRuntimeEnterRunningState(void)
{
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleRunning);
    KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
}

static void OrynKernelRuntimeRunBootSequence(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelDiagnosticsRunBootProofs(kernelBootInfo);
    OrynKernelRuntimeHandleActivePanic();
    OrynKernelRuntimeEnterRunningState();
    OrynKernelDiagnosticsRunHaltProofs();
}

void OrynKernelRuntimeEnterAndStartBootSequence(const OrynBootInfo* bootInfo)
{
    const OrynBootInfo* kernelBootInfo = OrynKernelRuntimeEnter(bootInfo);
    OrynKernelRuntimeRunBootSequence(kernelBootInfo);
    OrynKernelRuntimeExitForNonInteractiveVm();
    OrynKernelRuntimeHaltForever();
}
