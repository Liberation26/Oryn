#include "KernelDiagnosticsProofsInternal.h"

static void OrynKernelDiagnosticsRunEarlySmpProof(const OrynBootInfo* kernelBootInfo)
{
#if ORYN_VM_SMP_CPUS > 1 && (ORYN_VM_APIC || ORYN_VM_APIC2)
    KernelIoWriteString("[KERNEL] SMP: starting early after APIC/APIC2 enable.\n");
    (void)OrynKernelSmpInit(kernelBootInfo);
    OrynKernelSmpPrintProof();
#else
    (void)kernelBootInfo;
    KernelIoWriteString("[KERNEL] INFO: SMP AP startup skipped by VMSettings.\n");
#endif
}

static void OrynKernelDiagnosticsRunPicProofs(void)
{
#if ORYN_VM_PIC
    (void)OrynKernelPicInitAndDisable();
    OrynKernelPicPrintProof();
    (void)OrynKernelInterruptsRunPicTimerProof();
    OrynKernelInterruptsPrintPicRuntimeProof();
    OrynKernelPicPrintProof();
#else
    KernelIoWriteString("[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.\n");
#endif
}

static void OrynKernelDiagnosticsRunApicProofs(const OrynBootInfo* kernelBootInfo)
{
#if ORYN_VM_APIC || ORYN_VM_APIC2
#if ORYN_VM_APIC2
    (void)OrynKernelApicInit(1);
#else
    KernelIoWriteString("[KERNEL] INFO: APIC2/x2APIC disabled by VMSettings.\n");
    (void)OrynKernelApicInit(0);
#endif
    OrynKernelApicPrintProof();
#else
    KernelIoWriteString("[KERNEL] INFO: APIC proofs skipped by VMSettings.\n");
#endif

#if ORYN_VM_SMP_CPUS > 1 && (ORYN_VM_APIC || ORYN_VM_APIC2)
    OrynKernelDiagnosticsRunEarlySmpProof(kernelBootInfo);
#else
    (void)kernelBootInfo;
    KernelIoWriteString("[KERNEL] INFO: SMP AP startup skipped by VMSettings.\n");
#endif
}

static void OrynKernelDiagnosticsRunHpetProofs(const OrynBootInfo* kernelBootInfo)
{
#if ORYN_VM_HPET
    (void)OrynKernelHpetInit(kernelBootInfo);
    OrynKernelHpetPrintProof();
#else
    (void)kernelBootInfo;
    KernelIoWriteString("[KERNEL] INFO: HPET proof skipped by VMSettings.\n");
#endif
}

static void OrynKernelDiagnosticsRunActiveTimerProof(void)
{
#if ORYN_VM_APIC || ORYN_VM_APIC2
    (void)OrynKernelInterruptsRunApicTimerProof();
#if ORYN_VM_PIC
    OrynKernelInterruptsPrintRuntimeProof();
#else
    OrynKernelInterruptsPrintApicRuntimeProof();
#endif
#else
    KernelIoWriteString("[KERNEL] PASS: VMSettings interrupt/timer profile applied.\n");
#endif

#if (ORYN_VM_APIC || ORYN_VM_APIC2) && (!ORYN_VM_PIC || !ORYN_VM_HPET || !ORYN_VM_APIC2 || !ORYN_VM_APIC)
    KernelIoWriteString("[KERNEL] PASS: VMSettings interrupt/timer profile applied.\n");
#endif
}

void OrynKernelDiagnosticsRunInterruptTimerProofs(const OrynBootInfo* kernelBootInfo)
{
    OrynKernelCpuDetect();
    OrynKernelCpuPrintFeatures();
    OrynKernelDiagnosticsRunPicProofs();
    OrynKernelDiagnosticsRunApicProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunHpetProofs(kernelBootInfo);
    OrynKernelDiagnosticsRunActiveTimerProof();
}
