#include "TargetBuildInternal.h"
#include <stdio.h>

void BuildObjectFileName(const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size)
{
    char base_name[ORYN_MAX_PATH];
    char stem[ORYN_MAX_PATH];
    char object_name[ORYN_MAX_PATH];
    unsigned long long path_hash = ComputePathHash(source_file);

    OrynGetBaseName(base_name, sizeof(base_name), source_file);
    OrynReplaceExtension(stem, sizeof(stem), base_name, "");
    snprintf(object_name, sizeof(object_name), "%s-%016llX.o", stem, path_hash);
    OrynJoinPath(object_file, object_file_size, project->object_dir, object_name);
}

void BuildObjectSidecarPath(char* output, size_t output_size, const char* object_file, const char* extension)
{
    snprintf(output, output_size, "%s%s", object_file, extension);
}
