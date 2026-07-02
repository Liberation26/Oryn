#include "TargetBuildInternal.h"
#include <stdlib.h>

int ProjectBoolEnabledBuild(const char* value, int default_value)
{
    if (value == 0 || value[0] == 0)
    {
        return default_value;
    }

    if (TextEqualsIgnoreCaseBuild(value, "on") ||
        TextEqualsIgnoreCaseBuild(value, "yes") ||
        TextEqualsIgnoreCaseBuild(value, "true") ||
        TextEqualsIgnoreCaseBuild(value, "1") ||
        TextEqualsIgnoreCaseBuild(value, "enabled"))
    {
        return 1;
    }

    if (TextEqualsIgnoreCaseBuild(value, "off") ||
        TextEqualsIgnoreCaseBuild(value, "no") ||
        TextEqualsIgnoreCaseBuild(value, "false") ||
        TextEqualsIgnoreCaseBuild(value, "0") ||
        TextEqualsIgnoreCaseBuild(value, "disabled"))
    {
        return 0;
    }

    return default_value;
}

unsigned int ProjectCpuCountBuild(const OrynProject* project)
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

int ProjectStorageInterfaceIsVirtioBlockBuild(const OrynProject* project)
{
    return TextEqualsIgnoreCaseBuild(project->run_storage_interface, "virtio") ||
        TextEqualsIgnoreCaseBuild(project->run_storage_interface, "virtio-blk") ||
        TextEqualsIgnoreCaseBuild(project->run_storage_interface, "virtio_blk");
}

int ProjectDisplayIsInteractiveBuild(const OrynProject* project)
{
    const char* display = project->run_display;
    if (display == 0 || display[0] == 0)
    {
        return 0;
    }

    if (TextEqualsIgnoreCaseBuild(display, "none") ||
        TextEqualsIgnoreCaseBuild(display, "headless") ||
        TextEqualsIgnoreCaseBuild(display, "yes") ||
        TextEqualsIgnoreCaseBuild(display, "true"))
    {
        return 0;
    }

    return 1;
}
