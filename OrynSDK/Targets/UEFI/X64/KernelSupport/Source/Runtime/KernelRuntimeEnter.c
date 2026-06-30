#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "KernelLifecycle.h"
#include "KernelModuleManifest.h"
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
    OrynKernelModuleManifestInit();
    OrynKernelModuleManifestReady(OrynKernelModuleScreenReport);
    OrynKernelLifecycleInit();
    OrynKernelModuleManifestReady(OrynKernelModuleLifecycle);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleEntered);

    const OrynBootInfo* kernelBootInfo = KernelBootInfoAdopt(bootInfo);
    OrynKernelModuleManifestReady(OrynKernelModuleBootInfo);
    (void)OrynKernelLifecycleTransition(OrynKernelLifecycleBootInfoAdopted);
    OrynKernelPanicInit(kernelBootInfo);
    OrynKernelModuleManifestReady(OrynKernelModulePanic);
    return kernelBootInfo;
}
