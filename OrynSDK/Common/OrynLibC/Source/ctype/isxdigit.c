#include "ctype.h"

int isxdigit(int value)
{
    return isdigit(value) ||
        (value >= 'a' && value <= 'f') ||
        (value >= 'A' && value <= 'F');
}
