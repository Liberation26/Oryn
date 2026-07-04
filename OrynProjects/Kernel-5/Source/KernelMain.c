#include "KernelBootProof.h"
#include "KernelModuleManifest.h"
#include "KernelRuntime.h"
#include "KernelScreenReport.h"

static void Kernel5TouchSelectedSdkModuleRoots(void)
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

    OrynKernelScreenReportOkOrFail(
        touched > 0U && missing == 0U,
        "Project kernel touched every selected SDK module link root.",
        "Project kernel selected SDK module link root check failed.");
}

static void Kernel5VerifySelectedSdkModulesPresent(void)
{
    unsigned int selectedRoots = OrynKernelSelectedModuleLinkRootCount();
    unsigned int missingRoots = OrynKernelSelectedModuleMissingLinkRootCount();

    OrynKernelScreenReportOkOrFail(
        selectedRoots > 0U && missingRoots == 0U,
        "Project kernel selected SDK modules are physically linked.",
        "Project kernel is missing selected SDK module link roots.");
}

static void Kernel5RunSdkAuthoredKernel(const OrynBootInfo* bootInfo)
{
    const OrynBootInfo* kernelBootInfo = OrynKernelRuntimeEnter(bootInfo);

    Kernel5TouchSelectedSdkModuleRoots();
    Kernel5VerifySelectedSdkModulesPresent();
    OrynKernelBootProofRunSequence(kernelBootInfo);
    OrynKernelRuntimeExitForNonInteractiveVm();
    OrynKernelRuntimeHaltForever();
}

void KernelStart(const OrynBootInfo* bootInfo)
{
    Kernel5RunSdkAuthoredKernel(bootInfo);
}
