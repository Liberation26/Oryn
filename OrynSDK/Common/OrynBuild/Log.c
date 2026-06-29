#include "OrynBuild.h"
#include <stdio.h>

void OrynLogInfo(const char* message)
{
    printf("[INFO] %s\n", message);
}

void OrynLogStep(const char* message)
{
    printf("[STEP] %s\n", message);
}

void OrynLogOk(const char* message)
{
    printf("[ OK ] %s\n", message);
}

void OrynLogWarn(const char* message)
{
    printf("[WARN] %s\n", message);
}

void OrynLogFail(const char* message)
{
    printf("[FAIL] %s\n", message);
}

void OrynLogKeyValue(const char* key, const char* value)
{
    printf("[INFO] %-16s %s\n", key, value);
}
