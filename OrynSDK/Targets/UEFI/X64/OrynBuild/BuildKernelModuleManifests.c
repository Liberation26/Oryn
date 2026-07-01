#include "TargetBuildInternal.h"
#include <ctype.h>

#define ORYN_MAX_KERNEL_MODULE_MANIFESTS 96
#define ORYN_MAX_REQUIRE_TEXT 512

typedef struct OrynKernelModuleManifestSource
{
    char Order[16];
    char Id[128];
    char Name[128];
    char Items[512];
    char Requires[ORYN_MAX_REQUIRE_TEXT];
    char Path[ORYN_MAX_PATH];
} OrynKernelModuleManifestSource;

static void StripLine(char* text)
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

static int StartsWithText(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
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

static int IsSafeDisplayText(const char* text)
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

static int ParseKernelModuleManifestFile(const char* path, OrynKernelModuleManifestSource* output)
{
    memset(output, 0, sizeof(*output));
    snprintf(output->Path, sizeof(output->Path), "%s", path);

    FILE* file = fopen(path, "r");
    if (!file)
    {
        return 0;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file))
    {
        StripLine(line);
        if (line[0] == 0 || line[0] == '#')
        {
            continue;
        }

        if (StartsWithText(line, "Order="))
        {
            snprintf(output->Order, sizeof(output->Order), "%s", line + 6);
        }
        else if (StartsWithText(line, "Id="))
        {
            snprintf(output->Id, sizeof(output->Id), "%s", line + 3);
        }
        else if (StartsWithText(line, "Name="))
        {
            snprintf(output->Name, sizeof(output->Name), "%s", line + 5);
        }
        else if (StartsWithText(line, "Items="))
        {
            snprintf(output->Items, sizeof(output->Items), "%s", line + 6);
        }
        else if (StartsWithText(line, "Requires="))
        {
            snprintf(output->Requires, sizeof(output->Requires), "%s", line + 9);
        }
    }

    fclose(file);

    if (output->Order[0] == 0 || output->Id[0] == 0 || output->Name[0] == 0 || output->Items[0] == 0)
    {
        return 0;
    }

    if (!IsSafeManifestId(output->Id) || !IsSafeDisplayText(output->Name) || !IsSafeDisplayText(output->Items))
    {
        return 0;
    }

    return 1;
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

static int ReadKernelModuleManifestSources(const OrynProject* project, OrynKernelModuleManifestSource* modules, int* module_count)
{
    char manifest_dir[ORYN_MAX_PATH];
    OrynJoinPath(manifest_dir, sizeof(manifest_dir), project->sdk_root, "Common/Kernel/ModuleManifests");

    DIR* dir = opendir(manifest_dir);
    if (!dir)
    {
        OrynLogFail("Kernel module manifest source directory is missing.");
        LogBuildPlanSkip(project, "kernel-module-manifest", "Common/Kernel/ModuleManifests could not be opened.");
        return 0;
    }

    *module_count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (!EndsWithBuild(entry->d_name, ".module"))
        {
            continue;
        }

        if (*module_count >= ORYN_MAX_KERNEL_MODULE_MANIFESTS)
        {
            closedir(dir);
            OrynLogFail("Too many kernel module manifest files.");
            return 0;
        }

        char path[ORYN_MAX_PATH];
        OrynJoinPath(path, sizeof(path), manifest_dir, entry->d_name);
        if (!ParseKernelModuleManifestFile(path, &modules[*module_count]))
        {
            closedir(dir);
            char message[ORYN_MAX_PATH + 96];
            snprintf(message, sizeof(message), "Invalid kernel module manifest: %s", path);
            OrynLogFail(message);
            LogBuildPlanSkip(project, "kernel-module-manifest", message);
            return 0;
        }

        *module_count += 1;
    }

    closedir(dir);
    qsort(modules, (size_t)*module_count, sizeof(modules[0]), CompareKernelModuleManifestSources);

    if (*module_count == 0)
    {
        OrynLogFail("No kernel module manifest files were found.");
        return 0;
    }

    for (int index = 0; index < *module_count; ++index)
    {
        for (int probe = index + 1; probe < *module_count; ++probe)
        {
            if (strcmp(modules[index].Id, modules[probe].Id) == 0)
            {
                OrynLogFail("Duplicate kernel module manifest id.");
                return 0;
            }
        }
    }

    char message[160];
    snprintf(message, sizeof(message), "Loaded %d per-module kernel manifest file(s).", *module_count);
    OrynLogOk(message);
    LogBuildPlanDecision(project, "kernel-module-manifest", message);
    return 1;
}

static void WriteRequiresArray(FILE* file, const char* id, const char* requires)
{
    fprintf(file, "    static const OrynKernelModuleId requires_%s[] = { ", id);
    if (requires[0] == 0)
    {
        fprintf(file, "OrynKernelModuleCount");
    }
    else
    {
        char buffer[ORYN_MAX_REQUIRE_TEXT];
        snprintf(buffer, sizeof(buffer), "%s", requires);
        char* token = strtok(buffer, ",");
        int written = 0;
        while (token)
        {
            StripLine(token);
            if (written > 0)
            {
                fprintf(file, ", ");
            }
            fprintf(file, "%s", token);
            written += 1;
            token = strtok(NULL, ",");
        }
    }

    fprintf(file, " };\n");
}

static int CountRequires(const char* requires)
{
    if (requires[0] == 0)
    {
        return 0;
    }

    int count = 1;
    for (const char* cursor = requires; *cursor != 0; ++cursor)
    {
        if (*cursor == ',')
        {
            count += 1;
        }
    }

    return count;
}

static int WriteKernelModuleManifestHeader(const OrynProject* project, const OrynKernelModuleManifestSource* modules, int module_count)
{
    char header_path[ORYN_MAX_PATH];
    OrynJoinPath(header_path, sizeof(header_path), project->sdk_root, "Common/Kernel/Include/KernelModuleManifest.h");

    FILE* file = fopen(header_path, "w");
    if (!file)
    {
        return 0;
    }

    fprintf(file, "#ifndef ORYN_KERNEL_MODULE_MANIFEST_H\n");
    fprintf(file, "#define ORYN_KERNEL_MODULE_MANIFEST_H\n\n");
    fprintf(file, "/* Generated from kernel module manifest files. Do not hand-edit module tables here. */\n\n");
    fprintf(file, "typedef enum OrynKernelModuleId\n{\n");
    for (int index = 0; index < module_count; ++index)
    {
        fprintf(file, "    %s = %d,\n", modules[index].Id, index);
    }
    fprintf(file, "    OrynKernelModuleCount\n");
    fprintf(file, "} OrynKernelModuleId;\n\n");
    fprintf(file, "typedef enum OrynKernelModuleState\n{\n");
    fprintf(file, "    OrynKernelModuleStateAbsent = 0,\n");
    fprintf(file, "    OrynKernelModuleStateRegistered,\n");
    fprintf(file, "    OrynKernelModuleStateStarting,\n");
    fprintf(file, "    OrynKernelModuleStateReady,\n");
    fprintf(file, "    OrynKernelModuleStateSkipped,\n");
    fprintf(file, "    OrynKernelModuleStateFailed\n");
    fprintf(file, "} OrynKernelModuleState;\n\n");
    fprintf(file, "typedef struct OrynKernelModuleManifestItem\n{\n");
    fprintf(file, "    OrynKernelModuleId Id;\n");
    fprintf(file, "    const char* Name;\n");
    fprintf(file, "    const char* Items;\n");
    fprintf(file, "    OrynKernelModuleId Requires[6];\n");
    fprintf(file, "    unsigned int RequireCount;\n");
    fprintf(file, "    OrynKernelModuleState State;\n");
    fprintf(file, "} OrynKernelModuleManifestItem;\n\n");
    fprintf(file, "void OrynKernelModuleManifestInit(void);\n");
    fprintf(file, "const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestCanStart(OrynKernelModuleId id);\n");
    fprintf(file, "unsigned int OrynKernelModuleManifestRequireCount(OrynKernelModuleId id);\n");
    fprintf(file, "OrynKernelModuleId OrynKernelModuleManifestRequireAt(OrynKernelModuleId id, unsigned int index);\n");
    fprintf(file, "int OrynKernelModuleManifestIsReady(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestBegin(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestReady(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestSkipped(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestFailed(OrynKernelModuleId id);\n");
    fprintf(file, "const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state);\n");
    fprintf(file, "void OrynKernelModuleManifestPrintProof(void);\n\n");
    fprintf(file, "#endif\n");
    fclose(file);
    return 1;
}

static int WriteKernelModuleManifestData(const OrynProject* project, const OrynKernelModuleManifestSource* modules, int module_count)
{
    char data_path[ORYN_MAX_PATH];
    OrynJoinPath(data_path, sizeof(data_path), project->sdk_root, "Common/Kernel/Source/Manifest/KernelModuleManifestData.c");

    FILE* file = fopen(data_path, "w");
    if (!file)
    {
        return 0;
    }

    fprintf(file, "#include \"KernelModuleManifest.h\"\n\n");
    fprintf(file, "/* Generated from kernel module manifest files. Do not hand-edit module tables here. */\n\n");
    fprintf(file, "static OrynKernelModuleManifestItem gKernelModuleManifest[OrynKernelModuleCount];\n\n");
    fprintf(file, "static void SetModule(\n");
    fprintf(file, "    OrynKernelModuleId id,\n");
    fprintf(file, "    const char* name,\n");
    fprintf(file, "    const char* items,\n");
    fprintf(file, "    const OrynKernelModuleId* requires,\n");
    fprintf(file, "    unsigned int requireCount)\n{\n");
    fprintf(file, "    gKernelModuleManifest[id].Id = id;\n");
    fprintf(file, "    gKernelModuleManifest[id].Name = name;\n");
    fprintf(file, "    gKernelModuleManifest[id].Items = items;\n");
    fprintf(file, "    gKernelModuleManifest[id].RequireCount = requireCount;\n");
    fprintf(file, "    gKernelModuleManifest[id].State = OrynKernelModuleStateRegistered;\n");
    fprintf(file, "    for (unsigned int index = 0U; index < requireCount && index < 6U; ++index)\n    {\n");
    fprintf(file, "        gKernelModuleManifest[id].Requires[index] = requires[index];\n");
    fprintf(file, "    }\n}\n\n");
    fprintf(file, "void OrynKernelModuleManifestInit(void)\n{\n");
    for (int index = 0; index < module_count; ++index)
    {
        WriteRequiresArray(file, modules[index].Id, modules[index].Requires);
    }
    fprintf(file, "\n");
    for (int index = 0; index < module_count; ++index)
    {
        fprintf(file, "    SetModule(%s, \"%s\", \"%s\", requires_%s, %dU);\n",
            modules[index].Id,
            modules[index].Name,
            modules[index].Items,
            modules[index].Id,
            CountRequires(modules[index].Requires));
    }
    fprintf(file, "}\n\n");
    fprintf(file, "OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id)\n{\n");
    fprintf(file, "    if ((unsigned int)id >= (unsigned int)OrynKernelModuleCount)\n    {\n        return 0;\n    }\n\n");
    fprintf(file, "    return &gKernelModuleManifest[id];\n}\n\n");
    fprintf(file, "const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id)\n{\n");
    fprintf(file, "    return OrynKernelModuleManifestMutable(id);\n}\n");

    fclose(file);
    return 1;
}

static int WriteKernelModuleManifestReport(const OrynProject* project, const OrynKernelModuleManifestSource* modules, int module_count)
{
    char plan_dir[ORYN_MAX_PATH];
    OrynJoinPath(plan_dir, sizeof(plan_dir), project->build_dir, "Plan");
    OrynMakeDirectoryRecursive(plan_dir);

    char report_path[ORYN_MAX_PATH];
    OrynJoinPath(report_path, sizeof(report_path), plan_dir, "KernelModuleManifestGeneration.txt");
    FILE* file = fopen(report_path, "w");
    if (!file)
    {
        return 0;
    }

    fprintf(file, "ORYN_KERNEL_MODULE_MANIFEST_GENERATION_V1\n");
    fprintf(file, "PackageVersion=%s\n", ORYN_VERSION);
    fprintf(file, "SourceRoot=Common/Kernel/ModuleManifests\n");
    fprintf(file, "GeneratedHeader=Common/Kernel/Include/KernelModuleManifest.h\n");
    fprintf(file, "GeneratedData=Common/Kernel/Source/Manifest/KernelModuleManifestData.c\n");
    fprintf(file, "ModuleCount=%d\n", module_count);
    fprintf(file, "Fields=Order<TAB>Id<TAB>Name<TAB>Requires<TAB>SourcePath\n");
    fprintf(file, "Modules:\n");
    for (int index = 0; index < module_count; ++index)
    {
        fprintf(file, "%s\t%s\t%s\t%s\t%s\n",
            modules[index].Order,
            modules[index].Id,
            modules[index].Name,
            modules[index].Requires,
            modules[index].Path);
    }

    fclose(file);
    return 1;
}

int GenerateKernelModuleManifestTables(const OrynProject* project)
{
    OrynKernelModuleManifestSource modules[ORYN_MAX_KERNEL_MODULE_MANIFESTS];
    int module_count = 0;
    if (!ReadKernelModuleManifestSources(project, modules, &module_count))
    {
        return 0;
    }

    if (!WriteKernelModuleManifestHeader(project, modules, module_count))
    {
        OrynLogFail("Failed to write generated KernelModuleManifest.h.");
        return 0;
    }

    if (!WriteKernelModuleManifestData(project, modules, module_count))
    {
        OrynLogFail("Failed to write generated KernelModuleManifestData.c.");
        return 0;
    }

    if (!WriteKernelModuleManifestReport(project, modules, module_count))
    {
        OrynLogWarn("Kernel module manifest generation report could not be written.");
    }

    LogBuildPlanDecision(project, "kernel-module-manifest", "Generated kernel module C tables from per-module manifest files.");
    return 1;
}
