#include "ctype.h"

int tolower(int value)
{
    return isupper(value) ? value + ('a' - 'A') : value;
}
