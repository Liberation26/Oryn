#include "ctype.h"

int ispunct(int value)
{
    return isgraph(value) && !isalnum(value);
}
