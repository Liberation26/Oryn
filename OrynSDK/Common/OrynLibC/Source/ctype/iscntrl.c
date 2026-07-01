#include "ctype.h"

int iscntrl(int value)
{
    return (value >= 0 && value < 32) || value == 127;
}
