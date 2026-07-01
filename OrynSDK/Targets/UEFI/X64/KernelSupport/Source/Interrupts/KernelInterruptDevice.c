#include "KernelInterrupts.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

#define ORYN_INTERRUPT_DEVICE_ROUTE_COUNT 64U

typedef struct OrynInterruptDeviceRoute
{
    unsigned int Used;
    unsigned int Vector;
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    const char* Name;
} OrynInterruptDeviceRoute;

static OrynInterruptDeviceRoute gDeviceRoutes[ORYN_INTERRUPT_DEVICE_ROUTE_COUNT];
static unsigned int gDeviceRouteCount;
static unsigned int gDeviceRouteFailures;

static void RememberDeviceRoute(
    unsigned int vector,
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    const char* name)
{
    OrynInterruptDeviceRoute* route;
    if (gDeviceRouteCount >= ORYN_INTERRUPT_DEVICE_ROUTE_COUNT)
    {
        gDeviceRouteFailures += 1U;
        return;
    }
    route = &gDeviceRoutes[gDeviceRouteCount++];
    route->Used = 1U;
    route->Vector = vector;
    route->Bus = bus;
    route->Device = device;
    route->Function = function;
    route->Name = name;
}

int OrynKernelInterruptsRegisterDeviceHandler(
    unsigned int vector,
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    OrynKernelInterruptHandler handler,
    void* context,
    const char* name)
{
    OrynKernelInterruptState* state = (OrynKernelInterruptState*)OrynKernelInterruptsGetState();
    if (bus > 255U || device > 31U || function > 7U)
    {
        return 0;
    }
    if (!OrynKernelInterruptsRegisterHandler(vector, handler, context, name))
    {
        return 0;
    }
    RememberDeviceRoute(vector, bus, device, function, name);
    state->RegisteredDeviceHandlers = gDeviceRouteCount;
    state->DeviceRouteSlots = ORYN_INTERRUPT_DEVICE_ROUTE_COUNT;
    return 1;
}

int OrynKernelInterruptsFindDeviceHandler(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int* vectorOut)
{
    for (unsigned int index = 0U; index < gDeviceRouteCount; ++index)
    {
        OrynInterruptDeviceRoute* route = &gDeviceRoutes[index];
        if (route->Used && route->Bus == bus && route->Device == device &&
            route->Function == function)
        {
            if (vectorOut != 0)
            {
                *vectorOut = route->Vector;
            }
            return 1;
        }
    }
    return 0;
}

void OrynKernelInterruptsPrintDeviceProof(void)
{
    OrynKernelScreenReportOkOrFail(
        OrynKernelInterruptsGetState()->HandlerSlots == ORYN_INTERRUPT_VECTOR_COUNT,
        "Interrupt handlers can be registered by vector.",
        "Interrupt vector handler table unavailable.");
    OrynKernelScreenReportOkOrWarn(gDeviceRouteCount != 0U,
        "Interrupt handlers can be registered by device identity.",
        "No device interrupt handler has been registered yet.");
    KernelIoWriteString("[KERNEL] Interrupt device routes/slots/failures: ");
    KernelIoWriteDec64(gDeviceRouteCount);
    KernelIoWriteString("/");
    KernelIoWriteDec64(ORYN_INTERRUPT_DEVICE_ROUTE_COUNT);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gDeviceRouteFailures);
    KernelIoWriteString("\n");
}
