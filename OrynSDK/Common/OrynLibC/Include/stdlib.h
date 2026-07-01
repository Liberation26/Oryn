#ifndef ORYN_LIBC_STDLIB_H
#define ORYN_LIBC_STDLIB_H

#include "stddef.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX 32767

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;

int atoi(const char* text);
long atol(const char* text);
long long atoll(const char* text);
long strtol(const char* restrict text, char** restrict end, int base);
long long strtoll(const char* restrict text, char** restrict end, int base);
unsigned long strtoul(const char* restrict text, char** restrict end, int base);
unsigned long long strtoull(const char* restrict text, char** restrict end, int base);
int abs(int value);
long labs(long value);
long long llabs(long long value);
div_t div(int numerator, int denominator);
ldiv_t ldiv(long numerator, long denominator);
lldiv_t lldiv(long long numerator, long long denominator);
void* malloc(size_t size);
void free(void* memory);
void* calloc(size_t count, size_t size);
void* realloc(void* memory, size_t size);
void* bsearch(const void* key, const void* base, size_t count, size_t width,
    int (*compare)(const void*, const void*));
void qsort(void* base, size_t count, size_t width,
    int (*compare)(const void*, const void*));
int rand(void);
void srand(unsigned int seed);
void abort(void) __attribute__((noreturn));

#endif
