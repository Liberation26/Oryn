#ifndef ORYN_KERNEL_VIRTIO_BLOCK_H
#define ORYN_KERNEL_VIRTIO_BLOCK_H

#include "KernelBlockDevice.h"
#include "KernelPci.h"

#define ORYN_VIRTIO_BLK_MAX_CONTROLLERS 8U
#define ORYN_VIRTIO_BLK_SECTOR_SIZE 512U
#define ORYN_VIRTIO_BLK_VENDOR_ID 0x1AF4U
#define ORYN_VIRTIO_BLK_DEVICE_TRANSITIONAL 0x1001U
#define ORYN_VIRTIO_BLK_DEVICE_MODERN 0x1042U

#ifndef ORYN_VM_STORAGE_VIRTIO_BLK
#define ORYN_VM_STORAGE_VIRTIO_BLK 0
#endif

typedef struct OrynVirtioBlockControllerRecord
{
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    unsigned int VendorId;
    unsigned int DeviceId;
    unsigned int ModernDevice;
    unsigned int TransitionalDevice;
    unsigned long long MmioBase;
    unsigned int IoBase;
    unsigned int QueueSize;
    unsigned int FeaturesNegotiated;
    unsigned int BlockDeviceRegistered;
    unsigned long long CapacitySectors;
    OrynKernelBlockDevice BlockDevice;
} OrynVirtioBlockControllerRecord;

typedef struct OrynVirtioBlockState
{
    unsigned int Initialized;
    unsigned int TargetProfileSelected;
    unsigned int PciScanConsumed;
    unsigned int ControllersFound;
    unsigned int ControllersRecorded;
    unsigned int FeatureNegotiationReady;
    unsigned int VirtqueueFoundationReady;
    unsigned int RequestQueueFoundationReady;
    unsigned int DmaBufferFoundationReady;
    unsigned int BlockDevicesPrepared;
    unsigned int BlockDevicesRegistered;
    unsigned int ReadRequests;
    unsigned int WriteRequests;
    unsigned int FlushRequests;
    unsigned int ProofSyntheticDevicePassed;
    OrynVirtioBlockControllerRecord Controllers[ORYN_VIRTIO_BLK_MAX_CONTROLLERS];
} OrynVirtioBlockState;

void OrynVirtioBlockInitFromPci(void);
int OrynVirtioBlockRegisterPreparedBlockDevices(void);
const OrynVirtioBlockState* OrynVirtioBlockGetState(void);
int OrynVirtioBlockSelfTest(void);
void OrynVirtioBlockPrintProof(void);

#endif
