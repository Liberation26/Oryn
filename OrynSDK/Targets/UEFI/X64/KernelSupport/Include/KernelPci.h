#ifndef ORYN_KERNEL_PCI_H
#define ORYN_KERNEL_PCI_H

#include "OrynBootInfo.h"

#define ORYN_PCI_MAX_RECORDED_DEVICES 128U

typedef struct OrynKernelPciDevice
{
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    unsigned int VendorId;
    unsigned int DeviceId;
    unsigned int RevisionId;
    unsigned int ProgIf;
    unsigned int Subclass;
    unsigned int ClassCode;
    unsigned int HeaderType;
    unsigned int Bar0;
    unsigned int InterruptLine;
    unsigned int InterruptPin;
    unsigned int SecondaryBus;
} OrynKernelPciDevice;

typedef struct OrynKernelPciState
{
    unsigned int Initialized;
    unsigned int ConfigMechanism1Available;
    unsigned int AcpiRsdpPresent;
    unsigned int AcpiChecksumOk;
    unsigned int McfgTableFound;
    unsigned int McfgAllocationCount;
    unsigned int BusesScanned;
    unsigned int DeviceSlotsScanned;
    unsigned int FunctionSlotsScanned;
    unsigned int DevicesFound;
    unsigned int DevicesRecorded;
    unsigned int DeviceListTruncated;
    unsigned int MultifunctionDevices;
    unsigned int BridgesFound;
    unsigned int StorageControllersFound;
    unsigned int NetworkControllersFound;
    unsigned int DisplayControllersFound;
    unsigned int ClassDecodeReady;
    unsigned int FirstMcfgSegment;
    unsigned int FirstMcfgStartBus;
    unsigned int FirstMcfgEndBus;
    unsigned long long FirstMcfgBase;
    OrynKernelPciDevice Devices[ORYN_PCI_MAX_RECORDED_DEVICES];
} OrynKernelPciState;

void OrynKernelPciInit(const OrynBootInfo* bootInfo);
const OrynKernelPciState* OrynKernelPciGetState(void);
unsigned int OrynKernelPciConfigRead32(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset);
void OrynKernelPciConfigWrite32(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset,
    unsigned int value);
const char* OrynKernelPciClassName(unsigned int classCode);
void OrynKernelPciPrintProof(void);

#endif
