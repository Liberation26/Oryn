#include "OrynKernelSdk.h"

static void Kernel5Main(OrynKernelSdkContext* kernel)
{
    OrynKernelSdkWriteLine(kernel, "Hello World");
}

ORYN_KERNEL_APPLICATION("Kernel-5", Kernel5Main)
