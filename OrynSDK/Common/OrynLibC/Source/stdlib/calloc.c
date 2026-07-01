#include "OrynLibCAllocatorState.h"
#include "stdlib.h"
#include "string.h"

void* calloc(size_t count, size_t size)
{
    size_t total = count * size;
    void* memory = malloc(total);
    if (memory != 0) { memset(memory, 0, total); }
    return memory;
}
