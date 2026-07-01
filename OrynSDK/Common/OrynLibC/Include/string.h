#ifndef ORYN_LIBC_STRING_H
#define ORYN_LIBC_STRING_H

#include "stddef.h"

void* memcpy(void* restrict target, const void* restrict source, size_t size);
void* memmove(void* target, const void* source, size_t size);
void* memset(void* target, int value, size_t size);
int memcmp(const void* left, const void* right, size_t size);
void* memchr(const void* memory, int value, size_t size);

size_t strlen(const char* text);
size_t strnlen(const char* text, size_t max_size);
int strcmp(const char* left, const char* right);
int strncmp(const char* left, const char* right, size_t size);
char* strcpy(char* restrict target, const char* restrict source);
char* strncpy(char* restrict target, const char* restrict source, size_t size);
char* strcat(char* restrict target, const char* restrict source);
char* strncat(char* restrict target, const char* restrict source, size_t size);
char* strchr(const char* text, int value);
char* strrchr(const char* text, int value);
char* strstr(const char* haystack, const char* needle);
char* strerror(int error_number);

#endif
