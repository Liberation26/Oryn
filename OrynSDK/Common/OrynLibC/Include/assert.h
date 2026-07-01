#ifndef ORYN_LIBC_ASSERT_H
#define ORYN_LIBC_ASSERT_H

#include "stdlib.h"

#ifdef NDEBUG
#define assert(expression) ((void)0)
#else
void OrynAssertFail(const char* expression, const char* file, int line) __attribute__((noreturn));
#define assert(expression) ((expression) ? (void)0 : OrynAssertFail(#expression, __FILE__, __LINE__))
#endif

#endif
