#ifndef ORYN_LIBC_STDIO_H
#define ORYN_LIBC_STDIO_H

#include "stddef.h"
#include "stdarg.h"

#ifndef EOF
#define EOF (-1)
#endif

int snprintf(char* restrict target, size_t size, const char* restrict format, ...);
int vsnprintf(char* restrict target, size_t size, const char* restrict format, va_list args);

#endif
