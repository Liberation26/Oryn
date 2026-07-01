#include "OrynString.h"

int OrynStrncmp(const char* left, const char* right, size_t size)
{
    return strncmp(left, right, size);
}
