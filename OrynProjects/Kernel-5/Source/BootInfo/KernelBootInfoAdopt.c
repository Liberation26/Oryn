#include "KernelBootInfo.h"

static OrynBootInfo gKernelBootInfo;
static unsigned long long gKernelBootInfoSourceAddress;
static int gKernelBootInfoCopied;

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;

    for (unsigned long long index = 0ULL; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static void CopyBytes(void* target, const void* source, unsigned long long count)
{
    unsigned char* destination = (unsigned char*)target;
    const unsigned char* input = (const unsigned char*)source;

    for (unsigned long long index = 0ULL; index < count; ++index)
    {
        destination[index] = input[index];
    }
}

static unsigned long long BootInfoCopySize(const OrynBootInfo* bootInfo)
{
    unsigned long long copySize = (unsigned long long)sizeof(gKernelBootInfo);

    if (bootInfo->Size != 0U && (unsigned long long)bootInfo->Size < copySize)
    {
        copySize = (unsigned long long)bootInfo->Size;
    }

    return copySize;
}

const OrynBootInfo* KernelBootInfoAdopt(const OrynBootInfo* bootInfo)
{
    ClearBytes(&gKernelBootInfo, (unsigned long long)sizeof(gKernelBootInfo));
    gKernelBootInfoSourceAddress = (unsigned long long)bootInfo;
    gKernelBootInfoCopied = 0;

    if (bootInfo == 0)
    {
        return 0;
    }

    CopyBytes(&gKernelBootInfo, bootInfo, BootInfoCopySize(bootInfo));
    gKernelBootInfoCopied = 1;
    return &gKernelBootInfo;
}

int KernelBootInfoIsKernelOwned(const OrynBootInfo* bootInfo)
{
    return gKernelBootInfoCopied != 0 && bootInfo == &gKernelBootInfo;
}

unsigned long long KernelBootInfoSourceAddress(void)
{
    return gKernelBootInfoSourceAddress;
}
