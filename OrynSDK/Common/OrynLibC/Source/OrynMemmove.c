#include "OrynString.h"

void* OrynMemmove(void* target, const void* source, size_t size)
{
    return memmove(target, source, size);
}
