#include "KernelBootProof.h"
#include "KernelDiagnostics.h"
#include "KernelIo.h"
#include "KernelLifecycle.h"
#include "KernelPanic.h"

static void OrynKernelBootProofHandleActivePanic(void)
{
    if (!OrynKernelPanicIsActive())
    {
        return;
    }

    KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
    OrynKernelPanicHalt();
}

static void OrynKernelBootProofEnterRunningState(void)
{
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleRunning);
    KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
}

void OrynKernelBootProofRunSequence(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelBootProofRunCategoryChecks(kernelBootInfo);
    OrynKernelBootProofHandleActivePanic();
    OrynKernelBootProofEnterRunningState();
    OrynKernelDiagnosticsRunHaltProofs();
}
