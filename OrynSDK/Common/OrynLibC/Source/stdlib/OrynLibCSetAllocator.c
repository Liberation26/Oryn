#include "OrynLibCAllocatorState.h"

void OrynLibCSetAllocator(OrynLibCMallocProc malloc_proc,
    OrynLibCFreeProc free_proc,
    OrynLibCReallocProc realloc_proc)
{
    OrynLibCAllocatorMallocProc = malloc_proc;
    OrynLibCAllocatorFreeProc = free_proc;
    OrynLibCAllocatorReallocProc = realloc_proc;
}
