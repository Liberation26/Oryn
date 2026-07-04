#include "KernelBootProof.h"
#include "KernelDiagnosticsProofsInternal.h"
#include "KernelInitGraph.h"

static void OrynKernelBootProofRunInitGraphCheck(void)
{
    unsigned int index = 0;

    if (!OrynKernelInitGraphValidate())
    {
        OrynKernelScreenReportFail(0, "KernelInit graph dependency validation failed.");
        return;
    }

    OrynKernelScreenReportOk(0, "KernelInit graph has ordered stages and explicit dependencies.");

    for (index = 0; index < OrynKernelInitGraphCount(); ++index)
    {
        const OrynKernelInitStage* stage = OrynKernelInitGraphGet(index);
        unsigned int dependencyIndex = 0;

        if (!stage)
        {
            continue;
        }

        OrynKernelDiagnosticsLogText("[KERNEL] KernelInit stage ");
        OrynKernelDiagnosticsLogDec64(index);
        OrynKernelDiagnosticsLogText(": ");
        OrynKernelDiagnosticsLogText(stage->Name);
        OrynKernelDiagnosticsLogText(" needs ");

        if (stage->DependencyCount == 0U)
        {
            OrynKernelDiagnosticsLogText("none");
        }

        for (dependencyIndex = 0; dependencyIndex < stage->DependencyCount; ++dependencyIndex)
        {
            if (dependencyIndex > 0U)
            {
                OrynKernelDiagnosticsLogText(", ");
            }

            OrynKernelDiagnosticsLogText(OrynKernelInitStageName(stage->Dependencies[dependencyIndex]));
        }

        OrynKernelDiagnosticsLogText(".\n");
    }
}

static void OrynKernelBootProofRunAlwaysChecks(const OrynBootInfo* kernelBootInfo)
{
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleBootInfo))
    {
        OrynKernelDiagnosticsPrintEntryProofs(kernelBootInfo);
        OrynKernelModuleManifestReady(OrynKernelModuleBootInfo);
    }
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleScreenReport))
    {
        OrynKernelModuleManifestReady(OrynKernelModuleScreenReport);
    }
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleLifecycle))
    {
        OrynKernelModuleManifestReady(OrynKernelModuleLifecycle);
    }
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModulePanic))
    {
        OrynKernelModuleManifestReady(OrynKernelModulePanic);
    }
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
    OrynKernelBootProofRunInitGraphCheck();
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
