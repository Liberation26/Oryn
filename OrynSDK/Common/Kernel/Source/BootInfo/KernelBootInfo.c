#include "KernelBootInfo.h"

int KernelBootInfoHasFlag(const OrynBootInfo* bootInfo, unsigned long long flag)
{
    if (bootInfo == 0)
    {
        return 0;
    }

    return (bootInfo->Flags & flag) != 0ULL;
}
