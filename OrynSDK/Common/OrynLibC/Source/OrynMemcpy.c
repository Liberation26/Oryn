#include "OrynString.h"

void* OrynMemcpy(void* target, const void* source, size_t size)
{
    return memcpy(target, source, size);
}
