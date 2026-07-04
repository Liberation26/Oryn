#include "TargetBuildInternal.h"

#define ORYN_MANIFEST_REQUIRE_LIMIT 6U

static void StripManifestToken(char* text)
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

static void WriteRequiresArray(FILE* file, const char* id, const char* requires)
{
    fprintf(file, "    static const OrynKernelModuleId requires_%s[] = { ", id);
    if (requires[0] == 0)
    {
        fprintf(file, "OrynKernelModuleCount");
    }
    else
    {
        char buffer[ORYN_MAX_KERNEL_MANIFEST_REQUIRE_TEXT];
        snprintf(buffer, sizeof(buffer), "%s", requires);
        char* token = strtok(buffer, ",");
        int written = 0;
        while (token)
        {
            StripManifestToken(token);
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
    fprintf(file, "int OrynKernelModuleManifestBeginProof(OrynKernelModuleId id);\n");
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
    fprintf(file, "void OrynKernelModuleManifestTransitionProof(void);\n");
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
    fprintf(file, "SourceRoots=Common/Kernel/Source;Common/OrynLibC;Targets/UEFI/X64/KernelSupport/Source\n");
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

    LogBuildPlanDecision(project, "kernel-module-manifest", "Generated kernel module C tables from per-module manifest files with compiled-in registry, beside-module ownership, boot policy, and lifecycle callbacks.");
    return 1;
}
