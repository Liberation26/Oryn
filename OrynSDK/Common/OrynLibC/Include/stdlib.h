#ifndef ORYN_LIBC_STDLIB_H
#define ORYN_LIBC_STDLIB_H

#include "stddef.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

int atoi(const char* text);
long strtol(const char* restrict text, char** restrict end, int base);
unsigned long strtoul(const char* restrict text, char** restrict end, int base);
unsigned long long strtoull(const char* restrict text, char** restrict end, int base);
int abs(int value);
long labs(long value);
long long llabs(long long value);
div_t div(int numerator, int denominator);
ldiv_t ldiv(long numerator, long denominator);
lldiv_t lldiv(long long numerator, long long denominator);
void abort(void) __attribute__((noreturn));

#endif
