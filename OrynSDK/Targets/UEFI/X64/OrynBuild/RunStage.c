#include "OrynBuild.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int OrynResolveWindowsStageRoot(char* output, size_t output_size)
{
    const char* env = getenv("ORYN_QEMU_STAGE_DIR");
    if (env != 0 && env[0] != 0)
    {
        if (strchr(env, ':') != 0 && strstr(env, "/mnt/") != env)
        {
            if (!OrynConvertWindowsPathToWsl(env, output, output_size))
            {
                return 0;
            }
        }
        else
        {
            snprintf(output, output_size, "%s", env);
        }

        return OrynMakeDirectoryRecursive(output);
    }

    const char* candidates[] =
    {
        "/mnt/c/Users/daves/AppData/Local/Temp/OrynWSL",
        "/mnt/c/Temp/OrynWSL",
        0
    };

    for (int index = 0; candidates[index] != 0; ++index)
    {
        if (OrynMakeDirectoryRecursive(candidates[index]))
        {
            snprintf(output, output_size, "%s", candidates[index]);
            return 1;
        }
    }

    output[0] = 0;
    return 0;
}

void OrynMakeStageFilePath(char* output, size_t output_size, const char* stage_root, const char* project_name, const char* suffix)
{
    char name[256];
    size_t used = 0;

    for (const char* current = project_name; *current != 0 && used + 1 < sizeof(name); ++current)
    {
        char ch = *current;
        if ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-')
        {
            name[used++] = ch;
        }
        else
        {
            name[used++] = '_';
        }
    }

    if (used == 0)
    {
        snprintf(name, sizeof(name), "OrynProject");
    }
    else
    {
        name[used] = 0;
    }

    snprintf(output, output_size, "%s/%s%s", stage_root, name, suffix);
}
