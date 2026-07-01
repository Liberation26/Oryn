#include "ctype.h"

int isalnum(int value)
{
    return isalpha(value) || isdigit(value);
}
