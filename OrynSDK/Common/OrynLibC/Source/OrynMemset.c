#include "OrynString.h"

void* OrynMemset(void* target, int value, size_t size)
{
    return memset(target, value, size);
}
