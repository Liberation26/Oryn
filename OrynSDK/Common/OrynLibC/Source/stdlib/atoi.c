#include "stdlib.h"

int atoi(const char* text)
{
    return (int)strtol(text, 0, 10);
}
