#include "OrynLibCAllocatorState.h"
#include "stdlib.h"

void* realloc(void* memory, size_t size)
{
    return OrynLibCAllocatorReallocProc ? OrynLibCAllocatorReallocProc(memory, size) : 0;
}
