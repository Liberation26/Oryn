#include "ctype.h"

int toupper(int value)
{
    return islower(value) ? value - ('a' - 'A') : value;
}
