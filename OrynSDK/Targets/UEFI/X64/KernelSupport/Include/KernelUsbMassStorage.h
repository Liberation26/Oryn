#ifndef ORYN_KERNEL_USB_MASS_STORAGE_H
#define ORYN_KERNEL_USB_MASS_STORAGE_H

#include "KernelBlockDevice.h"
#include "KernelPci.h"

#define ORYN_USB_STORAGE_MAX_HOSTS 16U
#define ORYN_USB_STORAGE_MAX_DEVICES 32U
#define ORYN_USB_STORAGE_SECTOR_SIZE 512U

#define ORYN_USB_CONTROLLER_UHCI 0U
#define ORYN_USB_CONTROLLER_OHCI 0x10U
#define ORYN_USB_CONTROLLER_EHCI 0x20U
#define ORYN_USB_CONTROLLER_XHCI 0x30U

#define ORYN_USB_MASS_SUBCLASS_SCSI 0x06U
#define ORYN_USB_MASS_PROTOCOL_BOT 0x50U
#define ORYN_USB_SCSI_READ10 0x28U
#define ORYN_USB_SCSI_WRITE10 0x2AU
#define ORYN_USB_SCSI_INQUIRY 0x12U
#define ORYN_USB_SCSI_READ_CAPACITY10 0x25U

typedef struct OrynUsbMassStorageHostRecord
{
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    unsigned int VendorId;
    unsigned int DeviceId;
    unsigned int ProgIf;
    unsigned int ControllerKind;
    unsigned int MmioBar;
    unsigned int Registered;
} OrynUsbMassStorageHostRecord;

typedef struct OrynUsbMassStorageDeviceRecord
{
    unsigned int HostIndex;
    unsigned int UsbAddress;
    unsigned int InterfaceNumber;
    unsigned int Lun;
    unsigned int Subclass;
    unsigned int Protocol;
    unsigned int BulkInEndpoint;
    unsigned int BulkOutEndpoint;
    unsigned int BlockDeviceRegistered;
    unsigned long long BlockCount;
    unsigned int BytesPerSector;
    OrynKernelBlockDevice BlockDevice;
} OrynUsbMassStorageDeviceRecord;

typedef struct OrynUsbMassStorageState
{
    unsigned int Initialized;
    unsigned int PciScanConsumed;
    unsigned int HostControllersFound;
    unsigned int HostControllersRecorded;
    unsigned int MassStorageInterfacesFound;
    unsigned int BlockDevicesPrepared;
    unsigned int BlockDevicesRegistered;
    unsigned int ReadRequests;
    unsigned int WriteRequests;
    unsigned int FlushRequests;
    unsigned int EnumerationFoundationReady;
    unsigned int BulkOnlyTransportReady;
    unsigned int ScsiCommandFoundationReady;
    unsigned int DmaBufferFoundationReady;
    unsigned int ProofSyntheticDevicePassed;
    OrynUsbMassStorageHostRecord Hosts[ORYN_USB_STORAGE_MAX_HOSTS];
    OrynUsbMassStorageDeviceRecord Devices[ORYN_USB_STORAGE_MAX_DEVICES];
} OrynUsbMassStorageState;

void OrynUsbMassStorageInitFromPci(void);
const OrynUsbMassStorageState* OrynUsbMassStorageGetState(void);
int OrynUsbMassStorageRegisterPreparedBlockDevices(void);
int OrynUsbMassStorageSelfTest(void);
void OrynUsbMassStoragePrintProof(void);

#endif
