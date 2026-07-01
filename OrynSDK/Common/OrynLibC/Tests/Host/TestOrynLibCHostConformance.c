
#include "ctype.h"
#include "errno.h"
#include "inttypes.h"
#include "OrynLibCAllocator.h"
#include "OrynString.h"
#include "stdarg.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static int failures = 0;

#define CHECK(condition) do { if (!(condition)) { failures++; } } while (0)

static unsigned char host_heap[4096];
static size_t host_heap_used;

static void* HostAlloc(size_t size)
{
    void* result;
    if (size == 0)
    {
        size = 1;
    }
    if (host_heap_used + size > sizeof(host_heap))
    {
        return 0;
    }
    result = host_heap + host_heap_used;
    host_heap_used += (size + 7U) & ~7U;
    return result;
}

static void HostFree(void* memory)
{
    (void)memory;
}

static void* HostRealloc(void* memory, size_t size)
{
    void* result = HostAlloc(size);
    if (memory != 0 && result != 0)
    {
        memcpy(result, memory, size < 16U ? size : 16U);
    }
    return result;
}

static int CompareInt(const void* left, const void* right)
{
    int a = *(const int*)left;
    int b = *(const int*)right;
    return (a > b) - (a < b);
}

static int CallVsnprintf(char* output, size_t output_size, const char* format, ...)
{
    int written;
    va_list args;
    va_start(args, format);
    written = vsnprintf(output, output_size, format, args);
    va_end(args);
    return written;
}

int main(void)
{
    char buffer[128];
    char source[] = "abc";
    char overlap[32] = "abcdef";
    char* end = 0;
    int values[5] = {5, 1, 4, 2, 3};
    int key = 4;
    int* found;
    div_t div_result;
    ldiv_t ldiv_result;
    lldiv_t lldiv_result;
    imaxdiv_t imaxdiv_result;
    char* allocated;
    char* zeroed;
    char* resized;
    int rand_a;
    int rand_b;

    CHECK(isdigit('4') && !isdigit('a'));
    CHECK(isalpha('a') && isalpha('Z') && !isalpha('1'));
    CHECK(isalnum('9') && isalnum('b') && !isalnum('#'));
    CHECK(isblank(' ') && isblank('\t') && !isblank('\n'));
    CHECK(iscntrl('\n') && !iscntrl('A'));
    CHECK(isgraph('A') && !isgraph(' '));
    CHECK(isprint(' ') && !isprint('\x7f'));
    CHECK(ispunct('!') && !ispunct('A'));
    CHECK(isspace('\n') && isspace(' ') && !isspace('x'));
    CHECK(isxdigit('f') && isxdigit('F') && !isxdigit('g'));
    CHECK(tolower('A') == 'a' && tolower('a') == 'a');
    CHECK(toupper('a') == 'A' && toupper('A') == 'A');

    errno = 0;
    CHECK(errno == 0);
    errno = EINVAL;
    CHECK(errno == EINVAL);

    CHECK(memset(buffer, 'x', 3) == buffer && buffer[0] == 'x' && buffer[2] == 'x');
    CHECK(memcpy(buffer, source, 4) == buffer && strcmp(buffer, "abc") == 0);
    CHECK(memmove(overlap + 2, overlap, 4) == overlap + 2 && memcmp(overlap, "ababcd", 6) == 0);
    CHECK(memcmp("abc", "abd", 3) < 0);
    CHECK(*(char*)memchr("abc", 'b', 3) == 'b' && memchr("abc", 'z', 3) == 0);
    CHECK(strlen("abc") == 3 && strnlen("abcdef", 3) == 3);
    CHECK(strcmp("abc", "abc") == 0 && strcmp("abc", "abd") < 0);
    CHECK(strncmp("abc", "abd", 2) == 0 && strncmp("abc", "abd", 3) < 0);
    CHECK(strcpy(buffer, "hi") == buffer && strcmp(buffer, "hi") == 0);
    memset(buffer, 'z', sizeof(buffer));
    CHECK(strncpy(buffer, "hi", 4) == buffer && buffer[0] == 'h' && buffer[2] == '\0');
    strcpy(buffer, "a");
    CHECK(strcat(buffer, "b") == buffer && strcmp(buffer, "ab") == 0);
    strcpy(buffer, "a");
    CHECK(strncat(buffer, "bcd", 2) == buffer && strcmp(buffer, "abc") == 0);
    CHECK(*strchr("abc", 'b') == 'b' && strchr("abc", 'z') == 0);
    CHECK(*strrchr("abca", 'a') == 'a' && strrchr("abc", 'z') == 0);
    CHECK(strspn("abc123", "abc") == 3);
    CHECK(strcspn("abc123", "123") == 3);
    CHECK(*strpbrk("abc", "xzcy") == 'c');
    CHECK(strstr("hello world", "world") != 0 && strstr("hello", "zz") == 0);

    {
        char tokens[] = "a,b;c";
        CHECK(strcmp(strtok(tokens, ",;"), "a") == 0);
        CHECK(strcmp(strtok(0, ",;"), "b") == 0);
        CHECK(strcmp(strtok(0, ",;"), "c") == 0);
        CHECK(strtok(0, ",;") == 0);
    }

    CHECK(strerror(EINVAL) != 0);
    CHECK(OrynMemset(buffer, 0, 4) == buffer && buffer[3] == 0);
    CHECK(OrynMemcpy(buffer, "xy", 3) == buffer && OrynStrcmp(buffer, "xy") == 0);
    strcpy(overlap, "abcdef");
    CHECK(OrynMemmove(overlap + 1, overlap, 3) == overlap + 1);
    CHECK(OrynMemcmp("a", "b", 1) < 0);
    CHECK(OrynStrlen("four") == 4);
    CHECK(OrynStrncmp("ab", "ac", 1) == 0);

    CHECK(atoi("-42") == -42 && atol("123") == 123L && atoll("-123") == -123LL);
    CHECK(strtol("7b", &end, 10) == 7 && *end == 'b');
    CHECK(strtoll("-20", &end, 10) == -20LL);
    CHECK(strtoul("ff", &end, 16) == 255UL);
    CHECK(strtoull("77", &end, 8) == 63ULL);
    CHECK(abs(-5) == 5 && labs(-6L) == 6L && llabs(-7LL) == 7LL);

    div_result = div(7, 3);
    CHECK(div_result.quot == 2 && div_result.rem == 1);
    ldiv_result = ldiv(7L, 3L);
    CHECK(ldiv_result.quot == 2 && ldiv_result.rem == 1);
    lldiv_result = lldiv(7LL, 3LL);
    CHECK(lldiv_result.quot == 2 && lldiv_result.rem == 1);
    CHECK(strtoimax("-9", &end, 10) == -9);
    CHECK(strtoumax("10", &end, 10) == 10);
    imaxdiv_result = imaxdiv(9, 4);
    CHECK(imaxdiv_result.quot == 2 && imaxdiv_result.rem == 1);
    CHECK(imaxabs(-12) == 12);

    OrynLibCSetAllocator(HostAlloc, HostFree, HostRealloc);
    CHECK(OrynLibCHasAllocator());
    allocated = (char*)malloc(8);
    CHECK(allocated != 0);
    strcpy(allocated, "abc");
    zeroed = (char*)calloc(4, 2);
    CHECK(zeroed != 0 && zeroed[0] == 0 && zeroed[7] == 0);
    resized = (char*)realloc(allocated, 16);
    CHECK(resized != 0);
    free(resized);

    qsort(values, 5, sizeof(int), CompareInt);
    CHECK(values[0] == 1 && values[4] == 5);
    found = (int*)bsearch(&key, values, 5, sizeof(int), CompareInt);
    CHECK(found != 0 && *found == 4);
    srand(1);
    rand_a = rand();
    srand(1);
    rand_b = rand();
    CHECK(rand_a == rand_b && rand_a >= 0 && rand_a <= RAND_MAX);

    CHECK(snprintf(buffer, sizeof(buffer), "A%s%d%u%x%c", "B", -3, 7U, 15U, '!') > 0);
    CHECK(strcmp(buffer, "AB-37f!") == 0);
    CHECK(CallVsnprintf(buffer, sizeof(buffer), "V%s%d", "S", 8) > 0);
    CHECK(strcmp(buffer, "VS8") == 0);

    return failures ? 1 : 0;
}
