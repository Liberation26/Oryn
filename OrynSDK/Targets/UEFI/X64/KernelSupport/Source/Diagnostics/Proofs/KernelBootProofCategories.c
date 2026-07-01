#include "KernelBootProof.h"
#include "KernelDiagnosticsProofsInternal.h"

static void OrynKernelBootProofRunAlwaysChecks(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelDiagnosticsPrintEntryProofs(kernelBootInfo);
    OrynKernelModuleManifestPrintProof();
    OrynKernelDiagnosticsRunLibCProof();
    OrynKernelDiagnosticsPrintBootOptionPlan(kernelBootInfo);
}

static void OrynKernelBootProofRunDescriptorChecks(void)
{
    OrynKernelDiagnosticsRunDescriptorProofs();
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleDescriptorsReady);
}

static void OrynKernelBootProofRunInterruptTimerChecks(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelDiagnosticsRunInterruptTimerProofs(kernelBootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleInterruptsReady);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleTimersReady);
}

static void OrynKernelBootProofRunOptionalPciCheck(const OrynBootInfo* kernelBootInfo)
{
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModulePci))
    {
        OrynKernelDiagnosticsRunPciProof(kernelBootInfo);
    }
}

static void OrynKernelBootProofRunValidBootInfoChecks(const OrynBootInfo* kernelBootInfo)
{
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleConsole))
    {
        OrynKernelDiagnosticsRunConsoleProofs(kernelBootInfo);
    }

    KernelBootInfoPrintSummary(kernelBootInfo);

    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleFat32))
    {
        OrynKernelDiagnosticsRunFat32VfsProof(kernelBootInfo);
    }

    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModulePhysicalMemory))
    {
        OrynKernelDiagnosticsRunMemoryProofs(kernelBootInfo);
    }
}

void OrynKernelBootProofRunCategoryChecks(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelBootProofRunAlwaysChecks(kernelBootInfo);

    OrynKernelBootInfoStatus bootStatus = KernelBootInfoValidate(kernelBootInfo);

    OrynKernelBootProofRunDescriptorChecks();
    OrynKernelBootProofRunInterruptTimerChecks(kernelBootInfo);
    OrynKernelBootProofRunOptionalPciCheck(kernelBootInfo);

    if (bootStatus.IsValid)
    {
        OrynKernelBootProofRunValidBootInfoChecks(kernelBootInfo);
        return;
    }

    OrynKernelDiagnosticsRunBootInfoFailureProof();
}
