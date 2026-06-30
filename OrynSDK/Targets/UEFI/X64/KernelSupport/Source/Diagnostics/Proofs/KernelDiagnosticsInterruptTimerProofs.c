#include "KernelDiagnosticsProofsInternal.h"
#include "KernelScreenReport.h"

static void OrynKernelDiagnosticsRunEarlySmpProof(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleSmp))
    {
        return;
    }

    KernelIoWriteString("[KERNEL] SMP: starting early after APIC/APIC2 enable.\n");
    (void)OrynKernelSmpInit(kernelBootInfo);
    OrynKernelSmpPrintProof();
}

static void OrynKernelDiagnosticsRunPicProofs(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModulePic))
    {
        return;
    }

    (void)OrynKernelPicInitAndDisable();
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
    (void)OrynKernelApicInit(1);
#else
    KernelIoWriteString("[KERNEL] INFO: APIC2/x2APIC disabled by VM profile.\n");
    (void)OrynKernelApicInit(0);
#endif
    OrynKernelApicPrintProof();
    OrynKernelDiagnosticsRunEarlySmpProof(kernelBootInfo);
}

static void OrynKernelDiagnosticsRunHpetProofs(const OrynBootInfo* kernelBootInfo)
{
    if (!OrynKernelDiagnosticsShouldStartModule(kernelBootInfo, OrynKernelModuleHpet))
    {
        return;
    }

    (void)OrynKernelHpetInit(kernelBootInfo);
    OrynKernelHpetPrintProof();
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
    OrynKernelCpuDetect();
    OrynKernelCpuPrintFeatures();
    OrynKernelDiagnosticsRunPicProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunApicProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunHpetProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunActiveTimerProof();
}
