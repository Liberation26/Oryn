#include "OrynLibCAllocator.h"
#include "OrynLibCProof.h"
#include "ctype.h"
#include "errno.h"
#include "inttypes.h"
#include "stdio.h"
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
    char tokens[16];
    strcpy(text, "ab");
    strcat(text, "cd");
    strncat(text, "efgh", 2U);
    strcpy(tokens, "a,b");
    return strlen(text) == 6U && strcmp(text, "abcdef") == 0 &&
        strncmp(text, "abc", 3U) == 0 && strchr(text, 'd') != 0 &&
        strrchr(text, 'f') != 0 && strstr(text, "cde") != 0 &&
        strspn("abc123", "abc") == 3U && strcspn("abc123", "12") == 3U &&
        strpbrk("abc123", "31") != 0 && strcmp(strtok(tokens, ","), "a") == 0 &&
        strcmp(strtok(0, ","), "b") == 0;
}

static int CheckCTypeAndNumbers(void)
{
    char* end = 0;
    errno = 0;
    return isdigit('7') && isalpha('Q') && isalnum('8') && isspace('\n') &&
        isblank('\t') && isprint('A') && ispunct('!') && isxdigit('f') &&
        tolower('A') == 'a' && toupper('z') == 'Z' && atoi("-42") == -42 &&
        atol("-43") == -43L && atoll("-44") == -44LL &&
        strtol("0x10", &end, 0) == 16L && *end == 0 &&
        strtoll("-20", &end, 10) == -20LL && *end == 0 &&
        strtoul("17", &end, 10) == 17UL && *end == 0 &&
        strtoull("20", &end, 10) == 20ULL && *end == 0 &&
        strtoimax("21", &end, 10) == 21 && strtoumax("22", &end, 10) == 22U;
}

static int CompareInts(const void* left, const void* right)
{
    int a = *(const int*)left;
    int b = *(const int*)right;
    return (a > b) - (a < b);
}

static int CheckUtility(void)
{
    char out[32];
    int values[4] = { 4, 1, 3, 2 };
    int key = 3;
    qsort(values, 4U, sizeof(values[0]), CompareInts);
    snprintf(out, sizeof(out), "%s-%d-%x", "ok", 7, 15U);
    return values[0] == 1 && values[3] == 4 &&
        bsearch(&key, values, 4U, sizeof(values[0]), CompareInts) != 0 &&
        strcmp(out, "ok-7-f") == 0 && rand() >= 0 && !OrynLibCHasAllocator();
}

int OrynLibCRunSelfProof(void)
{
    return CheckMemory() && CheckStrings() && CheckCTypeAndNumbers() && CheckUtility() &&
        abs(-7) == 7 && llabs(-9LL) == 9LL && imaxabs(-10) == 10 &&
        strerror(EINVAL) != 0;
}
