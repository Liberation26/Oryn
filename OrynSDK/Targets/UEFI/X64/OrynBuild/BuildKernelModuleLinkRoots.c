#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

static int TokenEqualsBuild(const char* token, const char* expected)
{
    return strcmp(token, expected) == 0;
}

static int BootSelectionFlagEnabled(const OrynProject* project, const char* define_name)
{
    char header_path[ORYN_MAX_PATH];
    int value = 0;
    OrynJoinPath(header_path, sizeof(header_path), project->selected_kernel_include_dir, "OrynBootInfoSelection.h");
    if (!ReadHeaderDefineInt(header_path, define_name, &value))
    {
        return 0;
    }

    return value != 0;
}

static int SelectionTokenEnabled(const OrynProject* project, const char* token)
{
    if (TokenEqualsBuild(token, "Always")) return 1;
    if (TokenEqualsBuild(token, "VmPic")) return ProjectBoolEnabledBuild(project->run_pic, 1);
    if (TokenEqualsBuild(token, "VmApic")) return ProjectBoolEnabledBuild(project->run_apic, 1) || ProjectBoolEnabledBuild(project->run_apic2, 1);
    if (TokenEqualsBuild(token, "VmHpet")) return ProjectBoolEnabledBuild(project->run_hpet, 1);
    if (TokenEqualsBuild(token, "VmSmp")) return ProjectCpuCountBuild(project) > 1U;
    if (TokenEqualsBuild(token, "BootInfoFramebuffer")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_FRAMEBUFFER");
    if (TokenEqualsBuild(token, "BootInfoRsdp")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_RSDP");
    if (TokenEqualsBuild(token, "BootInfoMemoryMap")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_MEMORY_MAP");
    if (TokenEqualsBuild(token, "BootInfoKernelRange")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_KERNEL_RANGE");
    if (TokenEqualsBuild(token, "BootInfoFirmwareData")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_FIRMWARE_DATA");
    if (TokenEqualsBuild(token, "BootInfoPlatformTables")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_PLATFORM_TABLES");
    if (TokenEqualsBuild(token, "BootInfoNvram")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_NVRAM");
    if (TokenEqualsBuild(token, "BootInfoRuntimeServices")) return BootSelectionFlagEnabled(project, "ORYN_BOOTINFO_WANT_RUNTIME_SERVICES");
    return 0;
}

int KernelModuleSelectedForBuild(const OrynProject* project, const OrynKernelModuleManifestSource* module)
{
    if (!module->CompiledIn)
    {
        return 0;
    }

    if (module->Required)
    {
        return 1;
    }

    char buffer[ORYN_MAX_KERNEL_MANIFEST_SELECT_TEXT];
    snprintf(buffer, sizeof(buffer), "%s", module->Select);
    char* save_pointer = 0;
    char* token = strtok_r(buffer, ",", &save_pointer);
    while (token != 0)
    {
        while (*token == ' ' || *token == '\t')
        {
            token += 1;
        }

        char* end = token + strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t'))
        {
            end[-1] = 0;
            end -= 1;
        }

        if (token[0] != 0 && !SelectionTokenEnabled(project, token))
        {
            return 0;
        }
        token = strtok_r(0, ",", &save_pointer);
    }

    return 1;
}

static void ManifestOwnedSourceRoot(const OrynKernelModuleManifestSource* module, char* output, size_t output_size)
{
    char directory[ORYN_MAX_PATH];
    OrynGetDirectoryName(directory, sizeof(directory), module->Path);

    char source_directory[ORYN_MAX_PATH];
    OrynJoinPath(source_directory, sizeof(source_directory), directory, "Source");
    if (OrynDirectoryExists(source_directory))
    {
        snprintf(output, output_size, "%s", source_directory);
        return;
    }

    snprintf(output, output_size, "%s", directory);
}

static int SourceListContainsRoot(const OrynStringList* sources, const char* root)
{
    size_t root_length = strlen(root);
    for (int index = 0; index < sources->count; ++index)
    {
        if (strncmp(sources->items[index], root, root_length) == 0 &&
            (sources->items[index][root_length] == '/' || sources->items[index][root_length] == 0))
        {
            return 1;
        }
    }

    return 0;
}

static int CountCSourceMaterial(const OrynProject* project, const char* module_name, const char* root)
{
    OrynStringList sources;
    if (!CollectCFilesFromDirectoryMode(project, module_name, root, 1, &sources))
    {
        return -1;
    }

    return sources.count;
}

int ValidateSelectedKernelModuleSourceRoots(const OrynProject* project, const OrynStringList* resolved_sources)
{
    OrynKernelModuleManifestSource modules[ORYN_MAX_KERNEL_MODULE_MANIFESTS];
    int module_count = 0;
    int selected_count = 0;
    if (!ReadKernelModuleManifestSources(project, modules, &module_count))
    {
        return 0;
    }

    for (int index = 0; index < module_count; ++index)
    {
        if (!KernelModuleSelectedForBuild(project, &modules[index]))
        {
            continue;
        }

        selected_count += 1;
        char source_root[ORYN_MAX_PATH];
        ManifestOwnedSourceRoot(&modules[index], source_root, sizeof(source_root));
        int source_count = CountCSourceMaterial(project, modules[index].Name, source_root);
        if (source_count <= 0)
        {
            char message[ORYN_MAX_PATH + 192];
            snprintf(message, sizeof(message), "Selected kernel module %s has no owned C source material at %s.", modules[index].Name, source_root);
            OrynLogFail(message);
            LogBuildPlanSkip(project, "selected-module-source-missing", message);
            return 0;
        }

        if (!SourceListContainsRoot(resolved_sources, source_root))
        {
            char message[ORYN_MAX_PATH + 192];
            snprintf(message, sizeof(message), "Selected kernel module %s owns source root not present in resolved archive plan: %s.", modules[index].Name, source_root);
            OrynLogFail(message);
            LogBuildPlanSkip(project, "selected-module-not-in-link-plan", message);
            return 0;
        }

        char detail[ORYN_MAX_PATH + 192];
        snprintf(detail, sizeof(detail), "module=%s source-root=%s source-count=%d", modules[index].Name, source_root, source_count);
        LogBuildPlanDecision(project, "selected-module-source-root", detail);
    }

    char message[160];
    snprintf(message, sizeof(message), "Selected kernel module source roots validated: %d module(s).", selected_count);
    OrynLogOk(message);
    LogBuildPlanDecision(project, "selected-module-source-validation", message);
    return selected_count > 0;
}

void AppendSelectedKernelModuleLinkRootFlags(const OrynProject* project, char* command, size_t command_size)
{
    OrynKernelModuleManifestSource modules[ORYN_MAX_KERNEL_MODULE_MANIFESTS];
    int module_count = 0;
    if (!ReadKernelModuleManifestSources(project, modules, &module_count))
    {
        return;
    }

    for (int index = 0; index < module_count; ++index)
    {
        if (!KernelModuleSelectedForBuild(project, &modules[index]))
        {
            continue;
        }

        char flag[256];
        snprintf(flag, sizeof(flag), " --undefined=OrynKernelModuleLinkRoot_%s", modules[index].Id);
        strncat(command, flag, command_size - strlen(command) - 1U);

        char detail[384];
        snprintf(detail, sizeof(detail), "module=%s root=OrynKernelModuleLinkRoot_%s", modules[index].Name, modules[index].Id);
        LogBuildPlanDecision(project, "selected-module-link-root", detail);
    }
}
