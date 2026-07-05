#include "OrynKernelSdk.h"
#include "OrynStatus.h"

static OrynStatus Kernel5MainChecked(OrynKernelSdkContext* kernel)
{
    if (kernel == 0)
    {
        return OrynStatusInvalidArgument("Kernel SDK context was null.");
    }

    OrynKernelSdkWriteLine(kernel, "Hello World");

    return OrynStatusOk("Kernel-5 completed.");
}

static void Kernel5Main(OrynKernelSdkContext* kernel)
{
    OrynStatus status = Kernel5MainChecked(kernel);

    if (ORYN_STATUS_FAILED(status))
    {
        OrynKernelSdkWriteLine(kernel, status.Message);
    }
}

ORYN_KERNEL_APPLICATION("Kernel-5", Kernel5Main)
