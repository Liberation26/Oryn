#include "ctype.h"

int isdigit(int value) { return value >= '0' && value <= '9'; }
int islower(int value) { return value >= 'a' && value <= 'z'; }
int isupper(int value) { return value >= 'A' && value <= 'Z'; }
int isalpha(int value) { return islower(value) || isupper(value); }
int isalnum(int value) { return isalpha(value) || isdigit(value); }
int isblank(int value) { return value == ' ' || value == '\t'; }
int iscntrl(int value) { return (value >= 0 && value < 32) || value == 127; }
int isgraph(int value) { return value > ' ' && value <= '~'; }
int isprint(int value) { return value >= ' ' && value <= '~'; }
int ispunct(int value) { return isgraph(value) && !isalnum(value); }
int isspace(int value)
{
    return value == ' ' || value == '\f' || value == '\n' ||
        value == '\r' || value == '\t' || value == '\v';
}
int isxdigit(int value)
{
    return isdigit(value) || (value >= 'a' && value <= 'f') ||
        (value >= 'A' && value <= 'F');
}
int tolower(int value) { return isupper(value) ? value + ('a' - 'A') : value; }
int toupper(int value) { return islower(value) ? value - ('a' - 'A') : value; }
