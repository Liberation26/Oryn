#include "TargetBuildInternal.h"
void BuildQemuStageProjectName(const OrynProject* project, char* output, size_t output_size)
{
    char output_base[ORYN_MAX_PATH];
    OrynGetBaseName(output_base, sizeof(output_base), project->output_dir);

    if (output_base[0] != 0 && strcmp(output_base, "Output") != 0)
    {
        snprintf(output, output_size, "%s-%s", project->name, output_base);
        return;
    }

    snprintf(output, output_size, "%s", project->name);
}

