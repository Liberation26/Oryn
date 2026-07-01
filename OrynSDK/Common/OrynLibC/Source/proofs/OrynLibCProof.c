#include "OrynLibCProof.h"
#include "ctype.h"
#include "errno.h"
#include "stdlib.h"
#include "string.h"

static int CheckMemory(void)
{
    char buffer[16];
    char moved[16];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, "abc", 4U);
    memmove(moved, buffer, 4U);
    return strcmp(buffer, "abc") == 0 && strcmp(moved, "abc") == 0 &&
        memcmp(buffer, moved, 4U) == 0 && memchr(buffer, 'b', 4U) != 0;
}

static int CheckStrings(void)
{
    char text[32];
    strcpy(text, "ab");
    strcat(text, "cd");
    strncat(text, "efgh", 2U);
    return strlen(text) == 6U && strcmp(text, "abcdef") == 0 &&
        strncmp(text, "abc", 3U) == 0 && strchr(text, 'd') != 0 &&
        strrchr(text, 'f') != 0 && strstr(text, "cde") != 0;
}

static int CheckCTypeAndNumbers(void)
{
    char* end = 0;
    errno = 0;
    return isdigit('7') && isalpha('Q') && isalnum('8') && isspace('\n') &&
        isxdigit('f') && tolower('A') == 'a' && toupper('z') == 'Z' &&
        atoi("-42") == -42 && strtol("0x10", &end, 0) == 16L && *end == 0 &&
        strtoul("17", &end, 10) == 17UL && *end == 0 &&
        strtoull("20", &end, 10) == 20ULL && *end == 0;
}

int OrynLibCRunSelfProof(void)
{
    return CheckMemory() && CheckStrings() && CheckCTypeAndNumbers() &&
        abs(-7) == 7 && llabs(-9LL) == 9LL && strerror(EINVAL) != 0;
}
