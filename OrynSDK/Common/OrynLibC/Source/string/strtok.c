#include "string.h"

char* strtok(char* restrict text, const char* restrict delimiters)
{
    static char* next = 0;
    char* token;
    if (text == 0) { text = next; }
    if (text == 0) { return 0; }
    text += strspn(text, delimiters);
    if (*text == 0) { next = 0; return 0; }
    token = text;
    text += strcspn(text, delimiters);
    if (*text != 0)
    {
        *text = 0;
        next = text + 1;
    }
    else
    {
        next = 0;
    }
    return token;
}
