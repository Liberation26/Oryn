#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "KernelLifecycle.h"
#include "KernelPanic.h"
#include "KernelRuntimeInternal.h"
#include "KernelScreenReport.h"

static void OrynKernelRuntimeDisableInterrupts(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

const OrynBootInfo* OrynKernelRuntimeEnter(const OrynBootInfo* bootInfo)
{
    OrynKernelRuntimeDisableInterrupts();
    KernelIoInit();
    OrynKernelScreenReportInit();
    OrynKernelLifecycleInit();
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleEntered);

    const OrynBootInfo* kernelBootInfo = KernelBootInfoAdopt(bootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleBootInfoAdopted);
    OrynKernelPanicInit(kernelBootInfo);
    return kernelBootInfo;
}
