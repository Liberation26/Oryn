#include "KernelIo.h"
#include "KernelModuleManifest.h"
#include "KernelModuleStartPlan.h"
#include "KernelRuntimeStartup.h"
#include <stdio.h>

static void PrintRuntimeStartupFailure(
    const OrynKernelCompiledModuleRecord* module,
    const char* reason)
{
    printf(
        "[KERNEL] FAIL: Runtime startup module %s failed: %s.\n",
        module && module->Name ? module->Name : "unknown",
        reason ? reason : "unknown reason");
}

static void PrintRuntimeStartupSummary(const OrynKernelRuntimeStartupResult* result)
{
    printf(
        "[KERNEL] OK: Runtime SDK startup selected modules: started=%u skipped=%u failed=%u missing-link-roots=%u.\n",
        result->Started,
        result->Skipped,
        result->Failed,
        result->MissingLinkRoots);
}

static int StartCompiledModule(
    const OrynBootInfo* bootInfo,
    const OrynKernelCompiledModuleRecord* module,
    OrynKernelRuntimeStartupResult* result)
{
    OrynKernelModuleStartDecision decision;

    if (!module || !module->CompiledIn || !module->SelectedInBuild)
    {
        return 1;
    }

    result->Considered += 1U;
    decision = OrynKernelModuleStartPlanDecide(bootInfo, module->Id);
    if (!decision.ShouldStart)
    {
        if (decision.IsFailure)
        {
            result->Failed += 1U;
            PrintRuntimeStartupFailure(module, decision.Reason);
            return 0;
        }

        result->Skipped += 1U;
        return 1;
    }

    if (!module->LinkRoot)
    {
        OrynKernelModuleManifestFailed(module->Id);
        result->Failed += 1U;
        result->MissingLinkRoots += 1U;
        PrintRuntimeStartupFailure(module, "selected module has no link root");
        return 0;
    }

    module->LinkRoot();
    OrynKernelModuleManifestReady(module->Id);
    result->Started += 1U;
    return 1;
}

OrynKernelRuntimeStartupResult OrynKernelRuntimeStartSelectedModules(const OrynBootInfo* bootInfo)
{
    OrynKernelRuntimeStartupResult result;
    unsigned int index;

    result.Considered = 0U;
    result.Started = 0U;
    result.Skipped = 0U;
    result.Failed = 0U;
    result.MissingLinkRoots = 0U;

    OrynKernelModuleManifestInit();

    for (index = 0U; index < OrynKernelCompiledModuleCount(); ++index)
    {
        const OrynKernelCompiledModuleRecord* module = OrynKernelCompiledModuleGet(index);
        StartCompiledModule(bootInfo, module, &result);
    }

    if (OrynKernelRuntimeStartupSucceeded(&result))
    {
        PrintRuntimeStartupSummary(&result);
    }

    return result;
}

int OrynKernelRuntimeStartupSucceeded(const OrynKernelRuntimeStartupResult* result)
{
    return (result && result->Failed == 0U && result->MissingLinkRoots == 0U && result->Started > 0U) ? 1 : 0;
}
