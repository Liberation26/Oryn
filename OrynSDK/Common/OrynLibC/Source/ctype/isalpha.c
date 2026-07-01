#include "ctype.h"

int isalpha(int value)
{
    return islower(value) || isupper(value);
}
