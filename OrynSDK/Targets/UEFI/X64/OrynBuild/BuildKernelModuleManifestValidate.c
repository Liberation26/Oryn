#include "TargetBuildInternal.h"

#define ORYN_MANIFEST_VALIDATE_VISITING 1
#define ORYN_MANIFEST_VALIDATE_DONE 2

static void StripValidationToken(char* text)
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

static int FindKernelManifestIndex(const OrynKernelModuleManifestSource* modules, int module_count, const char* id)
{
    for (int index = 0; index < module_count; ++index)
    {
        if (strcmp(modules[index].Id, id) == 0)
        {
            return index;
        }
    }

    return -1;
}

static int ManifestRequiresId(const OrynKernelModuleManifestSource* module, const char* id)
{
    char buffer[ORYN_MAX_KERNEL_MANIFEST_REQUIRE_TEXT];
    snprintf(buffer, sizeof(buffer), "%s", module->Requires);
    char* token = strtok(buffer, ",");
    while (token != 0)
    {
        StripValidationToken(token);
        if (strcmp(token, id) == 0)
        {
            return 1;
        }
        token = strtok(0, ",");
    }

    return 0;
}

static int ValidateRequiredEdge(const OrynProject* project, const OrynKernelModuleManifestSource* modules, int module_count, const char* module_id, const char* required_id)
{
    int module_index = FindKernelManifestIndex(modules, module_count, module_id);
    if (module_index < 0)
    {
        return 1;
    }

    if (ManifestRequiresId(&modules[module_index], required_id))
    {
        return 1;
    }

    char message[256];
    snprintf(message, sizeof(message), "Manifest %s must require %s.", module_id, required_id);
    OrynLogFail(message);
    LogBuildPlanSkip(project, "manifest-required-edge", message);
    return 0;
}

static int ValidateRequirementList(const OrynProject* project, const OrynKernelModuleManifestSource* modules, int module_count, int module_index)
{
    char buffer[ORYN_MAX_KERNEL_MANIFEST_REQUIRE_TEXT];
    snprintf(buffer, sizeof(buffer), "%s", modules[module_index].Requires);
    char* token = strtok(buffer, ",");
    int require_count = 0;
    while (token != 0)
    {
        StripValidationToken(token);
        if (token[0] != 0)
        {
            require_count += 1;
            int require_index = FindKernelManifestIndex(modules, module_count, token);
            if (require_index < 0)
            {
                char message[384];
                snprintf(message, sizeof(message), "Manifest %s requires missing manifest %s.", modules[module_index].Id, token);
                OrynLogFail(message);
                LogBuildPlanSkip(project, "manifest-missing-prerequisite", message);
                return 0;
            }

            if (require_index == module_index)
            {
                char message[256];
                snprintf(message, sizeof(message), "Manifest %s cannot require itself.", modules[module_index].Id);
                OrynLogFail(message);
                LogBuildPlanSkip(project, "manifest-self-prerequisite", message);
                return 0;
            }

            if (require_index > module_index)
            {
                char message[384];
                snprintf(message, sizeof(message), "Manifest order invalid: %s must be before %s.", token, modules[module_index].Id);
                OrynLogFail(message);
                LogBuildPlanSkip(project, "manifest-prerequisite-order", message);
                return 0;
            }
        }
        token = strtok(0, ",");
    }

    if (require_count > 6)
    {
        char message[256];
        snprintf(message, sizeof(message), "Manifest %s has more than 6 prerequisites.", modules[module_index].Id);
        OrynLogFail(message);
        LogBuildPlanSkip(project, "manifest-prerequisite-count", message);
        return 0;
    }

    return 1;
}

static int ValidateManifestCycleVisit(const OrynProject* project, const OrynKernelModuleManifestSource* modules, int module_count, int module_index, int* states)
{
    if (states[module_index] == ORYN_MANIFEST_VALIDATE_DONE)
    {
        return 1;
    }

    if (states[module_index] == ORYN_MANIFEST_VALIDATE_VISITING)
    {
        char message[256];
        snprintf(message, sizeof(message), "Manifest prerequisite cycle at %s.", modules[module_index].Id);
        OrynLogFail(message);
        LogBuildPlanSkip(project, "manifest-prerequisite-cycle", message);
        return 0;
    }

    states[module_index] = ORYN_MANIFEST_VALIDATE_VISITING;
    char buffer[ORYN_MAX_KERNEL_MANIFEST_REQUIRE_TEXT];
    snprintf(buffer, sizeof(buffer), "%s", modules[module_index].Requires);
    char* save_pointer = 0;
    char* token = strtok_r(buffer, ",", &save_pointer);
    while (token != 0)
    {
        StripValidationToken(token);
        int require_index = FindKernelManifestIndex(modules, module_count, token);
        if (require_index >= 0 && !ValidateManifestCycleVisit(project, modules, module_count, require_index, states))
        {
            return 0;
        }
        token = strtok_r(0, ",", &save_pointer);
    }

    states[module_index] = ORYN_MANIFEST_VALIDATE_DONE;
    return 1;
}

int ValidateKernelModuleManifests(const OrynProject* project)
{
    OrynKernelModuleManifestSource modules[ORYN_MAX_KERNEL_MODULE_MANIFESTS];
    int module_count = 0;
    OrynLogStep("Validating kernel module manifests.");
    if (!ReadKernelModuleManifestSources(project, modules, &module_count))
    {
        return 0;
    }

    int states[ORYN_MAX_KERNEL_MODULE_MANIFESTS];
    memset(states, 0, sizeof(states));
    for (int index = 0; index < module_count; ++index)
    {
        if (!ValidateRequirementList(project, modules, module_count, index))
        {
            return 0;
        }
        if (!ValidateManifestCycleVisit(project, modules, module_count, index, states))
        {
            return 0;
        }
    }

    int ok = ValidateRequiredEdge(project, modules, module_count, "OrynKernelModulePic", "OrynKernelModuleCpu") &&
        ValidateRequiredEdge(project, modules, module_count, "OrynKernelModulePic", "OrynKernelModuleInterrupts") &&
        ValidateRequiredEdge(project, modules, module_count, "OrynKernelModuleApic", "OrynKernelModuleCpu") &&
        ValidateRequiredEdge(project, modules, module_count, "OrynKernelModuleApic", "OrynKernelModuleInterrupts") &&
        ValidateRequiredEdge(project, modules, module_count, "OrynKernelModuleApic", "OrynKernelModulePic");

    if (!ok)
    {
        return 0;
    }

    char message[160];
    snprintf(message, sizeof(message), "Kernel manifest validation passed for %d module(s).", module_count);
    OrynLogOk(message);
    LogBuildPlanDecision(project, "manifest-validation", message);
    return 1;
}
