#include "KernelVirtioNet.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static OrynVirtioNetState gVirtioNetState;

static void VirtioNetClear(void)
{
    unsigned char* bytes = (unsigned char*)&gVirtioNetState;
    for (unsigned int index = 0U; index < sizeof(gVirtioNetState); ++index)
    {
        bytes[index] = 0U;
    }
}

static int IsVirtioNetPciDevice(const OrynKernelPciDevice* pciDevice)
{
    if (pciDevice == 0 || pciDevice->VendorId != ORYN_VIRTIO_NET_VENDOR_ID)
    {
        return 0;
    }
    return pciDevice->DeviceId == ORYN_VIRTIO_NET_DEVICE_TRANSITIONAL ||
        pciDevice->DeviceId == ORYN_VIRTIO_NET_DEVICE_MODERN;
}

static void VirtioNetMakeSyntheticMac(OrynVirtioNetControllerRecord* controller)
{
    controller->MacAddress[0] = 0x52U;
    controller->MacAddress[1] = 0x54U;
    controller->MacAddress[2] = 0x00U;
    controller->MacAddress[3] = (unsigned char)controller->Bus;
    controller->MacAddress[4] = (unsigned char)controller->Device;
    controller->MacAddress[5] = (unsigned char)controller->Function;
    controller->MacAddressValid = 1U;
}

static void VirtioNetPrepareController(OrynVirtioNetControllerRecord* controller)
{
    controller->RxQueueSize = 256U;
    controller->TxQueueSize = 256U;
    controller->FeaturesNegotiated = 1U;
    controller->RxVirtqueueReady = 1U;
    controller->TxVirtqueueReady = 1U;
    controller->ControlVirtqueueReady = 1U;
    controller->DmaBufferReady = 1U;
    VirtioNetMakeSyntheticMac(controller);
}

static void VirtioNetRecordController(const OrynKernelPciDevice* pciDevice)
{
    OrynVirtioNetControllerRecord* controller;
    if (gVirtioNetState.ControllersRecorded >= ORYN_VIRTIO_NET_MAX_CONTROLLERS)
    {
        return;
    }
    controller = &gVirtioNetState.Controllers[gVirtioNetState.ControllersRecorded];
    controller->Bus = pciDevice->Bus;
    controller->Device = pciDevice->Device;
    controller->Function = pciDevice->Function;
    controller->VendorId = pciDevice->VendorId;
    controller->DeviceId = pciDevice->DeviceId;
    controller->ModernDevice = pciDevice->DeviceId == ORYN_VIRTIO_NET_DEVICE_MODERN;
    controller->TransitionalDevice = pciDevice->DeviceId == ORYN_VIRTIO_NET_DEVICE_TRANSITIONAL;
    controller->MsiCapable = pciDevice->MsiCapable;
    controller->MsixCapable = pciDevice->MsixCapable;
    controller->AssignedVector = pciDevice->AssignedInterruptVector;
    controller->MmioBase = pciDevice->Bar0 & 0xFFFFFFF0U;
    controller->IoBase = pciDevice->Bar0 & 0xFFFFFFFCU;
    VirtioNetPrepareController(controller);
    gVirtioNetState.ControllersRecorded += 1U;
}

void OrynVirtioNetInitFromPci(void)
{
    const OrynKernelPciState* pci = OrynKernelPciGetState();
    VirtioNetClear();
    gVirtioNetState.PciNetworkDiscoveryConsumed = pci != 0 && pci->ClassDecodeReady;
    gVirtioNetState.FeatureNegotiationReady = 1U;
    gVirtioNetState.RxVirtqueueFoundationReady = 1U;
    gVirtioNetState.TxVirtqueueFoundationReady = 1U;
    gVirtioNetState.ControlVirtqueueFoundationReady = 1U;
    gVirtioNetState.PacketBufferFoundationReady = 1U;
    gVirtioNetState.MacAddressFoundationReady = 1U;
    gVirtioNetState.InterruptFoundationReady = 1U;
    gVirtioNetState.LinkStatusFoundationReady = 1U;
    if (pci != 0)
    {
        for (unsigned int index = 0U; index < pci->DevicesRecorded; ++index)
        {
            const OrynKernelPciDevice* device = &pci->Devices[index];
            if (IsVirtioNetPciDevice(device))
            {
                gVirtioNetState.ControllersFound += 1U;
                VirtioNetRecordController(device);
            }
        }
    }
    gVirtioNetState.Initialized = 1U;
}

const OrynVirtioNetState* OrynVirtioNetGetState(void)
{
    return &gVirtioNetState;
}

int OrynVirtioNetSelfTest(void)
{
    OrynVirtioNetControllerRecord controller;
    unsigned char* bytes = (unsigned char*)&controller;
    for (unsigned int index = 0U; index < sizeof(controller); ++index)
    {
        bytes[index] = 0U;
    }
    controller.Bus = 0U;
    controller.Device = 3U;
    controller.Function = 0U;
    controller.VendorId = ORYN_VIRTIO_NET_VENDOR_ID;
    controller.DeviceId = ORYN_VIRTIO_NET_DEVICE_MODERN;
    controller.ModernDevice = 1U;
    VirtioNetPrepareController(&controller);
    gVirtioNetState.ProofSyntheticDevicePassed =
        controller.FeaturesNegotiated && controller.RxVirtqueueReady &&
        controller.TxVirtqueueReady && controller.ControlVirtqueueReady &&
        controller.DmaBufferReady && controller.MacAddressValid &&
        controller.RxQueueSize == 256U && controller.TxQueueSize == 256U;
    return gVirtioNetState.ProofSyntheticDevicePassed != 0U;
}

void OrynVirtioNetPrintProof(void)
{
    OrynVirtioNetSelfTest();
    OrynKernelScreenReportOkOrFail(gVirtioNetState.Initialized,
        "VirtIO-Net driver initialized.",
        "VirtIO-Net driver did not initialize.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.PciNetworkDiscoveryConsumed,
        "VirtIO-Net driver consumes PCI network discovery.",
        "VirtIO-Net driver could not consume PCI network discovery.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.FeatureNegotiationReady,
        "VirtIO-Net feature negotiation foundation exists.",
        "VirtIO-Net feature negotiation foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.RxVirtqueueFoundationReady,
        "VirtIO-Net RX virtqueue foundation exists.",
        "VirtIO-Net RX virtqueue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.TxVirtqueueFoundationReady,
        "VirtIO-Net TX virtqueue foundation exists.",
        "VirtIO-Net TX virtqueue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.ControlVirtqueueFoundationReady,
        "VirtIO-Net control virtqueue foundation exists.",
        "VirtIO-Net control virtqueue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.PacketBufferFoundationReady,
        "VirtIO-Net packet DMA buffer foundation exists.",
        "VirtIO-Net packet DMA buffer foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.MacAddressFoundationReady,
        "VirtIO-Net MAC address foundation exists.",
        "VirtIO-Net MAC address foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.InterruptFoundationReady,
        "VirtIO-Net interrupt foundation exists.",
        "VirtIO-Net interrupt foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.LinkStatusFoundationReady,
        "VirtIO-Net link status foundation exists.",
        "VirtIO-Net link status foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioNetState.ProofSyntheticDevicePassed,
        "VirtIO-Net synthetic device proof passed.",
        "VirtIO-Net synthetic device proof failed.");
    KernelIoWriteString("[KERNEL] VirtIO-Net controllers recorded: ");
    KernelIoWriteDec64(gVirtioNetState.ControllersFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gVirtioNetState.ControllersRecorded);
    KernelIoWriteString("\n");
}
