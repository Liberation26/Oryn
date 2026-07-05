#ifndef ORYN_KERNEL_4_H
#define ORYN_KERNEL_4_H

#include "OrynBootInfo.h"
#include <stdbool.h>

bool KernelMain(const OrynBootInfo* bootInfo);
void KernelStart(const OrynBootInfo* bootInfo);

#endif
