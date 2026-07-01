#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

void BuildArchiveDirectory(const OrynProject* project, char* output, size_t output_size)
{
    OrynJoinPath(output, output_size, project->build_dir, "Archives");
}

static void SanitizeArchiveName(char* output, size_t output_size, const char* module_name)
{
    size_t out = 0U;
    for (size_t index = 0U; module_name[index] != 0 && out + 1U < output_size; ++index)
    {
        char ch = module_name[index];
        if ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9'))
        {
            output[out++] = ch;
        }
        else
        {
            output[out++] = '_';
        }
    }

    output[out] = 0;
}

void BuildArchivePath(const OrynProject* project, const char* module_name, char* output, size_t output_size)
{
    char archive_dir[ORYN_MAX_PATH];
    char safe_name[256];
    char archive_file[320];

    BuildArchiveDirectory(project, archive_dir, sizeof(archive_dir));
    SanitizeArchiveName(safe_name, sizeof(safe_name), module_name);
    snprintf(archive_file, sizeof(archive_file), "lib%s.a", safe_name);
    OrynJoinPath(output, output_size, archive_dir, archive_file);
}
