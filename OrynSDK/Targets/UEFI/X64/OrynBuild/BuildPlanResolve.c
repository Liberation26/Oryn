#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

static int FindModuleIndex(const OrynBuildArchivePlan* plan, const char* name)
{
    for (int index = 0; index < plan->ModuleCount; ++index)
    {
        if (strcmp(plan->Modules[index].Name, name) == 0)
        {
            return index;
        }
    }

    return -1;
}

static int ResolveModuleIndex(const OrynProject* project, OrynBuildArchivePlan* plan, int module_index)
{
    OrynBuildModule* module = &plan->Modules[module_index];
    if (module->Resolved)
    {
        LogBuildPlanDecision(project, "module-already-resolved", module->Name);
        return 1;
    }

    if (module->Visiting)
    {
        char message[256];
        snprintf(message, sizeof(message), "Build module dependency cycle at %s.", module->Name);
        LogBuildPlanSkip(project, "dependency-cycle", message);
        OrynLogFail(message);
        return 0;
    }

    LogBuildPlanDecision(project, "module-resolve-enter", module->Name);
    module->Visiting = 1;
    for (int index = 0; index < module->RequireCount; ++index)
    {
        char detail[384];
        snprintf(detail, sizeof(detail), "module=%s requires=%s", module->Name, module->Requires[index]);
        LogBuildPlanDecision(project, "requirement-check", detail);

        int require_index = FindModuleIndex(plan, module->Requires[index]);
        if (require_index < 0)
        {
            char message[256];
            snprintf(message, sizeof(message), "Build module %s requires missing module %s.", module->Name, module->Requires[index]);
            LogBuildPlanSkip(project, "missing-required-module", message);
            OrynLogFail(message);
            return 0;
        }

        snprintf(detail, sizeof(detail), "module=%s requires=%s resolved-index=%d", module->Name, module->Requires[index], require_index);
        LogBuildPlanDecision(project, "requirement-found", detail);
        if (!ResolveModuleIndex(project, plan, require_index))
        {
            return 0;
        }
    }

    module->Visiting = 0;
    module->Resolved = 1;
    plan->ResolvedOrder[plan->ResolvedCount] = module_index;

    char detail[384];
    snprintf(detail, sizeof(detail), "order=%d module=%s archive=%s", plan->ResolvedCount, module->Name, module->ArchivePath);
    LogBuildPlanDecision(project, "module-resolved-order", detail);
    plan->ResolvedCount += 1;
    return 1;
}

int ResolveKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    plan->ResolvedCount = 0;
    for (int index = 0; index < plan->ModuleCount; ++index)
    {
        char detail[256];
        snprintf(detail, sizeof(detail), "root-index=%d module=%s", index, plan->Modules[index].Name);
        LogBuildPlanDecision(project, "resolve-root-module", detail);
        if (!ResolveModuleIndex(project, plan, index))
        {
            return 0;
        }
    }

    char message[128];
    snprintf(message, sizeof(message), "Resolved build module graph: %d module archive(s).", plan->ResolvedCount);
    LogBuildPlanDecision(project, "module-graph-resolved", message);
    OrynLogOk(message);
    return 1;
}
