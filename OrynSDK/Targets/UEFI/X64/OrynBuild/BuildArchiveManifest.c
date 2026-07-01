#include "TargetBuildInternal.h"
#include <stdio.h>

static void BuildArchiveManifestPath(const OrynProject* project, char* output, size_t output_size)
{
    char archive_dir[ORYN_MAX_PATH];
    BuildArchiveDirectory(project, archive_dir, sizeof(archive_dir));
    OrynJoinPath(output, output_size, archive_dir, "ArchiveManifest.txt");
}

static void WriteModuleRequires(FILE* file, const OrynBuildModule* module)
{
    fprintf(file, "Requires=");
    for (int index = 0; index < module->RequireCount; ++index)
    {
        fprintf(file, "%s%s", index == 0 ? "" : ",", module->Requires[index]);
    }
    fprintf(file, "\n");
}

void WriteArchiveManifest(const OrynProject* project, const OrynBuildArchivePlan* plan, const OrynBuildObjectStats* stats)
{
    char manifest_path[ORYN_MAX_PATH];
    BuildArchiveManifestPath(project, manifest_path, sizeof(manifest_path));
    FILE* file = fopen(manifest_path, "wb");
    if (file == 0)
    {
        OrynLogWarn("Could not write module/archive build manifest.");
        return;
    }

    fprintf(file, "Oryn module/archive resolver manifest\n");
    fprintf(file, "Version=%s\n", ORYN_VERSION);
    fprintf(file, "Modules=%d\n", plan->ResolvedCount);
    fprintf(file, "Sources=%d\n", stats->SourceCount);
    fprintf(file, "Compiled=%d\n", stats->CompiledCount);
    fprintf(file, "Reused=%d\n", stats->ReusedCount);
    char diagnostics_path[ORYN_MAX_PATH];
    BuildPlanDiagnosticsPath(project, diagnostics_path, sizeof(diagnostics_path));
    fprintf(file, "StaleRemoved=%d\n", stats->StaleRemovedCount);
    fprintf(file, "Diagnostics=%s\n\n", diagnostics_path);

    for (int order = 0; order < plan->ResolvedCount; ++order)
    {
        const OrynBuildModule* module = &plan->Modules[plan->ResolvedOrder[order]];
        fprintf(file, "Module=%s\n", module->Name);
        WriteModuleRequires(file, module);
        fprintf(file, "SourceRoot=%s\n", module->SourceRoot);
        fprintf(file, "Archive=%s\n", module->ArchivePath);
        fprintf(file, "SourceCount=%d\n", module->Sources->count);
        for (int source = 0; source < module->Sources->count; ++source)
        {
            fprintf(file, "Source[%d]=%s\n", source, module->Sources->items[source]);
        }
        fprintf(file, "ObjectCount=%d\n", module->Objects->count);
        for (int object = 0; object < module->Objects->count; ++object)
        {
            fprintf(file, "Object[%d]=%s\n", object, module->Objects->items[object]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}
