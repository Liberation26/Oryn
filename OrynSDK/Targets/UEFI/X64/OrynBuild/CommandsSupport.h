#ifndef ORYN_COMMANDS_SUPPORT_H
#define ORYN_COMMANDS_SUPPORT_H

#include "OrynBuild.h"
#include <stddef.h>

void PrintToolStatus(const char* label, const char* program, int* ok);
void PrintWindowsQemuStatus(int* ok);
void PrintFileIfPresent(const char* title, const char* path);
int ReadFileText(const char* path, char* output, size_t output_size);
int TextContains(const char* text, const char* needle);
const char* PassFail(int value);
const char* ResolveQemuDisplayMode(const OrynProject* project);
int IsSafeQemuDisplayMode(const char* display);
int RunQemuAndGetExitCode(const char* command, int* exit_code);

#endif
