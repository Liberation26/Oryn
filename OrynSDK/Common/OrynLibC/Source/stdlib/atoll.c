#include "stdlib.h"

long long atoll(const char* text)
{
    return strtoll(text, 0, 10);
}
