#include "KernelDiagnosticsProofsInternal.h"
#include "KernelModuleStartPlan.h"

static void ReportModuleWarn(OrynKernelModuleId id, const char* prefix, const char* reason)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    OrynKernelScreenReportBeginWarn();
    OrynKernelDiagnosticsLogText(prefix);
    OrynKernelDiagnosticsLogText(item ? item->Name : "unknown");
    OrynKernelDiagnosticsLogText(" because ");
    OrynKernelDiagnosticsLogText(reason ? reason : "nothing was selected to start");
    OrynKernelDiagnosticsLogText(".\n");
}

static void ReportModuleFail(OrynKernelModuleId id, const char* prefix, const char* reason)
{
    const OrynKernelModuleManifestItem* item = OrynKernelModuleManifestGet(id);
    OrynKernelScreenReportBeginFail();
    OrynKernelDiagnosticsLogText(prefix);
    OrynKernelDiagnosticsLogText(item ? item->Name : "unknown");
    OrynKernelDiagnosticsLogText(" because ");
    OrynKernelDiagnosticsLogText(reason ? reason : "a required prerequisite is missing");
    OrynKernelDiagnosticsLogText(".\n");
}

static void ReportModuleDecision(OrynKernelModuleId id, OrynKernelModuleStartDecision decision)
{
    if (decision.ShouldStart)
    {
        return;
    }

    if (decision.IsFailure)
    {
        if (!OrynKernelModuleManifestIsCompiledIn(id))
        {
            ReportModuleFail(id, "Required module not compiled in: ", decision.Reason);
            return;
        }
        if (OrynKernelModuleManifestIsRequired(id))
        {
            ReportModuleFail(id, "Required module has no selected boot input: ", decision.Reason);
            return;
        }
        ReportModuleFail(id, "Selected module missing prerequisite: ", decision.Reason);
        return;
    }

    if (decision.IsWarning)
    {
        if (OrynKernelModuleManifestFatalOnMissingPrerequisite(id))
        {
            ReportModuleWarn(id, "Optional module has nothing selected to start: ", decision.Reason);
            return;
        }
        ReportModuleWarn(id, "Selected optional module skipped: ", decision.Reason);
    }
}

int OrynKernelDiagnosticsShouldStartModule(const OrynBootInfo* bootInfo, OrynKernelModuleId id)
{
    OrynKernelModuleStartDecision decision = OrynKernelModuleStartPlanDecide(bootInfo, id);
    ReportModuleDecision(id, decision);
    return decision.ShouldStart;
}

void OrynKernelDiagnosticsPrintBootOptionPlan(const OrynBootInfo* bootInfo)
{
    (void)bootInfo;
    OrynKernelScreenReportOk(0, "Kernel module start plan is owned outside Diagnostics proof code.");
    OrynKernelScreenReportOk(0, "Runtime start decisions come from BootInfo and project-selected manifest tokens.");
    OrynKernelScreenReportOk(0, "Selected modules use manifest fatal/non-fatal prerequisite policy.");
}
