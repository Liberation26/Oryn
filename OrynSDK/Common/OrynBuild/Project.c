#include "OrynBuild.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void Trim(char* text)
{
    char* start = text;
    while (*start != 0 && isspace((unsigned char)*start))
    {
        ++start;
    }

    if (start != text)
    {
        memmove(text, start, strlen(start) + 1);
    }

    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1]))
    {
        text[length - 1] = 0;
        --length;
    }
}

static void SetDefaultProjectValues(OrynProject* project)
{
    snprintf(project->name, sizeof(project->name), "Kernel-5");
    snprintf(project->type, sizeof(project->type), "Kernel");
    snprintf(project->target, sizeof(project->target), "uefi-x64");
    snprintf(project->toolchain, sizeof(project->toolchain), "clang");
    snprintf(project->architecture, sizeof(project->architecture), "x86_64");
    snprintf(project->entry, sizeof(project->entry), "KernelStart");
    snprintf(project->run_display, sizeof(project->run_display), "none");
}

static int ResolveExecutablePath(const char* executable_path, char* output, size_t output_size)
{
    if (strchr(executable_path, '/') != 0)
    {
        if (realpath(executable_path, output) != 0)
        {
            return 1;
        }
    }

    char found[ORYN_MAX_PATH];
    if (OrynFindProgram(executable_path, found, sizeof(found)))
    {
        if (realpath(found, output) != 0)
        {
            return 1;
        }
    }

    snprintf(output, output_size, "%s", executable_path);
    return 0;
}

static void ResolveSdkRoot(const char* executable_path, char* output, size_t output_size)
{
    char resolved[ORYN_MAX_PATH];
    ResolveExecutablePath(executable_path, resolved, sizeof(resolved));

    char bin_directory[ORYN_MAX_PATH];
    OrynGetDirectoryName(bin_directory, sizeof(bin_directory), resolved);

    char maybe_common[ORYN_MAX_PATH];
    OrynGetDirectoryName(maybe_common, sizeof(maybe_common), bin_directory);

    char maybe_common_name[ORYN_MAX_PATH];
    OrynGetBaseName(maybe_common_name, sizeof(maybe_common_name), maybe_common);

    if (strcmp(maybe_common_name, "Common") == 0)
    {
        char maybe_sdk[ORYN_MAX_PATH];
        OrynGetDirectoryName(maybe_sdk, sizeof(maybe_sdk), maybe_common);
        snprintf(output, output_size, "%s", maybe_sdk);
        return;
    }

    snprintf(output, output_size, "%s", maybe_common);
}


static void ApplyProjectKey(OrynProject* project, const char* key, const char* value)
{
    if (strcmp(key, "Name") == 0)
    {
        snprintf(project->name, sizeof(project->name), "%s", value);
    }
    else if (strcmp(key, "Type") == 0)
    {
        snprintf(project->type, sizeof(project->type), "%s", value);
    }
    else if (strcmp(key, "Target") == 0)
    {
        snprintf(project->target, sizeof(project->target), "%s", value);
    }
    else if (strcmp(key, "Toolchain") == 0)
    {
        snprintf(project->toolchain, sizeof(project->toolchain), "%s", value);
    }
    else if (strcmp(key, "Architecture") == 0)
    {
        snprintf(project->architecture, sizeof(project->architecture), "%s", value);
    }
    else if (strcmp(key, "Entry") == 0)
    {
        snprintf(project->entry, sizeof(project->entry), "%s", value);
    }
    else if (strcmp(key, "Display") == 0)
    {
        snprintf(project->run_display, sizeof(project->run_display), "%s", value);
    }
}

int OrynLoadProject(const char* executable_path, const char* project_file, OrynProject* project)
{
    memset(project, 0, sizeof(*project));
    SetDefaultProjectValues(project);

    char real_project[ORYN_MAX_PATH];
    if (realpath(project_file, real_project) == 0)
    {
        OrynLogFail("Project file was not found.");
        return 0;
    }

    snprintf(project->project_file, sizeof(project->project_file), "%s", real_project);
    OrynGetDirectoryName(project->project_root, sizeof(project->project_root), real_project);
    ResolveSdkRoot(executable_path, project->sdk_root, sizeof(project->sdk_root));

    OrynJoinPath(project->source_dir, sizeof(project->source_dir), project->project_root, "Source");
    OrynJoinPath(project->include_dir, sizeof(project->include_dir), project->project_root, "Include");
    OrynJoinPath(project->sdk_kernel_common_include_dir, sizeof(project->sdk_kernel_common_include_dir), project->sdk_root, "Common/Kernel/Include");
    OrynJoinPath(project->sdk_kernel_common_source_dir, sizeof(project->sdk_kernel_common_source_dir), project->sdk_root, "Common/Kernel/Source");
    OrynJoinPath(project->sdk_kernel_target_include_dir, sizeof(project->sdk_kernel_target_include_dir), project->sdk_root, "Targets/UEFI/X64/KernelSupport/Include");
    OrynJoinPath(project->sdk_kernel_target_source_dir, sizeof(project->sdk_kernel_target_source_dir), project->sdk_root, "Targets/UEFI/X64/KernelSupport/Source");
    OrynJoinPath(project->build_dir, sizeof(project->build_dir), project->project_root, "Build");
    OrynJoinPath(project->object_dir, sizeof(project->object_dir), project->build_dir, "Objects");
    OrynJoinPath(project->output_dir, sizeof(project->output_dir), project->project_root, "Output");
    OrynJoinPath(project->esp_dir, sizeof(project->esp_dir), project->output_dir, "ESP");
    OrynJoinPath(project->kernel_variants_root, sizeof(project->kernel_variants_root), project->project_root, "Kernel");
    OrynFindOvmf(project->ovmf_path, sizeof(project->ovmf_path));

    FILE* file = fopen(real_project, "rb");
    if (file == 0)
    {
        OrynLogFail("Could not open project file.");
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file) != 0)
    {
        Trim(line);
        if (line[0] == 0 || line[0] == '#' || line[0] == '[')
        {
            continue;
        }

        char* equals = strchr(line, '=');
        if (equals == 0)
        {
            continue;
        }

        *equals = 0;
        char* key = line;
        char* value = equals + 1;
        Trim(key);
        Trim(value);
        ApplyProjectKey(project, key, value);
    }

    fclose(file);
    OrynResolveBootInfoSelection(project);
    return 1;
}
