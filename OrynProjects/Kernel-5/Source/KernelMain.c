#include "OrynKernelSdk.h"

static void Kernel5Main(OrynKernelSdkContext* kernel)
{
    OrynKernelSdkReportOk(kernel, "Kernel-5 is built as an Oryn SDK kernel application.");
    OrynKernelSdkRunBootProof(kernel);
}

ORYN_KERNEL_APPLICATION("Kernel-5", Kernel5Main)
