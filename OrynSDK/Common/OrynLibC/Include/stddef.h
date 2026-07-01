#ifndef ORYN_LIBC_STDDEF_H
#define ORYN_LIBC_STDDEF_H

#define NULL ((void*)0)

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WCHAR_TYPE__ wchar_t;

typedef struct { long long __align; long double __align2; } max_align_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif
