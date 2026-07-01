#include "assert.h"

#ifndef NDEBUG
void OrynAssertFail(const char* expression, const char* file, int line)
{
    (void)expression;
    (void)file;
    (void)line;
    abort();
}
#endif
