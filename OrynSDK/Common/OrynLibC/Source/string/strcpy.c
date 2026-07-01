#include "string.h"

char* strcpy(char* restrict target, const char* restrict source)
{
    char* start = target;
    while ((*target++ = *source++) != 0) { }
    return start;
}
