#include "string.h"

char* strcat(char* restrict target, const char* restrict source)
{
    char* end = target + strlen(target);
    strcpy(end, source);
    return target;
}
