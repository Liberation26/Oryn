#include "stdarg.h"
#include "stdio.h"

int printf(const char* restrict format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);

    return result;
}
