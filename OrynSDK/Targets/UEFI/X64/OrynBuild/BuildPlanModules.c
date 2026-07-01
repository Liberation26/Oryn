#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

#define ORYN_MAX_BUILD_MODULE_MANIFESTS 96

typedef struct OrynBuildModuleManifestFile
{
    char Order[16];
    char Name[128];
    char Root[64];
    char RelativePath[ORYN_MAX_PATH];
    char Recursive[16];
    char Requires[512];
    char SourcePath[ORYN_MAX_PATH];
} OrynBuildModuleManifestFile;

static void StripBuildModuleLine(char* text)
{
    size_t length = strlen(text);
    while (length > 0U && (text[length - 1U] == '\n' || text[length - 1U] == '\r' || text[length - 1U] == ' ' || text[length - 1U] == '\t'))
    {
        text[length - 1U] = 0;
        length -= 1U;
    }

    char* start = text;
    while (*start == ' ' || *start == '\t')
    {
        start += 1;
    }

    if (start != text)
    {
        memmove(text, start, strlen(start) + 1U);
    }
}

static int BuildModuleStartsWith(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int IsSafeBuildModuleName(const char* text)
{
    if (text[0] == 0)
    {
        return 0;
    }

    for (const char* cursor = text; *cursor != 0; ++cursor)
    {
        char value = *cursor;
        if (!((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_' || value == '-'))
        {
            return 0;
        }
    }

    return 1;
}

static int IsSafeBuildModuleRelativePath(const char* text)
{
    if (text[0] == '/' || strstr(text, "..") != 0 || strchr(text, '\\') != 0)
    {
        return 0;
    }

    return 1;
}

static int ParseBuildModuleManifestFile(const char* path, OrynBuildModuleManifestFile* output)
{
    memset(output, 0, sizeof(*output));
    snprintf(output->SourcePath, sizeof(output->SourcePath), "%s", path);
    FILE* file = fopen(path, "r");
    if (!file)
    {
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        StripBuildModuleLine(line);
        if (line[0] == 0 || line[0] == '#')
        {
            continue;
        }

        if (BuildModuleStartsWith(line, "Order=")) snprintf(output->Order, sizeof(output->Order), "%s", line + 6);
        else if (BuildModuleStartsWith(line, "Name=")) snprintf(output->Name, sizeof(output->Name), "%s", line + 5);
        else if (BuildModuleStartsWith(line, "Root=")) snprintf(output->Root, sizeof(output->Root), "%s", line + 5);
        else if (BuildModuleStartsWith(line, "RelativePath=")) snprintf(output->RelativePath, sizeof(output->RelativePath), "%s", line + 13);
        else if (BuildModuleStartsWith(line, "Recursive=")) snprintf(output->Recursive, sizeof(output->Recursive), "%s", line + 10);
        else if (BuildModuleStartsWith(line, "Requires=")) snprintf(output->Requires, sizeof(output->Requires), "%s", line + 9);
    }

    fclose(file);
    if (output->Order[0] == 0 || output->Name[0] == 0 || output->Root[0] == 0 || output->RelativePath[0] == 0 || output->Recursive[0] == 0)
    {
        return 0;
    }

    return IsSafeBuildModuleName(output->Name) && IsSafeBuildModuleRelativePath(output->RelativePath);
}

static int CompareBuildModuleManifestFiles(const void* left, const void* right)
{
    const OrynBuildModuleManifestFile* left_module = (const OrynBuildModuleManifestFile*)left;
    const OrynBuildModuleManifestFile* right_module = (const OrynBuildModuleManifestFile*)right;
    int order_compare = strcmp(left_module->Order, right_module->Order);
    if (order_compare != 0)
    {
        return order_compare;
    }

    return strcmp(left_module->Name, right_module->Name);
}

static int LoadBuildModuleManifestDirectory(const OrynProject* project, const char* directory, OrynBuildModuleManifestFile* modules, int* module_count)
{
    DIR* dir = opendir(directory);
    if (!dir)
    {
        char message[ORYN_MAX_PATH + 96];
        snprintf(message, sizeof(message), "Build module manifest directory could not be opened: %s", directory);
        OrynLogFail(message);
        LogBuildPlanSkip(project, "build-module-manifest", message);
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!EndsWithBuild(entry->d_name, ".buildmodule"))
        {
            continue;
        }

        if (*module_count >= ORYN_MAX_BUILD_MODULE_MANIFESTS)
        {
            closedir(dir);
            OrynLogFail("Too many build module manifest files.");
            return 0;
        }

        char path[ORYN_MAX_PATH];
        OrynJoinPath(path, sizeof(path), directory, entry->d_name);
        if (!ParseBuildModuleManifestFile(path, &modules[*module_count]))
        {
            closedir(dir);
            OrynLogFail("Invalid build module manifest file.");
            LogBuildPlanSkip(project, "build-module-manifest", path);
            return 0;
        }

        *module_count += 1;
    }

    closedir(dir);
    return 1;
}

static int ResolveBuildModuleRoot(const OrynProject* project, const OrynBuildModuleManifestFile* module, char* output, size_t output_size)
{
    char base[ORYN_MAX_PATH];
    if (strcmp(module->Root, "SdkRoot") == 0)
    {
        snprintf(base, sizeof(base), "%s", project->sdk_root);
    }
    else if (strcmp(module->Root, "CommonKernelSource") == 0)
    {
        snprintf(base, sizeof(base), "%s", project->sdk_kernel_common_source_dir);
    }
    else if (strcmp(module->Root, "TargetKernelSource") == 0)
    {
        snprintf(base, sizeof(base), "%s", project->sdk_kernel_target_source_dir);
    }
    else
    {
        return 0;
    }

    if (strcmp(module->RelativePath, ".") == 0)
    {
        snprintf(output, output_size, "%s", base);
    }
    else
    {
        OrynJoinPath(output, output_size, base, module->RelativePath);
    }

    return 1;
}

static int AddBuildModulesFromManifestFiles(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    OrynBuildModuleManifestFile modules[ORYN_MAX_BUILD_MODULE_MANIFESTS];
    int module_count = 0;
    char common_dir[ORYN_MAX_PATH];
    char target_dir[ORYN_MAX_PATH];
    OrynJoinPath(common_dir, sizeof(common_dir), project->sdk_root, "Common/BuildModuleManifests");
    OrynJoinPath(target_dir, sizeof(target_dir), project->sdk_root, "Targets/UEFI/X64/BuildModuleManifests");

    if (!LoadBuildModuleManifestDirectory(project, common_dir, modules, &module_count)) return 0;
    if (!LoadBuildModuleManifestDirectory(project, target_dir, modules, &module_count)) return 0;
    qsort(modules, (size_t)module_count, sizeof(modules[0]), CompareBuildModuleManifestFiles);

    char message[160];
    snprintf(message, sizeof(message), "Loaded %d build module manifest file(s).", module_count);
    OrynLogOk(message);
    LogBuildPlanDecision(project, "build-module-manifest", message);

    for (int index = 0; index < module_count; ++index)
    {
        char source_root[ORYN_MAX_PATH];
        if (!ResolveBuildModuleRoot(project, &modules[index], source_root, sizeof(source_root)))
        {
            OrynLogFail("Unknown build module manifest root.");
            LogBuildPlanSkip(project, "build-module-manifest", modules[index].SourcePath);
            return 0;
        }

        int recursive = strcmp(modules[index].Recursive, "yes") == 0 || strcmp(modules[index].Recursive, "true") == 0 || strcmp(modules[index].Recursive, "1") == 0;
        if (!AddBuildModule(plan, project, modules[index].Name, source_root, recursive, modules[index].Requires))
        {
            return 0;
        }
        LogBuildPlanDecision(project, "build-module-manifest-source", modules[index].SourcePath);
    }

    return 1;
}

static int AddProjectModule(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    return AddBuildModule(plan, project, "ProjectKernel", project->source_dir, 1, "TargetRuntime,SDKRuntime");
}

int BuildKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    memset(plan, 0, sizeof(*plan));
    if (!AddBuildModulesFromManifestFiles(project, plan))
    {
        return 0;
    }

    return AddProjectModule(project, plan) && ResolveKernelArchivePlan(project, plan);
}
