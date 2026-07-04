#include "OrynKernelSdk.h"

#include "KernelBootProof.h"
#include "KernelIo.h"
#include "KernelModuleManifest.h"
#include "KernelRuntime.h"
#include "KernelRuntimeInternal.h"
#include "KernelScreenReport.h"

#define QEMU_EXIT_PORT 0xF4

static inline void OrynKernelSdkOut32(unsigned short port, unsigned int value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static void OrynKernelSdkDisableInterrupts(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

static void OrynKernelSdkHaltLoop(void)
{
    for (;;)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

static void OrynKernelSdkEnsureRuntimeEntered(OrynKernelSdkContext* kernel)
{
    if (kernel == 0 || kernel->RuntimeEntered != 0U)
    {
        return;
    }

    kernel->BootInfo = OrynKernelRuntimeEnter(kernel->BootInfo);
    kernel->RuntimeEntered = 1U;
}

static void TouchSelectedSdkModuleRoots(OrynKernelSdkContext* kernel)
{
    unsigned int index = 0U;
    unsigned int touched = 0U;
    unsigned int missing = 0U;

    for (index = 0U; index < OrynKernelCompiledModuleCount(); ++index)
    {
        const OrynKernelCompiledModuleRecord* module = OrynKernelCompiledModuleGet(index);

        if (module == 0 || !module->SelectedInBuild)
        {
            continue;
        }

        OrynKernelModuleManifestSelected(module->Id);

        if (module->LinkRoot == 0)
        {
            ++missing;
            continue;
        }

        module->LinkRoot();
        ++touched;
    }

    kernel->SelectedModuleCount = touched;
    kernel->MissingModuleLinkRootCount = missing;
}

static void VerifySelectedSdkModulesPresent(OrynKernelSdkContext* kernel)
{
    kernel->SelectedModuleCount = OrynKernelSelectedModuleLinkRootCount();
    kernel->MissingModuleLinkRootCount = OrynKernelSelectedModuleMissingLinkRootCount();
}

void OrynKernelSdkWrite(OrynKernelSdkContext* kernel, const char* text)
{
    (void)kernel;
    if (text != 0)
    {
        KernelIoWriteString(text);
    }
}

void OrynKernelSdkWriteLine(OrynKernelSdkContext* kernel, const char* text)
{
    OrynKernelSdkWrite(kernel, text);
    OrynKernelSdkWrite(kernel, "\n");
}

void OrynKernelSdkReportOk(OrynKernelSdkContext* kernel, const char* message)
{
    const char* category = "SDK Kernel";
    OrynKernelSdkEnsureRuntimeEntered(kernel);
    if (kernel != 0 && kernel->KernelName != 0)
    {
        category = kernel->KernelName;
    }

    OrynKernelScreenReportOk(category, message);
}

void OrynKernelSdkReportFail(OrynKernelSdkContext* kernel, const char* message)
{
    const char* category = "SDK Kernel";
    OrynKernelSdkEnsureRuntimeEntered(kernel);
    if (kernel != 0 && kernel->KernelName != 0)
    {
        category = kernel->KernelName;
    }

    OrynKernelScreenReportFail(category, message);
}

void OrynKernelSdkRunBootProof(OrynKernelSdkContext* kernel)
{
    if (kernel == 0 || kernel->BootInfo == 0)
    {
        OrynKernelSdkReportFail(kernel, "SDK boot proof cannot run without BootInfo.");
        return;
    }

    OrynKernelSdkEnsureRuntimeEntered(kernel);
    TouchSelectedSdkModuleRoots(kernel);
    VerifySelectedSdkModulesPresent(kernel);
    OrynKernelBootProofRunSequence(kernel->BootInfo);
}

void OrynKernelSdkHalt(OrynKernelSdkContext* kernel)
{
    (void)kernel;
#if !ORYN_VM_INTERACTIVE_DISPLAY
    OrynKernelSdkOut32(QEMU_EXIT_PORT, 0x10U);
#endif
    OrynKernelSdkHaltLoop();
}

void OrynKernelSdkStart(
    const OrynBootInfo* bootInfo,
    const OrynKernelSdkApplication* application)
{
    OrynKernelSdkContext kernel;
    OrynKernelSdkDisableInterrupts();
    KernelIoInit();

    kernel.BootInfo = bootInfo;
    kernel.KernelName = application != 0 ? application->KernelName : "SDK Kernel";
    kernel.SelectedModuleCount = 0U;
    kernel.MissingModuleLinkRootCount = 0U;
    kernel.RuntimeEntered = 0U;

    if (application != 0 && application->Main != 0)
    {
        application->Main(&kernel);
    }

    OrynKernelSdkHalt(&kernel);
}
