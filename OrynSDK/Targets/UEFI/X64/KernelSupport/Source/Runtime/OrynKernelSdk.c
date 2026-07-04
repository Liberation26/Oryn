#include "OrynKernelSdk.h"

#include "KernelBootProof.h"
#include "KernelModuleManifest.h"
#include "KernelRuntime.h"
#include "KernelScreenReport.h"

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

    OrynKernelScreenReportOkOrFail(
        touched > 0U && missing == 0U,
        "SDK selected module link roots are present.",
        "SDK selected module link root check failed.");
}

static void VerifySelectedSdkModulesPresent(OrynKernelSdkContext* kernel)
{
    unsigned int selectedRoots = OrynKernelSelectedModuleLinkRootCount();
    unsigned int missingRoots = OrynKernelSelectedModuleMissingLinkRootCount();

    kernel->SelectedModuleCount = selectedRoots;
    kernel->MissingModuleLinkRootCount = missingRoots;

    OrynKernelScreenReportOkOrFail(
        selectedRoots > 0U && missingRoots == 0U,
        "SDK selected modules are physically linked.",
        "SDK selected modules are missing link roots.");
}

void OrynKernelSdkReportOk(OrynKernelSdkContext* kernel, const char* message)
{
    const char* category = "SDK Kernel";
    if (kernel != 0 && kernel->KernelName != 0)
    {
        category = kernel->KernelName;
    }

    OrynKernelScreenReportOk(category, message);
}

void OrynKernelSdkReportFail(OrynKernelSdkContext* kernel, const char* message)
{
    const char* category = "SDK Kernel";
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

    OrynKernelBootProofRunSequence(kernel->BootInfo);
}

void OrynKernelSdkHalt(OrynKernelSdkContext* kernel)
{
    (void)kernel;
    OrynKernelRuntimeExitForNonInteractiveVm();
    OrynKernelRuntimeHaltForever();
}

void OrynKernelSdkStart(
    const OrynBootInfo* bootInfo,
    const OrynKernelSdkApplication* application)
{
    OrynKernelSdkContext kernel;
    kernel.BootInfo = OrynKernelRuntimeEnter(bootInfo);
    kernel.KernelName = application != 0 ? application->KernelName : "SDK Kernel";
    kernel.SelectedModuleCount = 0U;
    kernel.MissingModuleLinkRootCount = 0U;

    TouchSelectedSdkModuleRoots(&kernel);
    VerifySelectedSdkModulesPresent(&kernel);
    OrynKernelSdkReportOk(&kernel, "Kernel entry is running through the public Oryn SDK application API.");

    if (application != 0 && application->Main != 0)
    {
        application->Main(&kernel);
    }
    else
    {
        OrynKernelSdkReportFail(&kernel, "SDK kernel application has no Main function.");
    }

    OrynKernelSdkHalt(&kernel);
}
