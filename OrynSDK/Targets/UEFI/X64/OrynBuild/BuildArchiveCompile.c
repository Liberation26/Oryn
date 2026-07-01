#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

static int AppendAggregate(OrynStringList* target, const OrynStringList* source)
{
    for (int index = 0; index < source->count; ++index)
    {
        if (target->count >= ORYN_MAX_ITEMS)
        {
            OrynLogFail("Too many resolved source/object entries for the build manifest.");
            return 0;
        }
        snprintf(target->items[target->count], ORYN_MAX_PATH, "%s", source->items[index]);
        target->count += 1;
    }

    return 1;
}

static int CompileModuleSources(const OrynProject* project, OrynBuildModule* module, OrynBuildObjectStats* stats)
{
    module->Objects->count = 0;
    for (int index = 0; index < module->Sources->count; ++index)
    {
        int was_compiled = 0;
        char object_file[ORYN_MAX_PATH];
        if (!CompileSourceFile(project, module->Sources->items[index], object_file, sizeof(object_file), &was_compiled))
        {
            return 0;
        }

        snprintf(module->Objects->items[module->Objects->count], ORYN_MAX_PATH, "%s", object_file);
        module->Objects->count += 1;
        stats->CompiledCount += was_compiled ? 1 : 0;
        stats->ReusedCount += was_compiled ? 0 : 1;
    }

    return ArchiveKernelModule(project, module);
}

int CompileKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan, OrynStringList* all_sources, OrynStringList* all_objects, OrynBuildObjectStats* stats)
{
    memset(stats, 0, sizeof(*stats));
    all_sources->count = 0;
    all_objects->count = 0;
    char archive_dir[ORYN_MAX_PATH];
    BuildArchiveDirectory(project, archive_dir, sizeof(archive_dir));
    OrynMakeDirectoryRecursive(archive_dir);

    for (int order = 0; order < plan->ResolvedCount; ++order)
    {
        OrynBuildModule* module = &plan->Modules[plan->ResolvedOrder[order]];
        if (!CollectCFilesFromDirectoryMode(project, module->Name, module->SourceRoot, module->Recursive, module->Sources))
        {
            OrynLogFail("Failed to collect module source files.");
            return 0;
        }

        if (module->Sources->count == 0)
        {
            char detail[ORYN_MAX_PATH + 160];
            snprintf(detail, sizeof(detail), "module=%s source-root=%s", module->Name, module->SourceRoot);
            LogBuildPlanSkip(project, "module-has-no-c-sources", detail);
            continue;
        }

        char detail[ORYN_MAX_PATH + 160];
        snprintf(detail, sizeof(detail), "module=%s source-count=%d", module->Name, module->Sources->count);
        LogBuildPlanDecision(project, "module-source-collection-complete", detail);
        stats->SourceCount += module->Sources->count;
        if (!CompileModuleSources(project, module, stats))
        {
            return 0;
        }

        if (!AppendAggregate(all_sources, module->Sources) || !AppendAggregate(all_objects, module->Objects))
        {
            return 0;
        }
    }

    stats->StaleRemovedCount = RemoveStaleObjectFiles(project, all_objects);
    return 1;
}
