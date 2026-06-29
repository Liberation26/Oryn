#ifndef ORYN_STRING_H
#define ORYN_STRING_H

#include "OrynStdDef.h"

void* OrynMemset(void* target, int value, size_t size);
void* OrynMemcpy(void* target, const void* source, size_t size);
size_t OrynStrlen(const char* text);

#endif
