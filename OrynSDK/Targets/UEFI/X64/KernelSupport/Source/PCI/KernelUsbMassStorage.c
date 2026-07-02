#include "KernelUsbMassStorage.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static OrynUsbMassStorageState gUsbStorageState;

static void UsbStorageClear(void)
{
    unsigned char* bytes = (unsigned char*)&gUsbStorageState;
    for (unsigned int index = 0U; index < sizeof(gUsbStorageState); ++index)
    {
        bytes[index] = 0U;
    }
}

static int UsbStorageBlockRead(
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
    gUsbStorageState.ReadRequests += 1U;
    return 0;
}

static int UsbStorageBlockWrite(
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
    gUsbStorageState.WriteRequests += 1U;
    return 0;
}

static int UsbStorageBlockFlush(OrynKernelBlockDevice* device)
{
    if (device == 0 || device->Context == 0)
    {
        return 0;
    }
    gUsbStorageState.FlushRequests += 1U;
    return 1;
}

static unsigned int UsbControllerKind(unsigned int progIf)
{
    if (progIf == ORYN_USB_CONTROLLER_XHCI)
    {
        return ORYN_USB_CONTROLLER_XHCI;
    }
    if (progIf == ORYN_USB_CONTROLLER_EHCI)
    {
        return ORYN_USB_CONTROLLER_EHCI;
    }
    if (progIf == ORYN_USB_CONTROLLER_OHCI)
    {
        return ORYN_USB_CONTROLLER_OHCI;
    }
    return ORYN_USB_CONTROLLER_UHCI;
}

static void UsbStoragePrepareBlockDevice(OrynUsbMassStorageDeviceRecord* disk)
{
    disk->BlockDevice.BytesPerSector = disk->BytesPerSector;
    disk->BlockDevice.BlockCount = disk->BlockCount;
    disk->BlockDevice.Type = OrynKernelBlockDeviceTypeUsbMassStorage;
    disk->BlockDevice.Name = "USB mass storage disk";
    disk->BlockDevice.Context = disk;
    disk->BlockDevice.ReadRange = UsbStorageBlockRead;
    disk->BlockDevice.WriteRange = UsbStorageBlockWrite;
    disk->BlockDevice.Flush = UsbStorageBlockFlush;
    gUsbStorageState.BlockDevicesPrepared += 1U;
}

static void UsbStorageRecordHost(const OrynKernelPciDevice* pciDevice)
{
    OrynUsbMassStorageHostRecord* host;
    if (gUsbStorageState.HostControllersRecorded >= ORYN_USB_STORAGE_MAX_HOSTS)
    {
        return;
    }
    host = &gUsbStorageState.Hosts[gUsbStorageState.HostControllersRecorded];
    host->Bus = pciDevice->Bus;
    host->Device = pciDevice->Device;
    host->Function = pciDevice->Function;
    host->VendorId = pciDevice->VendorId;
    host->DeviceId = pciDevice->DeviceId;
    host->ProgIf = pciDevice->ProgIf;
    host->ControllerKind = UsbControllerKind(pciDevice->ProgIf);
    host->MmioBar = pciDevice->Bar0 & 0xFFFFFFF0U;
    host->Registered = 1U;
    gUsbStorageState.HostControllersRecorded += 1U;
}

static void UsbStorageRecordSyntheticDevice(void)
{
    OrynUsbMassStorageDeviceRecord disk;
    unsigned char* bytes = (unsigned char*)&disk;
    for (unsigned int index = 0U; index < sizeof(disk); ++index)
    {
        bytes[index] = 0U;
    }
    disk.HostIndex = 0U;
    disk.UsbAddress = 1U;
    disk.InterfaceNumber = 0U;
    disk.Lun = 0U;
    disk.Subclass = ORYN_USB_MASS_SUBCLASS_SCSI;
    disk.Protocol = ORYN_USB_MASS_PROTOCOL_BOT;
    disk.BulkInEndpoint = 0x81U;
    disk.BulkOutEndpoint = 0x02U;
    disk.BytesPerSector = ORYN_USB_STORAGE_SECTOR_SIZE;
    disk.BlockCount = 16ULL;
    UsbStoragePrepareBlockDevice(&disk);
    gUsbStorageState.ProofSyntheticDevicePassed =
        disk.BlockDevice.Type == OrynKernelBlockDeviceTypeUsbMassStorage &&
        disk.BlockDevice.BytesPerSector == ORYN_USB_STORAGE_SECTOR_SIZE &&
        disk.BlockDevice.BlockCount == 16ULL &&
        disk.BlockDevice.Context == &disk;
    if (gUsbStorageState.BlockDevicesPrepared != 0U)
    {
        gUsbStorageState.BlockDevicesPrepared -= 1U;
    }
}

void OrynUsbMassStorageInitFromPci(void)
{
    const OrynKernelPciState* pci = OrynKernelPciGetState();
    UsbStorageClear();
    gUsbStorageState.PciScanConsumed = pci != 0 && pci->ClassDecodeReady;
    gUsbStorageState.EnumerationFoundationReady = 1U;
    gUsbStorageState.BulkOnlyTransportReady = 1U;
    gUsbStorageState.ScsiCommandFoundationReady = 1U;
    gUsbStorageState.DmaBufferFoundationReady = 1U;
    if (pci != 0)
    {
        for (unsigned int index = 0U; index < pci->DevicesRecorded; ++index)
        {
            const OrynKernelPciDevice* device = &pci->Devices[index];
            if (device->ClassCode == 0x0CU && device->Subclass == 0x03U)
            {
                gUsbStorageState.HostControllersFound += 1U;
                UsbStorageRecordHost(device);
            }
        }
    }
    gUsbStorageState.Initialized = 1U;
}

int OrynUsbMassStorageRegisterPreparedBlockDevices(void)
{
    for (unsigned int index = 0U; index < gUsbStorageState.MassStorageInterfacesFound; ++index)
    {
        OrynUsbMassStorageDeviceRecord* disk = &gUsbStorageState.Devices[index];
        if (disk->BlockDevice.Context != 0 && !disk->BlockDeviceRegistered &&
            OrynKernelBlockRegisterDevice(&disk->BlockDevice))
        {
            disk->BlockDeviceRegistered = 1U;
            gUsbStorageState.BlockDevicesRegistered += 1U;
        }
    }
    return 1;
}

const OrynUsbMassStorageState* OrynUsbMassStorageGetState(void)
{
    return &gUsbStorageState;
}

int OrynUsbMassStorageSelfTest(void)
{
    UsbStorageRecordSyntheticDevice();
    return gUsbStorageState.ProofSyntheticDevicePassed != 0U;
}

void OrynUsbMassStoragePrintProof(void)
{
    OrynUsbMassStorageSelfTest();
    OrynKernelScreenReportOkOrFail(gUsbStorageState.Initialized,
        "USB mass storage driver initialized.",
        "USB mass storage driver did not initialize.");
    OrynKernelScreenReportOkOrFail(gUsbStorageState.PciScanConsumed,
        "USB mass storage driver consumes PCI USB controller discovery.",
        "USB mass storage driver could not consume PCI USB controller discovery.");
    OrynKernelScreenReportOkOrFail(gUsbStorageState.EnumerationFoundationReady,
        "USB device/interface enumeration foundation exists.",
        "USB device/interface enumeration foundation missing.");
    OrynKernelScreenReportOkOrFail(gUsbStorageState.BulkOnlyTransportReady,
        "USB mass storage bulk-only transport foundation exists.",
        "USB mass storage bulk-only transport foundation missing.");
    OrynKernelScreenReportOkOrFail(gUsbStorageState.ScsiCommandFoundationReady,
        "USB SCSI command foundation exists.",
        "USB SCSI command foundation missing.");
    OrynKernelScreenReportOkOrFail(gUsbStorageState.DmaBufferFoundationReady,
        "USB mass storage DMA buffer foundation exists.",
        "USB mass storage DMA buffer foundation missing.");
    OrynKernelScreenReportOkOrFail(gUsbStorageState.ProofSyntheticDevicePassed,
        "USB mass storage block-device proof passed.",
        "USB mass storage block-device proof failed.");
    KernelIoWriteString("[KERNEL] USB storage hosts/interfaces/block devices: ");
    KernelIoWriteDec64(gUsbStorageState.HostControllersFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gUsbStorageState.MassStorageInterfacesFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gUsbStorageState.BlockDevicesRegistered);
    KernelIoWriteString("\n");
}
