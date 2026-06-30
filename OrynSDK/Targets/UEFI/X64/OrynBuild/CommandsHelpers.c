#include "TargetBuildInternal.h"
#include "OrynBuild.h"
#include "CommandsSupport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int TextEqualsIgnoreCaseCommand(const char* left, const char* right)
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (*left != 0 && *right != 0)
    {
        char a = *left;
        char b = *right;
        if (a >= 'A' && a <= 'Z')
        {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z')
        {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b)
        {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == 0 && *right == 0;
}

int ProjectBoolEnabled(const char* value, int default_value)
{
    if (value == 0 || value[0] == 0)
    {
        return default_value;
    }

    if (TextEqualsIgnoreCaseCommand(value, "on") ||
        TextEqualsIgnoreCaseCommand(value, "yes") ||
        TextEqualsIgnoreCaseCommand(value, "true") ||
        TextEqualsIgnoreCaseCommand(value, "1") ||
        TextEqualsIgnoreCaseCommand(value, "enabled"))
    {
        return 1;
    }

    if (TextEqualsIgnoreCaseCommand(value, "off") ||
        TextEqualsIgnoreCaseCommand(value, "no") ||
        TextEqualsIgnoreCaseCommand(value, "false") ||
        TextEqualsIgnoreCaseCommand(value, "0") ||
        TextEqualsIgnoreCaseCommand(value, "disabled"))
    {
        return 0;
    }

    return default_value;
}

unsigned int ProjectCpuCount(const OrynProject* project)
{
    char* end = 0;
    unsigned long value = strtoul(project->run_smp, &end, 10);
    if (end == project->run_smp || *end != 0 || value == 0UL)
    {
        return 1U;
    }
    if (value > 64UL)
    {
        value = 64UL;
    }
    return (unsigned int)value;
}

const char* OnOffText(int value)
{
    return value ? "on" : "off";
}

int IsInteractiveDisplayMode(const char* display_mode)
{
    if (display_mode == 0 || display_mode[0] == 0)
    {
        return 0;
    }

    return !TextEqualsIgnoreCaseCommand(display_mode, "none");
}

unsigned int ResolveQemuHeadlessTimeoutSeconds(void)
{
    const char* env = getenv("ORYN_QEMU_HEADLESS_TIMEOUT_SECONDS");
    char* end = 0;
    unsigned long value;

    if (env == 0 || env[0] == 0)
    {
        return 45U;
    }

    value = strtoul(env, &end, 10);
    if (end == env || *end != 0 || value < 5UL)
    {
        return 45U;
    }

    if (value > 600UL)
    {
        value = 600UL;
    }

    return (unsigned int)value;
}

const char* ResolveQemuCpuModel(const char* configured_cpu)
{
    if (configured_cpu == 0 || configured_cpu[0] == 0)
    {
        return "qemu64";
    }

    if (TextEqualsIgnoreCaseCommand(configured_cpu, "host") ||
        TextEqualsIgnoreCaseCommand(configured_cpu, "native"))
    {
        return "max";
    }

    return configured_cpu;
}

int QemuCpuWasTranslated(const char* configured_cpu, const char* resolved_cpu)
{
    return configured_cpu != 0 && resolved_cpu != 0 &&
        !TextEqualsIgnoreCaseCommand(configured_cpu, resolved_cpu);
}

int DebugTextIsEmpty(const char* text)
{
    return text == 0 || text[0] == 0;
}

int IsSafeQemuArgumentValue(const char* value)
{
    if (value == 0 || value[0] == 0)
    {
        return 0;
    }

    for (int index = 0; value[index] != 0; ++index)
    {
        char ch = value[index];
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_' || ch == '-' || ch == '+' || ch == ',' ||
              ch == '=' || ch == ':' || ch == '.'))
        {
            return 0;
        }
    }

    return 1;
}

void BuildProjectImagePath(const OrynProject* project, char* output, size_t output_size)
{
    char image_name[256];
    snprintf(image_name, sizeof(image_name), "%s.img", project->name);
    OrynJoinPath(output, output_size, project->output_dir, image_name);
}

