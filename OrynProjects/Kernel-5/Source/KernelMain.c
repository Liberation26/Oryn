#include "OrynBootInfo.h"
#include <stdbool.h>
#include <stdio.h>

bool KernelMain(const OrynBootInfo* bootInfo)
{
    int written;

    (void)bootInfo;

    written = printf("Hello World\n");
    if (written < 0)
    {
        return false;
    }

    return true;
}
