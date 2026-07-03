#include "TargetBuildInternal.h"
#include <ctype.h>

static void StripManifestLine(char* text)
{
    size_t length = strlen(text);
    while (length > 0U &&
        (text[length - 1U] == '\n' || text[length - 1U] == '\r' || text[length - 1U] == ' ' || text[length - 1U] == '\t'))
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

static int StartsWithManifestText(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int ParseManifestYesNo(const char* text, int defaultValue)
{
    if (text == 0 || text[0] == 0)
    {
        return defaultValue;
    }
    if (strcmp(text, "Yes") == 0 || strcmp(text, "yes") == 0 || strcmp(text, "1") == 0 || strcmp(text, "true") == 0)
    {
        return 1;
    }
    if (strcmp(text, "No") == 0 || strcmp(text, "no") == 0 || strcmp(text, "0") == 0 || strcmp(text, "false") == 0)
    {
        return 0;
    }
    return defaultValue;
}

static int IsSafeManifestId(const char* text)
{
    if (strncmp(text, "OrynKernelModule", 16U) != 0)
    {
        return 0;
    }

    for (const char* cursor = text; *cursor != 0; ++cursor)
    {
        if (!isalnum((unsigned char)*cursor) && *cursor != '_')
        {
            return 0;
        }
    }

    return 1;
}

static int IsSafeManifestDisplayText(const char* text)
{
    for (const char* cursor = text; *cursor != 0; ++cursor)
    {
        unsigned char value = (unsigned char)*cursor;
        if (value < 32U || value == '"' || value == '\\')
        {
            return 0;
        }
    }

    return 1;
}

static int IsSafeManifestSelectionText(const char* text)
{
    if (text[0] == 0)
    {
        return 0;
    }

    for (const char* cursor = text; *cursor != 0; ++cursor)
    {
        if (!isalnum((unsigned char)*cursor) && *cursor != ',' && *cursor != '_')
        {
            return 0;
        }
    }

    return 1;
}

static int IsSafeManifestCallbackSymbol(const char* text)
{
    if (text == 0 || text[0] == 0)
    {
        return 0;
    }
    if (!(isalpha((unsigned char)text[0]) || text[0] == '_'))
    {
        return 0;
    }

    for (const char* cursor = text; *cursor != 0; ++cursor)
    {
        if (!isalnum((unsigned char)*cursor) && *cursor != '_')
        {
            return 0;
        }
    }

    return 1;
}

static int ParseKernelModuleManifestFile(const char* path, OrynKernelModuleManifestSource* output)
{
    memset(output, 0, sizeof(*output));
    snprintf(output->Path, sizeof(output->Path), "%s", path);
    output->CompiledIn = 1;
    snprintf(output->Select, sizeof(output->Select), "Always");
    snprintf(output->StopCallback, sizeof(output->StopCallback), "OrynKernelModuleDefaultStop");
    snprintf(output->PanicCallback, sizeof(output->PanicCallback), "OrynKernelModuleDefaultPanic");
    snprintf(output->ShutdownCallback, sizeof(output->ShutdownCallback), "OrynKernelModuleDefaultShutdown");

    FILE* file = fopen(path, "r");
    if (!file)
    {
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        StripManifestLine(line);
        if (line[0] == 0 || line[0] == '#')
        {
            continue;
        }

        if (StartsWithManifestText(line, "Order=")) snprintf(output->Order, sizeof(output->Order), "%s", line + 6);
        else if (StartsWithManifestText(line, "Id=")) snprintf(output->Id, sizeof(output->Id), "%s", line + 3);
        else if (StartsWithManifestText(line, "Name=")) snprintf(output->Name, sizeof(output->Name), "%s", line + 5);
        else if (StartsWithManifestText(line, "Items=")) snprintf(output->Items, sizeof(output->Items), "%s", line + 6);
        else if (StartsWithManifestText(line, "Requires=")) snprintf(output->Requires, sizeof(output->Requires), "%s", line + 9);
        else if (StartsWithManifestText(line, "CompiledIn=")) output->CompiledIn = ParseManifestYesNo(line + 11, 1);
        else if (StartsWithManifestText(line, "Required=")) output->Required = ParseManifestYesNo(line + 9, 0);
        else if (StartsWithManifestText(line, "FatalOnMissingPrerequisite=")) output->FatalOnMissingPrerequisite = ParseManifestYesNo(line + 27, 0);
        else if (StartsWithManifestText(line, "Select=")) snprintf(output->Select, sizeof(output->Select), "%s", line + 7);
        else if (StartsWithManifestText(line, "StopCallback=")) snprintf(output->StopCallback, sizeof(output->StopCallback), "%s", line + 13);
        else if (StartsWithManifestText(line, "PanicCallback=")) snprintf(output->PanicCallback, sizeof(output->PanicCallback), "%s", line + 14);
        else if (StartsWithManifestText(line, "ShutdownCallback=")) snprintf(output->ShutdownCallback, sizeof(output->ShutdownCallback), "%s", line + 17);
    }

    fclose(file);

    if (output->Order[0] == 0 || output->Id[0] == 0 || output->Name[0] == 0 || output->Items[0] == 0)
    {
        return 0;
    }

    return IsSafeManifestId(output->Id) &&
        IsSafeManifestDisplayText(output->Name) &&
        IsSafeManifestDisplayText(output->Items) &&
        IsSafeManifestSelectionText(output->Select) &&
        IsSafeManifestCallbackSymbol(output->StopCallback) &&
        IsSafeManifestCallbackSymbol(output->PanicCallback) &&
        IsSafeManifestCallbackSymbol(output->ShutdownCallback);
}

static int CompareKernelModuleManifestSources(const void* left, const void* right)
{
    const OrynKernelModuleManifestSource* left_module = (const OrynKernelModuleManifestSource*)left;
    const OrynKernelModuleManifestSource* right_module = (const OrynKernelModuleManifestSource*)right;
    int order_compare = strcmp(left_module->Order, right_module->Order);
    if (order_compare != 0)
    {
        return order_compare;
    }

    return strcmp(left_module->Id, right_module->Id);
}

static int ManifestIdAlreadyLoaded(const OrynKernelModuleManifestSource* modules, int module_count, const char* id)
{
    for (int index = 0; index < module_count; ++index)
    {
        if (strcmp(modules[index].Id, id) == 0)
        {
            return 1;
        }
    }

    return 0;
}

static int AddManifestSource(const OrynProject* project, const char* path, OrynKernelModuleManifestSource* modules, int* module_count)
{
    if (*module_count >= ORYN_MAX_KERNEL_MODULE_MANIFESTS)
    {
        OrynLogFail("Too many kernel module manifest files.");
        return 0;
    }

    OrynKernelModuleManifestSource candidate;
    if (!ParseKernelModuleManifestFile(path, &candidate))
    {
        char message[ORYN_MAX_PATH + 96];
        snprintf(message, sizeof(message), "Invalid kernel module manifest: %s", path);
        OrynLogFail(message);
        LogBuildPlanSkip(project, "kernel-module-manifest", message);
        return 0;
    }

    if (ManifestIdAlreadyLoaded(modules, *module_count, candidate.Id))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Duplicate kernel module manifest id %s from %s", candidate.Id, path);
        OrynLogFail(message);
        LogBuildPlanSkip(project, "kernel-module-manifest", message);
        return 0;
    }

    modules[*module_count] = candidate;
    *module_count += 1;
    return 1;
}

static int ReadManifestDirectoryRecursive(const OrynProject* project, const char* directory, OrynKernelModuleManifestSource* modules, int* module_count)
{
    DIR* dir = opendir(directory);
    if (!dir)
    {
        return 1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char path[ORYN_MAX_PATH];
        OrynJoinPath(path, sizeof(path), directory, entry->d_name);
        struct stat status;
        if (stat(path, &status) != 0)
        {
            continue;
        }

        if (S_ISDIR(status.st_mode))
        {
            if (!ReadManifestDirectoryRecursive(project, path, modules, module_count))
            {
                closedir(dir);
                return 0;
            }
        }
        else if (EndsWithBuild(entry->d_name, ".module"))
        {
            if (!AddManifestSource(project, path, modules, module_count))
            {
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);
    return 1;
}

int ReadKernelModuleManifestSources(const OrynProject* project, OrynKernelModuleManifestSource* modules, int* module_count)
{
    const char* roots[] = {
        "Common/Kernel/Source",
        "Common/OrynLibC",
        "Targets/UEFI/X64/KernelSupport/Source"
    };

    *module_count = 0;
    for (unsigned int index = 0U; index < sizeof(roots) / sizeof(roots[0]); ++index)
    {
        char root[ORYN_MAX_PATH];
        OrynJoinPath(root, sizeof(root), project->sdk_root, roots[index]);
        if (!ReadManifestDirectoryRecursive(project, root, modules, module_count))
        {
            return 0;
        }
    }

    if (*module_count == 0)
    {
        OrynLogFail("No beside-module kernel manifest files were found.");
        LogBuildPlanSkip(project, "kernel-module-manifest", "No .module files found beside module source directories.");
        return 0;
    }

    qsort(modules, (size_t)*module_count, sizeof(modules[0]), CompareKernelModuleManifestSources);

    char message[160];
    snprintf(message, sizeof(message), "Loaded %d beside-module kernel manifest file(s).", *module_count);
    OrynLogOk(message);
    LogBuildPlanDecision(project, "kernel-module-manifest", message);
    return 1;
}
