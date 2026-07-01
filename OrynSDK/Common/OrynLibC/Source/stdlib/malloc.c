#include "OrynLibCAllocatorState.h"
#include "stdlib.h"

void* malloc(size_t size)
{
    return OrynLibCAllocatorMallocProc ? OrynLibCAllocatorMallocProc(size) : 0;
}
