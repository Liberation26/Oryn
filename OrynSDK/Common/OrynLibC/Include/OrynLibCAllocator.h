#ifndef ORYN_LIBC_ALLOCATOR_H
#define ORYN_LIBC_ALLOCATOR_H

#include "stddef.h"

typedef void* (*OrynLibCMallocProc)(size_t size);
typedef void (*OrynLibCFreeProc)(void* memory);
typedef void* (*OrynLibCReallocProc)(void* memory, size_t size);

void OrynLibCSetAllocator(OrynLibCMallocProc malloc_proc,
    OrynLibCFreeProc free_proc,
    OrynLibCReallocProc realloc_proc);
int OrynLibCHasAllocator(void);

#endif
