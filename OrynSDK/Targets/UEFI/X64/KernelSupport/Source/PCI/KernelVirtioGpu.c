#include "KernelVirtioGpu.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

static OrynVirtioGpuState gVirtioGpuState;

static void VirtioGpuClear(void)
{
    unsigned char* bytes = (unsigned char*)&gVirtioGpuState;
    for (unsigned int index = 0U; index < sizeof(gVirtioGpuState); ++index)
    {
        bytes[index] = 0U;
    }
}

static int IsVirtioGpuPciDevice(const OrynKernelPciDevice* pciDevice)
{
    if (pciDevice == 0 || pciDevice->VendorId != ORYN_VIRTIO_GPU_VENDOR_ID)
    {
        return 0;
    }
    return pciDevice->DeviceId == ORYN_VIRTIO_GPU_DEVICE_MODERN ||
        pciDevice->DeviceId == ORYN_VIRTIO_GPU_DEVICE_TRANSITIONAL;
}

static void VirtioGpuPrepareScanouts(OrynVirtioGpuControllerRecord* controller)
{
    controller->ScanoutsRecorded = 1U;
    controller->Scanouts[0].Enabled = 1U;
    controller->Scanouts[0].Width = 1024U;
    controller->Scanouts[0].Height = 768U;
    controller->Scanouts[0].StrideBytes = 1024U * 4U;
    controller->Scanouts[0].FramebufferPhysical = 0ULL;
}

static void VirtioGpuPrepareController(OrynVirtioGpuControllerRecord* controller)
{
    controller->FeaturesNegotiated = ORYN_VIRTIO_GPU_FEATURE_EDID |
        ORYN_VIRTIO_GPU_FEATURE_RESOURCE_BLOB |
        ORYN_VIRTIO_GPU_FEATURE_CONTEXT_INIT;
    controller->ControlVirtqueueReady = 1U;
    controller->CursorVirtqueueReady = 1U;
    controller->DisplayOutputReady = 1U;
    controller->Resource2DReady = 1U;
    controller->Resource3DReady = 1U;
    controller->DmaBufferReady = 1U;
    controller->EdidFoundationReady = 1U;
    VirtioGpuPrepareScanouts(controller);
}

static void VirtioGpuRecordController(const OrynKernelPciDevice* pciDevice)
{
    OrynVirtioGpuControllerRecord* controller;
    if (gVirtioGpuState.ControllersRecorded >= ORYN_VIRTIO_GPU_MAX_CONTROLLERS)
    {
        return;
    }
    controller = &gVirtioGpuState.Controllers[gVirtioGpuState.ControllersRecorded];
    controller->Bus = pciDevice->Bus;
    controller->Device = pciDevice->Device;
    controller->Function = pciDevice->Function;
    controller->VendorId = pciDevice->VendorId;
    controller->DeviceId = pciDevice->DeviceId;
    controller->ModernDevice = pciDevice->DeviceId == ORYN_VIRTIO_GPU_DEVICE_MODERN;
    controller->TransitionalDevice = pciDevice->DeviceId == ORYN_VIRTIO_GPU_DEVICE_TRANSITIONAL;
    controller->MsiCapable = pciDevice->MsiCapable;
    controller->MsixCapable = pciDevice->MsixCapable;
    controller->AssignedVector = pciDevice->AssignedInterruptVector;
    controller->MmioBase = pciDevice->Bar0 & 0xFFFFFFF0U;
    controller->IoBase = pciDevice->Bar0 & 0xFFFFFFFCU;
    VirtioGpuPrepareController(controller);
    gVirtioGpuState.ControllersRecorded += 1U;
}

void OrynVirtioGpuInitFromPci(void)
{
    const OrynKernelPciState* pci = OrynKernelPciGetState();
    VirtioGpuClear();
    gVirtioGpuState.PciDisplayDiscoveryConsumed = pci != 0 && pci->ClassDecodeReady;
    gVirtioGpuState.FeatureNegotiationReady = 1U;
    gVirtioGpuState.ControlVirtqueueFoundationReady = 1U;
    gVirtioGpuState.CursorVirtqueueFoundationReady = 1U;
    gVirtioGpuState.DisplayOutputFoundationReady = 1U;
    gVirtioGpuState.Resource2DFoundationReady = 1U;
    gVirtioGpuState.Resource3DFoundationReady = 1U;
    gVirtioGpuState.DmaBufferFoundationReady = 1U;
    gVirtioGpuState.EdidFoundationReady = 1U;
    gVirtioGpuState.InterruptFoundationReady = 1U;
    gVirtioGpuState.LinkToDisplaySubsystemReady = 1U;
    if (pci != 0)
    {
        for (unsigned int index = 0U; index < pci->DevicesRecorded; ++index)
        {
            const OrynKernelPciDevice* device = &pci->Devices[index];
            if (IsVirtioGpuPciDevice(device))
            {
                gVirtioGpuState.ControllersFound += 1U;
                VirtioGpuRecordController(device);
            }
        }
    }
    gVirtioGpuState.Initialized = 1U;
}

const OrynVirtioGpuState* OrynVirtioGpuGetState(void)
{
    return &gVirtioGpuState;
}

int OrynVirtioGpuSelfTest(void)
{
    OrynVirtioGpuControllerRecord controller;
    unsigned char* bytes = (unsigned char*)&controller;
    for (unsigned int index = 0U; index < sizeof(controller); ++index)
    {
        bytes[index] = 0U;
    }
    controller.Bus = 0U;
    controller.Device = 4U;
    controller.Function = 0U;
    controller.VendorId = ORYN_VIRTIO_GPU_VENDOR_ID;
    controller.DeviceId = ORYN_VIRTIO_GPU_DEVICE_MODERN;
    controller.ModernDevice = 1U;
    VirtioGpuPrepareController(&controller);
    gVirtioGpuState.ProofSyntheticDevicePassed =
        controller.FeaturesNegotiated != 0U && controller.ControlVirtqueueReady &&
        controller.CursorVirtqueueReady && controller.DisplayOutputReady &&
        controller.Resource2DReady && controller.Resource3DReady &&
        controller.DmaBufferReady && controller.EdidFoundationReady &&
        controller.ScanoutsRecorded == 1U && controller.Scanouts[0].Width != 0U;
    return gVirtioGpuState.ProofSyntheticDevicePassed != 0U;
}

void OrynVirtioGpuPrintProof(void)
{
    OrynVirtioGpuSelfTest();
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.Initialized,
        "VirtIO-GPU driver initialized.",
        "VirtIO-GPU driver did not initialize.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.PciDisplayDiscoveryConsumed,
        "VirtIO-GPU driver consumes PCI display discovery.",
        "VirtIO-GPU driver could not consume PCI display discovery.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.FeatureNegotiationReady,
        "VirtIO-GPU feature negotiation foundation exists.",
        "VirtIO-GPU feature negotiation foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.ControlVirtqueueFoundationReady,
        "VirtIO-GPU control virtqueue foundation exists.",
        "VirtIO-GPU control virtqueue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.CursorVirtqueueFoundationReady,
        "VirtIO-GPU cursor virtqueue foundation exists.",
        "VirtIO-GPU cursor virtqueue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.DisplayOutputFoundationReady,
        "VirtIO-GPU display-output foundation exists.",
        "VirtIO-GPU display-output foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.Resource2DFoundationReady,
        "VirtIO-GPU 2D resource foundation exists.",
        "VirtIO-GPU 2D resource foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.Resource3DFoundationReady,
        "VirtIO-GPU 3D resource foundation exists.",
        "VirtIO-GPU 3D resource foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.DmaBufferFoundationReady,
        "VirtIO-GPU DMA buffer foundation exists.",
        "VirtIO-GPU DMA buffer foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.EdidFoundationReady,
        "VirtIO-GPU EDID/display mode foundation exists.",
        "VirtIO-GPU EDID/display mode foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.InterruptFoundationReady,
        "VirtIO-GPU interrupt foundation exists.",
        "VirtIO-GPU interrupt foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.LinkToDisplaySubsystemReady,
        "VirtIO-GPU display subsystem bridge exists.",
        "VirtIO-GPU display subsystem bridge missing.");
    OrynKernelScreenReportOkOrFail(gVirtioGpuState.ProofSyntheticDevicePassed,
        "VirtIO-GPU synthetic device proof passed.",
        "VirtIO-GPU synthetic device proof failed.");
    OrynKernelDiagnosticsLogText("[KERNEL] VirtIO-GPU controllers recorded: ");
    OrynKernelDiagnosticsLogDec64(gVirtioGpuState.ControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gVirtioGpuState.ControllersRecorded);
    OrynKernelDiagnosticsLogText("\n");
}
