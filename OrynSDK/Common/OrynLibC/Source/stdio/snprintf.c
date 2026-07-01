#include "stdarg.h"
#include "stddef.h"
#include "stdio.h"

int snprintf(char* restrict target, size_t size, const char* restrict format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsnprintf(target, size, format, args);
    va_end(args);
    return result;
}
