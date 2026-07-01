#include "OrynString.h"

void* OrynMemset(void* target, int value, size_t size)
{
    return memset(target, value, size);
}

void* OrynMemcpy(void* target, const void* source, size_t size)
{
    return memcpy(target, source, size);
}

void* OrynMemmove(void* target, const void* source, size_t size)
{
    return memmove(target, source, size);
}

int OrynMemcmp(const void* left, const void* right, size_t size)
{
    return memcmp(left, right, size);
}

size_t OrynStrlen(const char* text)
{
    return strlen(text);
}

int OrynStrcmp(const char* left, const char* right)
{
    return strcmp(left, right);
}

int OrynStrncmp(const char* left, const char* right, size_t size)
{
    return strncmp(left, right, size);
}
