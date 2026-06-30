#include "Kernel.h"
#include "KernelBootInfo.h"
#include "KernelDiagnostics.h"
#include "KernelIo.h"
#include "KernelLifecycle.h"
#include "KernelPanic.h"
#include "KernelScreenReport.h"

#ifndef ORYN_VM_INTERACTIVE_DISPLAY
#define ORYN_VM_INTERACTIVE_DISPLAY 0
#endif

static void KernelDisableInterrupts(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

static void KernelHaltForever(void)
{
#if ORYN_VM_INTERACTIVE_DISPLAY
    for (;;)
    {
        __asm__ volatile ("sti");
        __asm__ volatile ("hlt");
    }
#else
    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
#endif
}

static const OrynBootInfo* KernelEnter(const OrynBootInfo* bootInfo)
{
    KernelDisableInterrupts();
    KernelIoInit();
    OrynKernelScreenReportInit();
    OrynKernelLifecycleInit();
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleEntered);

    const OrynBootInfo* kernelBootInfo = KernelBootInfoAdopt(bootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleBootInfoAdopted);
    OrynKernelPanicInit(kernelBootInfo);
    return kernelBootInfo;
}

void KernelStart(const OrynBootInfo* bootInfo)
{
    const OrynBootInfo* kernelBootInfo = KernelEnter(bootInfo);
    OrynKernelDiagnosticsRunBootProofs(kernelBootInfo);

    if (OrynKernelPanicIsActive())
    {
        KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
        OrynKernelPanicHalt();
    }

    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleRunning);
    KernelIoWriteString("[KERNEL] System halted by Kernel-5.\n");
    OrynKernelDiagnosticsRunHaltProofs();

#if !ORYN_VM_INTERACTIVE_DISPLAY
    KernelIoExitQemuSuccess();
#endif

    KernelHaltForever();
}
