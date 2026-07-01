#include "OrynLibCAllocator.h"
#include "errno.h"
#include "stdlib.h"
#include "string.h"

typedef struct OrynAllocHeader
{
    size_t Size;
} OrynAllocHeader;

static OrynLibCMallocProc gMalloc = 0;
static OrynLibCFreeProc gFree = 0;
static OrynLibCReallocProc gRealloc = 0;

void OrynLibCSetAllocator(OrynLibCMallocProc malloc_proc,
    OrynLibCFreeProc free_proc,
    OrynLibCReallocProc realloc_proc)
{
    gMalloc = malloc_proc;
    gFree = free_proc;
    gRealloc = realloc_proc;
}

int OrynLibCHasAllocator(void)
{
    return gMalloc != 0 && gFree != 0;
}

void* malloc(size_t size)
{
    if (gMalloc == 0) { errno = ENOMEM; return 0; }
    return gMalloc(size);
}

void free(void* memory)
{
    if (gFree != 0) { gFree(memory); }
}

void* calloc(size_t count, size_t size)
{
    void* memory;
    if (size != 0U && count > ((size_t)-1) / size) { errno = ENOMEM; return 0; }
    memory = malloc(count * size);
    if (memory != 0) { memset(memory, 0, count * size); }
    return memory;
}

void* realloc(void* memory, size_t size)
{
    if (gRealloc != 0) { return gRealloc(memory, size); }
    if (memory == 0) { return malloc(size); }
    if (size == 0U) { free(memory); return 0; }
    errno = ENOMEM;
    return 0;
}
