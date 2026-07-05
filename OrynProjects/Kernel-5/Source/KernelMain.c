#include "OrynBootInfo.h"
#include <stdio.h>

int KernelMain(const OrynBootInfo* bootInfo)
{
    int written;

    (void)bootInfo;

    written = printf("Hello World\n");
    if (written < 0)
    {
        return 1;
    }

    return 0;
}
