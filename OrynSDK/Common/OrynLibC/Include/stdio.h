#ifndef ORYN_LIBC_STDIO_H
#define ORYN_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* restrict format, ...);
int vprintf(const char* restrict format, va_list args);
int snprintf(char* restrict target, size_t size, const char* restrict format, ...);
int vsnprintf(char* restrict target, size_t size, const char* restrict format, va_list args);

#ifdef __cplusplus
}
#endif

#endif
