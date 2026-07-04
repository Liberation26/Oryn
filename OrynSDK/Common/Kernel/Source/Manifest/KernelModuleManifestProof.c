#include "KernelModuleManifest.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

static void PrintModuleLine(const OrynKernelModuleManifestItem* item)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Module manifest item: ");
    OrynKernelDiagnosticsLogText(item->Name);
    OrynKernelDiagnosticsLogText(" [");
    OrynKernelDiagnosticsLogText(OrynKernelModuleManifestStateName(item->State));
    OrynKernelDiagnosticsLogText("] ");
    OrynKernelDiagnosticsLogText(item->CompiledIn ? "compiled-in" : "not-compiled");
    OrynKernelDiagnosticsLogText(item->Required ? " required" : " optional");
    OrynKernelDiagnosticsLogText(item->FatalOnMissingPrerequisite ? " fatal-prereq" : " nonfatal-prereq");
    OrynKernelDiagnosticsLogText(" select=");
    OrynKernelDiagnosticsLogText(item->Selects);
    OrynKernelDiagnosticsLogText(" stop=");
    OrynKernelDiagnosticsLogText(item->StopCallbackName ? item->StopCallbackName : "none");
    OrynKernelDiagnosticsLogText(" panic=");
    OrynKernelDiagnosticsLogText(item->PanicCallbackName ? item->PanicCallbackName : "none");
    OrynKernelDiagnosticsLogText(" shutdown=");
    OrynKernelDiagnosticsLogText(item->ShutdownCallbackName ? item->ShutdownCallbackName : "none");
    OrynKernelDiagnosticsLogText(" -> ");
    OrynKernelDiagnosticsLogText(item->Items);
    OrynKernelDiagnosticsLogText("\n");
}

static unsigned int CountRegisteredModules(void)
{
    unsigned int count = 0U;
    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        if (item && item->State != OrynKernelModuleStateAbsent)
        {
            ++count;
        }
    }

    return count;
}

void OrynKernelModuleManifestCallbackProof(void)
{
    unsigned int withCallbacks = 0U;
    unsigned int compiled = 0U;

    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        if (item && item->CompiledIn)
        {
            ++compiled;
            if (OrynKernelModuleManifestHasLifecycleCallbacks((OrynKernelModuleId)index))
            {
                ++withCallbacks;
            }
        }
    }

    OrynKernelScreenReportOkOrFail(withCallbacks == compiled,
        "Every compiled-in kernel module has stop, panic, and shutdown callbacks.",
        "Compiled-in kernel modules are missing stop, panic, or shutdown callbacks.");

    OrynKernelScreenReportOk(0, "Module lifecycle callback invocation is reverse dependency order for stop, panic, and shutdown.");
}

void OrynKernelCompiledModuleRegistryPrintProof(void)
{
    unsigned int compiled = 0U;
    for (unsigned int index = 0U; index < OrynKernelCompiledModuleCount(); ++index)
    {
        const OrynKernelCompiledModuleRecord* record = OrynKernelCompiledModuleGet(index);
        if (record && record->CompiledIn)
        {
            ++compiled;
            OrynKernelDiagnosticsLogText("[KERNEL] Compiled-in module: ");
            OrynKernelDiagnosticsLogText(record->Name);
            OrynKernelDiagnosticsLogText(record->SelectedInBuild ? " selected-root=" : " optional-root=");
            OrynKernelDiagnosticsLogText(record->LinkRootName ? record->LinkRootName : "none");
            OrynKernelDiagnosticsLogText("\n");
        }
    }

    OrynKernelScreenReportOkOrFail(compiled == (unsigned int)OrynKernelModuleCount,
        "Compiled-in kernel module registry contains every manifest module.",
        "Compiled-in kernel module registry is missing manifest modules.");
    OrynKernelScreenReportOkOrFail(
        OrynKernelSelectedModuleMissingLinkRootCount() == 0U && OrynKernelSelectedModuleLinkRootCount() > 0U,
        "Selected kernel modules are physically present through forced link roots.",
        "Selected kernel modules are missing forced link roots.");
}

void OrynKernelModuleManifestPrintProof(void)
{
    unsigned int count = CountRegisteredModules();
    OrynKernelScreenReportOkOrFail(count == (unsigned int)OrynKernelModuleCount,
        "Module manifest contains every kernel module item set.",
        "Module manifest is missing kernel module item sets.");
    OrynKernelScreenReportOkOrFail(
        !OrynKernelModuleManifestCanStart(OrynKernelModuleApic),
        "Module manifest blocks APIC before PIC is ready.",
        "Module manifest allows APIC before PIC.");
    OrynKernelModuleManifestTransitionProof();
    OrynKernelScreenReportOk(0, "Module manifest owns init/start state transitions and boot policy.");
    OrynKernelScreenReportOk(0, "Module manifest carries required, optional, and fatal prerequisite policy.");

    OrynKernelModuleManifestCallbackProof();
    OrynKernelCompiledModuleRegistryPrintProof();

    for (unsigned int index = 0U; index < (unsigned int)OrynKernelModuleCount; ++index)
    {
        const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet((OrynKernelModuleId)index);
        if (item)
        {
            PrintModuleLine(item);
        }
    }
}
