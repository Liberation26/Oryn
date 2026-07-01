#include "OrynLibCAllocatorState.h"

int OrynLibCHasAllocator(void)
{
    return OrynLibCAllocatorMallocProc != 0 && OrynLibCAllocatorFreeProc != 0;
}
