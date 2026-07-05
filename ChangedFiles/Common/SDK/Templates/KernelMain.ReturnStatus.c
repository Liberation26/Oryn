#include "OrynKernelSdk.h"

static OrynStatus Kernel5Main(OrynKernelSdkContext* kernel)
{
    OrynStatus status;

    if (kernel == 0)
    {
        return OrynStatusInvalidArgument("Kernel SDK context was null.");
    }

    status = OrynKernelSdkWriteLine(kernel, "Hello World");
    if (ORYN_STATUS_FAILED(status))
    {
        return status;
    }

    return OrynStatusOk("Kernel-5 completed.");
}

ORYN_KERNEL_APPLICATION("Kernel-5", Kernel5Main)
