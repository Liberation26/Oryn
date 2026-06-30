#include "OrynBuild.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ORYN_COLOR_RESET "\033[0m"
#define ORYN_COLOR_INFO "\033[36m"
#define ORYN_COLOR_STEP "\033[35m"
#define ORYN_COLOR_OK "\033[32m"
#define ORYN_COLOR_WARN "\033[33m"
#define ORYN_COLOR_FAIL "\033[31m"
#define ORYN_COLOR_CMD "\033[34m"

static int OrynUseColor(void)
{
    const char* no_color = getenv("NO_COLOR");
    const char* oryn_no_color = getenv("ORYN_NO_COLOR");

    if (no_color != 0 && no_color[0] != 0)
    {
        return 0;
    }

    if (oryn_no_color != 0 && strcmp(oryn_no_color, "1") == 0)
    {
        return 0;
    }

    return 1;
}

static void OrynLogTag(FILE* stream, const char* color, const char* tag, const char* message)
{
    if (OrynUseColor())
    {
        fprintf(stream, "%s%s%s %s\n", color, tag, ORYN_COLOR_RESET, message);
    }
    else
    {
        fprintf(stream, "%s %s\n", tag, message);
    }
}

void OrynLogInfo(const char* message)
{
    OrynLogTag(stdout, ORYN_COLOR_INFO, "[INFO]", message);
}

void OrynLogStep(const char* message)
{
    OrynLogTag(stdout, ORYN_COLOR_STEP, "[STEP]", message);
}

void OrynLogOk(const char* message)
{
    OrynLogTag(stdout, ORYN_COLOR_OK, "[ OK ]", message);
}

void OrynLogWarn(const char* message)
{
    OrynLogTag(stdout, ORYN_COLOR_WARN, "[WARN]", message);
}

void OrynLogFail(const char* message)
{
    OrynLogTag(stderr, ORYN_COLOR_FAIL, "[FAIL]", message);
}

void OrynLogKeyValue(const char* key, const char* value)
{
    if (OrynUseColor())
    {
        printf("%s[INFO]%s %-16s %s\n", ORYN_COLOR_INFO, ORYN_COLOR_RESET, key, value);
    }
    else
    {
        printf("[INFO] %-16s %s\n", key, value);
    }
}

void OrynLogCommand(const char* command)
{
    if (OrynUseColor())
    {
        printf("%s[CMD ]%s %s\n", ORYN_COLOR_CMD, ORYN_COLOR_RESET, command);
    }
    else
    {
        printf("[CMD ] %s\n", command);
    }
}
