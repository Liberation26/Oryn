#include "ctype.h"

int isprint(int value)
{
    return value >= ' ' && value <= '~';
}
