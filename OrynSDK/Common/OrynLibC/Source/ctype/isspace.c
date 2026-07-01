#include "ctype.h"

int isspace(int value)
{
    return value == ' ' || value == '\t' || value == '\n' ||
        value == '\r' || value == '\v' || value == '\f';
}
