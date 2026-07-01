#include "TargetBuildInternal.h"
#include <ctype.h>

#define ORYN_MAX_KERNEL_MODULE_MANIFESTS 96
#define ORYN_MAX_REQUIRE_TEXT 512
#define ORYN_MAX_SELECT_TEXT 256

typedef struct OrynKernelModuleManifestSource
{
    char Order[16];
    char Id[128];
    char Name[128];
    char Items[512];
    char Requires[ORYN_MAX_REQUIRE_TEXT];
    char Select[ORYN_MAX_SELECT_TEXT];
    int CompiledIn;
    int Required;
    int FatalOnMissingPrerequisite;
    char StopCallback[128];
    char PanicCallback[128];
    char ShutdownCallback[128];
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

static int ParseYesNo(const char* text, int defaultValue)
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

static int IsSafeSelectionText(const char* text)
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


static int IsSafeCallbackSymbol(const char* text)
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
    output->Required = 0;
    output->FatalOnMissingPrerequisite = 0;
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
        else if (StartsWithText(line, "CompiledIn="))
        {
            output->CompiledIn = ParseYesNo(line + 11, 1);
        }
        else if (StartsWithText(line, "Required="))
        {
            output->Required = ParseYesNo(line + 9, 0);
        }
        else if (StartsWithText(line, "FatalOnMissingPrerequisite="))
        {
            output->FatalOnMissingPrerequisite = ParseYesNo(line + 27, 0);
        }
        else if (StartsWithText(line, "Select="))
        {
            snprintf(output->Select, sizeof(output->Select), "%s", line + 7);
        }
        else if (StartsWithText(line, "StopCallback="))
        {
            snprintf(output->StopCallback, sizeof(output->StopCallback), "%s", line + 13);
        }
        else if (StartsWithText(line, "PanicCallback="))
        {
            snprintf(output->PanicCallback, sizeof(output->PanicCallback), "%s", line + 14);
        }
        else if (StartsWithText(line, "ShutdownCallback="))
        {
            snprintf(output->ShutdownCallback, sizeof(output->ShutdownCallback), "%s", line + 17);
        }
    }

    fclose(file);

    if (output->Order[0] == 0 || output->Id[0] == 0 || output->Name[0] == 0 || output->Items[0] == 0)
    {
        return 0;
    }

    if (!IsSafeManifestId(output->Id) || !IsSafeDisplayText(output->Name) || !IsSafeDisplayText(output->Items) ||
        !IsSafeSelectionText(output->Select) ||
        !IsSafeCallbackSymbol(output->StopCallback) ||
        !IsSafeCallbackSymbol(output->PanicCallback) ||
        !IsSafeCallbackSymbol(output->ShutdownCallback))
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
    fprintf(file, "    OrynKernelModuleStateSelected,\n");
    fprintf(file, "    OrynKernelModuleStateStarting,\n");
    fprintf(file, "    OrynKernelModuleStateReady,\n");
    fprintf(file, "    OrynKernelModuleStateStopping,\n");
    fprintf(file, "    OrynKernelModuleStateStopped,\n");
    fprintf(file, "    OrynKernelModuleStatePanic,\n");
    fprintf(file, "    OrynKernelModuleStateShuttingDown,\n");
    fprintf(file, "    OrynKernelModuleStateShutdown,\n");
    fprintf(file, "    OrynKernelModuleStateSkipped,\n");
    fprintf(file, "    OrynKernelModuleStateFailed\n");
    fprintf(file, "} OrynKernelModuleState;\n\n");
    fprintf(file, "typedef int (*OrynKernelModuleLifecycleCallback)(OrynKernelModuleId id);\n\n");
    fprintf(file, "typedef struct OrynKernelModuleManifestItem\n{\n");
    fprintf(file, "    OrynKernelModuleId Id;\n");
    fprintf(file, "    const char* Name;\n");
    fprintf(file, "    const char* Items;\n");
    fprintf(file, "    const char* Selects;\n");
    fprintf(file, "    OrynKernelModuleId Requires[6];\n");
    fprintf(file, "    unsigned int RequireCount;\n");
    fprintf(file, "    int CompiledIn;\n");
    fprintf(file, "    int Required;\n");
    fprintf(file, "    int FatalOnMissingPrerequisite;\n");
    fprintf(file, "    const char* StopCallbackName;\n");
    fprintf(file, "    const char* PanicCallbackName;\n");
    fprintf(file, "    const char* ShutdownCallbackName;\n");
    fprintf(file, "    OrynKernelModuleLifecycleCallback StopCallback;\n");
    fprintf(file, "    OrynKernelModuleLifecycleCallback PanicCallback;\n");
    fprintf(file, "    OrynKernelModuleLifecycleCallback ShutdownCallback;\n");
    fprintf(file, "    OrynKernelModuleState State;\n");
    fprintf(file, "} OrynKernelModuleManifestItem;\n\n");
    fprintf(file, "typedef struct OrynKernelCompiledModuleRecord\n{\n");
    fprintf(file, "    OrynKernelModuleId Id;\n");
    fprintf(file, "    const char* Name;\n");
    fprintf(file, "    int CompiledIn;\n");
    fprintf(file, "} OrynKernelCompiledModuleRecord;\n\n");
    fprintf(file, "void OrynKernelModuleManifestInit(void);\n");
    fprintf(file, "const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestCanStart(OrynKernelModuleId id);\n");
    fprintf(file, "unsigned int OrynKernelModuleManifestRequireCount(OrynKernelModuleId id);\n");
    fprintf(file, "OrynKernelModuleId OrynKernelModuleManifestRequireAt(OrynKernelModuleId id, unsigned int index);\n");
    fprintf(file, "int OrynKernelModuleManifestIsReady(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestIsCompiledIn(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestIsRequired(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestFatalOnMissingPrerequisite(OrynKernelModuleId id);\n");
    fprintf(file, "const char* OrynKernelModuleManifestSelects(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestSelected(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestBegin(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestReady(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestSkipped(OrynKernelModuleId id);\n");
    fprintf(file, "void OrynKernelModuleManifestFailed(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleDefaultStop(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleDefaultPanic(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleDefaultShutdown(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestHasLifecycleCallbacks(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestStop(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestPanic(OrynKernelModuleId id);\n");
    fprintf(file, "int OrynKernelModuleManifestShutdown(OrynKernelModuleId id);\n");
    fprintf(file, "unsigned int OrynKernelModuleManifestInvokeStopCallbacks(void);\n");
    fprintf(file, "unsigned int OrynKernelModuleManifestInvokePanicCallbacks(void);\n");
    fprintf(file, "unsigned int OrynKernelModuleManifestInvokeShutdownCallbacks(void);\n");
    fprintf(file, "void OrynKernelModuleManifestCallbackProof(void);\n");
    fprintf(file, "const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state);\n");
    fprintf(file, "unsigned int OrynKernelCompiledModuleCount(void);\n");
    fprintf(file, "const OrynKernelCompiledModuleRecord* OrynKernelCompiledModuleGet(unsigned int index);\n");
    fprintf(file, "void OrynKernelCompiledModuleRegistryPrintProof(void);\n");
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
    fprintf(file, "int OrynKernelModuleDefaultStop(OrynKernelModuleId id)\n{\n    (void)id;\n    return 1;\n}\n\n");
    fprintf(file, "int OrynKernelModuleDefaultPanic(OrynKernelModuleId id)\n{\n    (void)id;\n    return 1;\n}\n\n");
    fprintf(file, "int OrynKernelModuleDefaultShutdown(OrynKernelModuleId id)\n{\n    (void)id;\n    return 1;\n}\n\n");
    fprintf(file, "static OrynKernelModuleManifestItem gKernelModuleManifest[OrynKernelModuleCount];\n");
    fprintf(file, "static OrynKernelCompiledModuleRecord gCompiledKernelModules[OrynKernelModuleCount];\n\n");
    fprintf(file, "static void SetModule(\n");
    fprintf(file, "    OrynKernelModuleId id,\n");
    fprintf(file, "    const char* name,\n");
    fprintf(file, "    const char* items,\n");
    fprintf(file, "    const char* selects,\n");
    fprintf(file, "    const OrynKernelModuleId* requires,\n");
    fprintf(file, "    unsigned int requireCount,\n");
    fprintf(file, "    int compiledIn,\n");
    fprintf(file, "    int required,\n");
    fprintf(file, "    int fatalOnMissingPrerequisite,\n");
    fprintf(file, "    const char* stopCallbackName,\n");
    fprintf(file, "    const char* panicCallbackName,\n");
    fprintf(file, "    const char* shutdownCallbackName,\n");
    fprintf(file, "    OrynKernelModuleLifecycleCallback stopCallback,\n");
    fprintf(file, "    OrynKernelModuleLifecycleCallback panicCallback,\n");
    fprintf(file, "    OrynKernelModuleLifecycleCallback shutdownCallback)\n{\n");
    fprintf(file, "    gKernelModuleManifest[id].Id = id;\n");
    fprintf(file, "    gKernelModuleManifest[id].Name = name;\n");
    fprintf(file, "    gKernelModuleManifest[id].Items = items;\n");
    fprintf(file, "    gKernelModuleManifest[id].Selects = selects;\n");
    fprintf(file, "    gKernelModuleManifest[id].RequireCount = requireCount;\n");
    fprintf(file, "    gKernelModuleManifest[id].CompiledIn = compiledIn;\n");
    fprintf(file, "    gKernelModuleManifest[id].Required = required;\n");
    fprintf(file, "    gKernelModuleManifest[id].FatalOnMissingPrerequisite = fatalOnMissingPrerequisite;\n");
    fprintf(file, "    gKernelModuleManifest[id].StopCallbackName = stopCallbackName;\n");
    fprintf(file, "    gKernelModuleManifest[id].PanicCallbackName = panicCallbackName;\n");
    fprintf(file, "    gKernelModuleManifest[id].ShutdownCallbackName = shutdownCallbackName;\n");
    fprintf(file, "    gKernelModuleManifest[id].StopCallback = stopCallback;\n");
    fprintf(file, "    gKernelModuleManifest[id].PanicCallback = panicCallback;\n");
    fprintf(file, "    gKernelModuleManifest[id].ShutdownCallback = shutdownCallback;\n");
    fprintf(file, "    gKernelModuleManifest[id].State = compiledIn ? OrynKernelModuleStateRegistered : OrynKernelModuleStateAbsent;\n");
    fprintf(file, "    gCompiledKernelModules[id].Id = id;\n");
    fprintf(file, "    gCompiledKernelModules[id].Name = name;\n");
    fprintf(file, "    gCompiledKernelModules[id].CompiledIn = compiledIn;\n");
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
        fprintf(file, "    SetModule(%s, \"%s\", \"%s\", \"%s\", requires_%s, %dU, %d, %d, %d, \"%s\", \"%s\", \"%s\", %s, %s, %s);\n",
            modules[index].Id,
            modules[index].Name,
            modules[index].Items,
            modules[index].Select,
            modules[index].Id,
            CountRequires(modules[index].Requires),
            modules[index].CompiledIn,
            modules[index].Required,
            modules[index].FatalOnMissingPrerequisite,
            modules[index].StopCallback,
            modules[index].PanicCallback,
            modules[index].ShutdownCallback,
            modules[index].StopCallback,
            modules[index].PanicCallback,
            modules[index].ShutdownCallback);
    }
    fprintf(file, "}\n\n");
    fprintf(file, "OrynKernelModuleManifestItem* OrynKernelModuleManifestMutable(OrynKernelModuleId id)\n{\n");
    fprintf(file, "    if ((unsigned int)id >= (unsigned int)OrynKernelModuleCount)\n    {\n        return 0;\n    }\n\n");
    fprintf(file, "    return &gKernelModuleManifest[id];\n}\n\n");
    fprintf(file, "const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id)\n{\n");
    fprintf(file, "    return OrynKernelModuleManifestMutable(id);\n}\n\n");
    fprintf(file, "unsigned int OrynKernelCompiledModuleCount(void)\n{\n    return (unsigned int)OrynKernelModuleCount;\n}\n\n");
    fprintf(file, "const OrynKernelCompiledModuleRecord* OrynKernelCompiledModuleGet(unsigned int index)\n{\n");
    fprintf(file, "    if (index >= (unsigned int)OrynKernelModuleCount)\n    {\n        return 0;\n    }\n    return &gCompiledKernelModules[index];\n}\n");

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

    fprintf(file, "ORYN_KERNEL_MODULE_MANIFEST_GENERATION_V2\n");
    fprintf(file, "PackageVersion=%s\n", ORYN_VERSION);
    fprintf(file, "SourceRoot=Common/Kernel/ModuleManifests\n");
    fprintf(file, "GeneratedHeader=Common/Kernel/Include/KernelModuleManifest.h\n");
    fprintf(file, "GeneratedData=Common/Kernel/Source/Manifest/KernelModuleManifestData.c\n");
    fprintf(file, "ModuleCount=%d\n", module_count);
    fprintf(file, "Fields=Order<TAB>Id<TAB>Name<TAB>Requires<TAB>Select<TAB>CompiledIn<TAB>Required<TAB>FatalOnMissingPrerequisite<TAB>StopCallback<TAB>PanicCallback<TAB>ShutdownCallback<TAB>SourcePath\n");
    fprintf(file, "Modules:\n");
    for (int index = 0; index < module_count; ++index)
    {
        fprintf(file, "%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%s\t%s\t%s\t%s\n",
            modules[index].Order,
            modules[index].Id,
            modules[index].Name,
            modules[index].Requires,
            modules[index].Select,
            modules[index].CompiledIn,
            modules[index].Required,
            modules[index].FatalOnMissingPrerequisite,
            modules[index].StopCallback,
            modules[index].PanicCallback,
            modules[index].ShutdownCallback,
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

    LogBuildPlanDecision(project, "kernel-module-manifest", "Generated kernel module C tables from per-module manifest files with compiled-in registry, boot policy, and lifecycle callbacks.");
    return 1;
}
