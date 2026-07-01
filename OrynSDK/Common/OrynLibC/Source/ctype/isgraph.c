#include "ctype.h"

int isgraph(int value)
{
    return value > ' ' && value <= '~';
}
