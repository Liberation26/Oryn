
#include "OrynBuild.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ResolveExecutable(const char* executable_path, char* output, size_t output_size)
{
    if (executable_path != 0 && strchr(executable_path, '/') != 0)
    {
        if (realpath(executable_path, output) != 0)
        {
            return 1;
        }
    }

    if (OrynFindProgram(executable_path, output, output_size))
    {
        return 1;
    }

    snprintf(output, output_size, "%s", executable_path ? executable_path : "oryn");
    return 0;
}

static void ResolveSdkRootFromExecutable(const char* executable_path, char* output, size_t output_size)
{
    char resolved[ORYN_MAX_PATH];
    char bin_dir[ORYN_MAX_PATH];
    char common_dir[ORYN_MAX_PATH];

    if (!ResolveExecutable(executable_path, resolved, sizeof(resolved)))
    {
        if (getcwd(output, output_size) != 0)
        {
            return;
        }
        snprintf(output, output_size, ".");
        return;
    }

    OrynGetDirectoryName(bin_dir, sizeof(bin_dir), resolved);
    OrynGetDirectoryName(common_dir, sizeof(common_dir), bin_dir);
    OrynGetDirectoryName(output, output_size, common_dir);
}

int OrynCommandLibCConformance(const char* executable_path)
{
    char sdk_root[ORYN_MAX_PATH];
    char script[ORYN_MAX_PATH];
    char command[ORYN_MAX_PATH * 2];
    char quoted_script[ORYN_MAX_PATH * 2];
    int result;

    ResolveSdkRootFromExecutable(executable_path, sdk_root, sizeof(sdk_root));
    OrynJoinPath(script, sizeof(script), sdk_root, "Common/Scripts/RunLibCHostConformance.sh");

    if (!OrynFileExists(script))
    {
        OrynLogFail("LibC host conformance script is missing.");
        OrynLogKeyValue("Expected", script);
        return 1;
    }

    OrynShellQuote(quoted_script, sizeof(quoted_script), script);
    snprintf(command, sizeof(command), "bash %s", quoted_script);

    OrynLogStep("Running external host-side OrynLibC conformance tests.");
    OrynLogCommand(command);
    result = OrynRunCommand(command);

    if (!result)
    {
        OrynLogFail("OrynLibC host conformance tests failed.");
        return 1;
    }

    OrynLogOk("OrynLibC host conformance tests passed.");
    return 0;
}
