#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>

int OrynPrereqRun(const char* executable_path, const char* mode);

static void PrintPrereqUsage(void)
{
    printf("Oryn prerequisite installer/app %s\n", ORYN_VERSION);
    printf("Usage:\n");
    printf("  oryn prerequisites check      Check every required host tool\n");
    printf("  oryn prerequisites plan       Show the installer command only\n");
    printf("  oryn prerequisites install    Install missing provider-managed tools\n");
    printf("  oryn prerequisites manifest   Print the required tool manifest\n");
    printf("Aliases:\n");
    printf("  oryn prereq <mode>\n");
}

int OrynCommandPrerequisites(const char* executable_path, int argument_count, char** arguments)
{
    const char* mode = "check";
    (void)executable_path;

    if (argument_count > 0)
    {
        mode = arguments[0];
    }

    if (strcmp(mode, "help") == 0 || strcmp(mode, "--help") == 0 || strcmp(mode, "-h") == 0)
    {
        PrintPrereqUsage();
        return 0;
    }

    if (strcmp(mode, "check") != 0 &&
        strcmp(mode, "plan") != 0 &&
        strcmp(mode, "install") != 0 &&
        strcmp(mode, "manifest") != 0)
    {
        OrynLogFail("Unknown prerequisites mode.");
        PrintPrereqUsage();
        return 1;
    }

    return OrynPrereqRun(executable_path, mode);
}
