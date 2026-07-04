#include "KernelDiagnosticsProofsInternal.h"
#include "KernelClock.h"
#include "KernelScreenReport.h"

static void OrynKernelDiagnosticsRunEarlySmpProof(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleSmp))
    {
        return;
    }

    OrynKernelDiagnosticsLogText("[KERNEL] SMP: starting early after APIC/APIC2 enable.\n");
    if (OrynKernelSmpInit(kernelBootInfo))
    {
        OrynKernelModuleManifestReady(OrynKernelModuleSmp);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleSmp);
    }
    OrynKernelSmpPrintProof();
}

static void OrynKernelDiagnosticsRunPicProofs(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModulePic))
    {
        return;
    }

    if (OrynKernelPicInitAndDisable())
    {
        OrynKernelModuleManifestReady(OrynKernelModulePic);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModulePic);
    }
    OrynKernelPicPrintProof();
    (void)OrynKernelInterruptsRunPicTimerProof();
    OrynKernelInterruptsPrintPicRuntimeProof();
    OrynKernelPicPrintProof();
}

static void OrynKernelDiagnosticsRunApicProofs(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleApic))
    {
        return;
    }

#if ORYN_VM_APIC2
    int ok = OrynKernelApicInit(1);
#else
    OrynKernelDiagnosticsLogText("[KERNEL] INFO: APIC2/x2APIC disabled by VM profile.\n");
    int ok = OrynKernelApicInit(0);
#endif
    if (ok)
    {
        OrynKernelModuleManifestReady(OrynKernelModuleApic);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleApic);
    }
    OrynKernelApicPrintProof();
    if (OrynKernelIoApicInit(kernelBootInfo))
    {
        OrynKernelIoApicRouteLegacySet(ORYN_INTERRUPT_IRQ_BASE);
    }
    OrynKernelIoApicPrintProof();
    OrynKernelDiagnosticsRunEarlySmpProof(kernelBootInfo);
}

static void OrynKernelDiagnosticsRunHpetProofs(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleHpet))
    {
        return;
    }

    if (OrynKernelHpetInit(kernelBootInfo))
    {
        OrynKernelModuleManifestReady(OrynKernelModuleHpet);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleHpet);
    }
    OrynKernelHpetPrintProof();
}


static void OrynKernelDiagnosticsRunClockProofs(const OrynBootInfo* kernelBootInfo)
{
    if (OrynKernelClockBootSelect(kernelBootInfo))
    {
        OrynKernelScreenReportOk(0, "Kernel clock boot selection completed.");
    }
    else
    {
        OrynKernelScreenReportFail(0, "Kernel clock boot selection failed.");
    }
    OrynKernelClockPrintProof();
}

static void OrynKernelDiagnosticsRunActiveTimerProof(void)
{
    if (OrynKernelModuleManifestIsReady(OrynKernelModuleApic))
    {
        (void)OrynKernelInterruptsRunApicTimerProof();
#if ORYN_VM_PIC
        OrynKernelInterruptsPrintRuntimeProof();
#else
        OrynKernelInterruptsPrintApicRuntimeProof();
#endif
        return;
    }

    OrynKernelScreenReportOk(0, "Boot option plan skipped APIC timer proof safely.");
}

void OrynKernelDiagnosticsRunInterruptTimerProofs(const OrynBootInfo* kernelBootInfo)
{
    if (OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleCpu))
    {
        OrynKernelCpuDetect();
        OrynKernelCpuPrintFeatures();
        OrynKernelModuleManifestReady(OrynKernelModuleCpu);
    }
    OrynKernelDiagnosticsRunPicProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunApicProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunHpetProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunClockProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunActiveTimerProof();
}
