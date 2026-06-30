#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

void LogSelectedBootInfoVariant(const OrynProject* project)
{
    if (project->selected_kernel_number > 0)
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Using BootInfo variant Kernel/%d", project->selected_kernel_number);
        OrynLogOk(message);
    }
    else
    {
        OrynLogInfo("Using SDK default BootInfo selection because no Kernel/<number> variant is selected.");
    }
}

int CollectKernelSources(const OrynProject* project, OrynStringList* sources)
{
    sources->count = 0;
    return AppendSourcesFromDirectory(project, project->sdk_kernel_common_source_dir, sources, "SDK common kernel") &&
        AppendSourcesFromDirectory(project, project->sdk_kernel_target_source_dir, sources, "SDK target kernel") &&
        AppendSourcesFromDirectory(project, project->source_dir, sources, "project kernel");
}

int CompileKernelSources(
    const OrynProject* project,
    const OrynStringList* sources,
    OrynStringList* objects,
    OrynBuildObjectStats* stats)
{
    objects->count = 0;
    stats->SourceCount = sources->count;
    stats->CompiledCount = 0;
    stats->ReusedCount = 0;
    stats->StaleRemovedCount = 0;

    for (int index = 0; index < sources->count; ++index)
    {
        int was_compiled = 0;
        if (!CompileSourceFile(project, sources->items[index], objects->items[objects->count], ORYN_MAX_PATH, &was_compiled))
        {
            OrynLogFail("Source compile failed.");
            return 0;
        }

        objects->count += 1;
        if (was_compiled)
        {
            stats->CompiledCount += 1;
        }
        else
        {
            stats->ReusedCount += 1;
        }
    }

    stats->StaleRemovedCount = RemoveStaleObjectFiles(project, objects);
    WriteObjectManifest(project, sources, objects, stats);
    return 1;
}

void BuildKernelLinkCommand(const OrynProject* project, const OrynStringList* objects, char* command, size_t command_size)
{
    char linker_script[ORYN_MAX_PATH];
    OrynJoinPath(linker_script, sizeof(linker_script), project->sdk_root, "Targets/UEFI/X64/Kernel.ld");

    char kernel_file_name[256];
    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);

    char kernel_elf[ORYN_MAX_PATH];
    OrynJoinPath(kernel_elf, sizeof(kernel_elf), project->build_dir, kernel_file_name);

    snprintf(command, command_size,
        "ld.lld -nostdlib -static -z max-page-size=0x1000 "
        "--defsym=ORYN_KERNEL_PHYSICAL_BASE=0x%llX "
        "--defsym=ORYN_KERNEL_VIRTUAL_BASE=0x%llX "
        "-T \"%s\" -o \"%s\"",
        project->kernel_physical_base,
        project->kernel_virtual_base,
        linker_script,
        kernel_elf);

    for (int index = 0; index < objects->count; ++index)
    {
        strncat(command, " \"", command_size - strlen(command) - 1U);
        strncat(command, objects->items[index], command_size - strlen(command) - 1U);
        strncat(command, "\"", command_size - strlen(command) - 1U);
    }
}

void LogIncrementalSummary(const OrynBuildObjectStats* stats)
{
    char message[256];
    snprintf(message, sizeof(message),
        "Incremental C-to-object: %d rebuilt, %d reused, %d stale removed.",
        stats->CompiledCount,
        stats->ReusedCount,
        stats->StaleRemovedCount);
    OrynLogOk(message);
}

int LinkKernelObjects(const OrynProject* project, const OrynStringList* objects)
{
    char command[ORYN_MAX_PATH * 8];
    BuildKernelLinkCommand(project, objects, command, sizeof(command));
    if (!OrynRunCommand(command))
    {
        OrynLogFail("Kernel link failed.");
        return 0;
    }

    char kernel_file_name[256];
    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);

    char link_message[ORYN_MAX_PATH + 64];
    snprintf(link_message, sizeof(link_message), "Linked Build/%s", kernel_file_name);
    OrynLogOk(link_message);
    return 1;
}

void LogKernelLayout(const OrynProject* project)
{
    char layout_message[256];
    snprintf(layout_message, sizeof(layout_message), "Kernel physical load base: 0x%llX", project->kernel_physical_base);
    OrynLogOk(layout_message);

    snprintf(layout_message, sizeof(layout_message), "Kernel chosen virtual base: 0x%llX", project->kernel_virtual_base);
    OrynLogOk(layout_message);
}

int OrynBuildKernel(const OrynProject* project)
{
    OrynLogStep("Building kernel.");
    LogSelectedBootInfoVariant(project);
    OrynMakeDirectoryRecursive(project->object_dir);

    OrynStringList sources;
    if (!CollectKernelSources(project, &sources))
    {
        return 0;
    }

    if (sources.count == 0)
    {
        OrynLogFail("No kernel source files were found.");
        return 0;
    }

    char message[256];
    snprintf(message, sizeof(message), "Kernel source units: %d", sources.count);
    OrynLogInfo(message);

    OrynStringList objects;
    OrynBuildObjectStats stats;
    if (!CompileKernelSources(project, &sources, &objects, &stats))
    {
        return 0;
    }

    LogIncrementalSummary(&stats);
    if (!LinkKernelObjects(project, &objects))
    {
        return 0;
    }

    LogKernelLayout(project);
    return 1;
}
