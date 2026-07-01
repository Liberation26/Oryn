#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

static void AddRequire(OrynBuildModule* module, const char* require_name, size_t length)
{
    if (length == 0U || module->RequireCount >= ORYN_MAX_MODULE_REQUIRES)
    {
        return;
    }

    snprintf(module->Requires[module->RequireCount], sizeof(module->Requires[module->RequireCount]), "%.*s", (int)length, require_name);
    module->RequireCount += 1;
}

static void DescribeRequires(const OrynBuildModule* module, char* output, size_t output_size)
{
    output[0] = 0;
    if (module->RequireCount == 0)
    {
        snprintf(output, output_size, "none");
        return;
    }

    for (int index = 0; index < module->RequireCount; ++index)
    {
        strncat(output, index == 0 ? "" : ",", output_size - strlen(output) - 1U);
        strncat(output, module->Requires[index], output_size - strlen(output) - 1U);
    }
}

static void ParseRequireList(OrynBuildModule* module, const char* requires)
{
    if (requires == 0 || requires[0] == 0)
    {
        return;
    }

    const char* start = requires;
    const char* current = requires;
    while (1)
    {
        if (*current == ',' || *current == 0)
        {
            const char* left = start;
            const char* right = current;
            while (left < right && (*left == ' ' || *left == '\t'))
            {
                ++left;
            }
            while (right > left && (right[-1] == ' ' || right[-1] == '\t'))
            {
                --right;
            }
            AddRequire(module, left, (size_t)(right - left));
            if (*current == 0)
            {
                break;
            }
            start = current + 1;
        }
        ++current;
    }
}

int AddBuildModule(OrynBuildArchivePlan* plan, const OrynProject* project, const char* name, const char* source_root, int recursive, const char* requires)
{
    if (plan->ModuleCount >= ORYN_MAX_BUILD_MODULES)
    {
        OrynLogFail("Too many build modules were discovered.");
        return 0;
    }

    OrynBuildModule* module = &plan->Modules[plan->ModuleCount];
    memset(module, 0, sizeof(*module));
    snprintf(module->Name, sizeof(module->Name), "%s", name);
    snprintf(module->SourceRoot, sizeof(module->SourceRoot), "%s", source_root);
    module->Recursive = recursive;
    module->Sources = (OrynStringList*)calloc(1U, sizeof(OrynStringList));
    module->Objects = (OrynStringList*)calloc(1U, sizeof(OrynStringList));
    if (module->Sources == 0 || module->Objects == 0)
    {
        OrynLogFail("Could not allocate build module source lists.");
        return 0;
    }
    module->Present = OrynDirectoryExists(source_root);
    BuildArchivePath(project, name, module->ArchivePath, sizeof(module->ArchivePath));
    ParseRequireList(module, requires);

    char require_text[512];
    char detail[ORYN_MAX_PATH + 768];
    DescribeRequires(module, require_text, sizeof(require_text));
    snprintf(detail, sizeof(detail),
        "module=%s present=%s recursive=%s source-root=%s requires=%s archive=%s",
        module->Name,
        module->Present ? "yes" : "no",
        module->Recursive ? "yes" : "no",
        module->SourceRoot,
        require_text,
        module->ArchivePath);
    LogBuildPlanDecision(project, "module-discovered", detail);
    if (!module->Present)
    {
        LogBuildPlanSkip(project, "module-source-root-missing", detail);
    }

    plan->ModuleCount += 1;
    return 1;
}
