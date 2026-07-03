#include "KernelBootProof.h"
#include "KernelDiagnostics.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelLifecycle.h"
#include "KernelPanic.h"
#include "KernelModuleManifest.h"
#include "KernelSelfTest.h"

static void OrynKernelBootProofHandleActivePanic(void)
{
    if (!OrynKernelPanicIsActive())
    {
        return;
    }

    OrynKernelDiagnosticsLogHaltMessage();
    OrynKernelPanicHalt();
}

static void OrynKernelBootProofEnterRunningState(void)
{
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleRunning);
    OrynKernelDiagnosticsLogHaltMessage();
}

void OrynKernelBootProofRunSequence(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelSelfTestRunCategoryChecks(kernelBootInfo);
    OrynKernelBootProofHandleActivePanic();
    OrynKernelBootProofEnterRunningState();
    OrynKernelDiagnosticsRunHaltProofs();
}
