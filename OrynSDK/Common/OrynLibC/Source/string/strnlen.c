#include "string.h"

size_t strnlen(const char* text, size_t max_size)
{
    size_t length = 0U;
    while (length < max_size && text[length] != 0) { ++length; }
    return length;
}
