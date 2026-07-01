#ifndef ORYN_STRING_H
#define ORYN_STRING_H

#include "string.h"

void* OrynMemset(void* target, int value, size_t size);
void* OrynMemcpy(void* target, const void* source, size_t size);
void* OrynMemmove(void* target, const void* source, size_t size);
int OrynMemcmp(const void* left, const void* right, size_t size);
size_t OrynStrlen(const char* text);
int OrynStrcmp(const char* left, const char* right);
int OrynStrncmp(const char* left, const char* right, size_t size);

#endif
