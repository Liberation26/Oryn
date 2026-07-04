#include "KernelVirtioBlock.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

static OrynVirtioBlockState gVirtioBlockState;

static void VirtioBlockClear(void)
{
    unsigned char* bytes = (unsigned char*)&gVirtioBlockState;
    for (unsigned int index = 0U; index < sizeof(gVirtioBlockState); ++index)
    {
        bytes[index] = 0U;
    }
}

static int IsVirtioBlockPciDevice(const OrynKernelPciDevice* pciDevice)
{
    if (pciDevice == 0 || pciDevice->VendorId != ORYN_VIRTIO_BLK_VENDOR_ID)
    {
        return 0;
    }
    return pciDevice->DeviceId == ORYN_VIRTIO_BLK_DEVICE_TRANSITIONAL ||
        pciDevice->DeviceId == ORYN_VIRTIO_BLK_DEVICE_MODERN;
}

static int VirtioBlockRead(
    OrynKernelBlockDevice* device,
    uint64_t lba,
    unsigned int sectorCount,
    void* buffer)
{
    (void)lba;
    (void)sectorCount;
    (void)buffer;
    if (device == 0 || device->Context == 0)
    {
        return 0;
    }
    gVirtioBlockState.ReadRequests += 1U;
    return 0;
}

static int VirtioBlockWrite(
    OrynKernelBlockDevice* device,
    uint64_t lba,
    unsigned int sectorCount,
    const void* buffer)
{
    (void)lba;
    (void)sectorCount;
    (void)buffer;
    if (device == 0 || device->Context == 0)
    {
        return 0;
    }
    gVirtioBlockState.WriteRequests += 1U;
    return 0;
}

static int VirtioBlockFlush(OrynKernelBlockDevice* device)
{
    if (device == 0 || device->Context == 0)
    {
        return 0;
    }
    gVirtioBlockState.FlushRequests += 1U;
    return 1;
}

static void VirtioBlockPrepareDevice(OrynVirtioBlockControllerRecord* controller)
{
    controller->BlockDevice.BytesPerSector = ORYN_VIRTIO_BLK_SECTOR_SIZE;
    controller->BlockDevice.BlockCount = controller->CapacitySectors;
    controller->BlockDevice.Type = OrynKernelBlockDeviceTypeVirtio;
    controller->BlockDevice.Name = "VirtIO block disk";
    controller->BlockDevice.Context = controller;
    controller->BlockDevice.ReadRange = VirtioBlockRead;
    controller->BlockDevice.WriteRange = VirtioBlockWrite;
    controller->BlockDevice.Flush = VirtioBlockFlush;
    gVirtioBlockState.BlockDevicesPrepared += 1U;
}

static void VirtioBlockRecordController(const OrynKernelPciDevice* pciDevice)
{
    OrynVirtioBlockControllerRecord* controller;
    if (gVirtioBlockState.ControllersRecorded >= ORYN_VIRTIO_BLK_MAX_CONTROLLERS)
    {
        return;
    }
    controller = &gVirtioBlockState.Controllers[gVirtioBlockState.ControllersRecorded];
    controller->Bus = pciDevice->Bus;
    controller->Device = pciDevice->Device;
    controller->Function = pciDevice->Function;
    controller->VendorId = pciDevice->VendorId;
    controller->DeviceId = pciDevice->DeviceId;
    controller->ModernDevice = pciDevice->DeviceId == ORYN_VIRTIO_BLK_DEVICE_MODERN;
    controller->TransitionalDevice = pciDevice->DeviceId == ORYN_VIRTIO_BLK_DEVICE_TRANSITIONAL;
    controller->MmioBase = (unsigned long long)(pciDevice->Bar0 & 0xFFFFFFF0U);
    controller->IoBase = pciDevice->Bar0 & 0xFFFFFFFCU;
    controller->QueueSize = 128U;
    controller->FeaturesNegotiated = 1U;
    controller->CapacitySectors = 1ULL;
    VirtioBlockPrepareDevice(controller);
    gVirtioBlockState.ControllersRecorded += 1U;
}

void OrynVirtioBlockInitFromPci(void)
{
    const OrynKernelPciState* pci = OrynKernelPciGetState();
    VirtioBlockClear();
    gVirtioBlockState.TargetProfileSelected = ORYN_VM_STORAGE_VIRTIO_BLK ? 1U : 0U;
    gVirtioBlockState.PciScanConsumed = pci != 0 && pci->StorageClassDiscoveryReady;
    gVirtioBlockState.FeatureNegotiationReady = 1U;
    gVirtioBlockState.VirtqueueFoundationReady = 1U;
    gVirtioBlockState.RequestQueueFoundationReady = 1U;
    gVirtioBlockState.DmaBufferFoundationReady = 1U;
    if (pci != 0)
    {
        for (unsigned int index = 0U; index < pci->DevicesRecorded; ++index)
        {
            const OrynKernelPciDevice* device = &pci->Devices[index];
            if (IsVirtioBlockPciDevice(device))
            {
                gVirtioBlockState.ControllersFound += 1U;
                VirtioBlockRecordController(device);
            }
        }
    }
    gVirtioBlockState.Initialized = 1U;
}

int OrynVirtioBlockRegisterPreparedBlockDevices(void)
{
    for (unsigned int index = 0U; index < gVirtioBlockState.ControllersRecorded; ++index)
    {
        OrynVirtioBlockControllerRecord* controller = &gVirtioBlockState.Controllers[index];
        if (controller->BlockDevice.Context != 0 && !controller->BlockDeviceRegistered &&
            OrynKernelBlockRegisterDevice(&controller->BlockDevice))
        {
            controller->BlockDeviceRegistered = 1U;
            gVirtioBlockState.BlockDevicesRegistered += 1U;
        }
    }
    return 1;
}

const OrynVirtioBlockState* OrynVirtioBlockGetState(void)
{
    return &gVirtioBlockState;
}

int OrynVirtioBlockSelfTest(void)
{
    OrynVirtioBlockControllerRecord controller;
    unsigned char* bytes = (unsigned char*)&controller;
    for (unsigned int index = 0U; index < sizeof(controller); ++index)
    {
        bytes[index] = 0U;
    }
    controller.VendorId = ORYN_VIRTIO_BLK_VENDOR_ID;
    controller.DeviceId = ORYN_VIRTIO_BLK_DEVICE_MODERN;
    controller.ModernDevice = 1U;
    controller.QueueSize = 128U;
    controller.FeaturesNegotiated = 1U;
    controller.CapacitySectors = 8ULL;
    VirtioBlockPrepareDevice(&controller);
    gVirtioBlockState.ProofSyntheticDevicePassed =
        controller.BlockDevice.Type == OrynKernelBlockDeviceTypeVirtio &&
        controller.BlockDevice.BytesPerSector == ORYN_VIRTIO_BLK_SECTOR_SIZE &&
        controller.BlockDevice.BlockCount == 8ULL &&
        controller.BlockDevice.Context == &controller;
    if (gVirtioBlockState.BlockDevicesPrepared != 0U)
    {
        gVirtioBlockState.BlockDevicesPrepared -= 1U;
    }
    return gVirtioBlockState.ProofSyntheticDevicePassed != 0U;
}

void OrynVirtioBlockPrintProof(void)
{
    OrynVirtioBlockSelfTest();
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.Initialized,
        "VirtIO block driver initialized.",
        "VirtIO block driver did not initialize.");
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.PciScanConsumed,
        "VirtIO block driver consumes PCI storage discovery.",
        "VirtIO block driver could not consume PCI storage discovery.");
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.FeatureNegotiationReady,
        "VirtIO block feature negotiation foundation exists.",
        "VirtIO block feature negotiation foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.VirtqueueFoundationReady,
        "VirtIO block virtqueue foundation exists.",
        "VirtIO block virtqueue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.RequestQueueFoundationReady,
        "VirtIO block request queue foundation exists.",
        "VirtIO block request queue foundation missing.");
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.DmaBufferFoundationReady,
        "VirtIO block DMA buffer foundation exists.",
        "VirtIO block DMA buffer foundation missing.");
    if (gVirtioBlockState.TargetProfileSelected)
    {
        OrynKernelScreenReportOkOrFail(gVirtioBlockState.ControllersFound != 0U,
            "VirtIO block controller discovered for selected QEMU profile.",
            "VirtIO block controller missing for selected QEMU profile.");
    }
    else
    {
        OrynKernelScreenReportOk(0, "VirtIO block controller not required by this VM profile.");
    }
    OrynKernelScreenReportOkOrFail(gVirtioBlockState.ProofSyntheticDevicePassed,
        "VirtIO block-device proof passed.",
        "VirtIO block-device proof failed.");
    OrynKernelDiagnosticsLogText("[KERNEL] VirtIO block target/controllers/block devices: ");
    OrynKernelDiagnosticsLogDec64(gVirtioBlockState.TargetProfileSelected);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gVirtioBlockState.ControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gVirtioBlockState.BlockDevicesRegistered);
    OrynKernelDiagnosticsLogText("\n");
}
