#include "stdlib.h"

long atol(const char* text)
{
    return strtol(text, 0, 10);
}
