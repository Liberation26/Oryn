#include "OrynLibCAllocatorState.h"
#include "stdlib.h"

void free(void* memory)
{
    if (OrynLibCAllocatorFreeProc) { OrynLibCAllocatorFreeProc(memory); }
}
