#ifndef ORYN_KERNEL_VIRTIO_NET_H
#define ORYN_KERNEL_VIRTIO_NET_H

#include "KernelPci.h"

#define ORYN_VIRTIO_NET_VENDOR_ID 0x1AF4U
#define ORYN_VIRTIO_NET_DEVICE_TRANSITIONAL 0x1000U
#define ORYN_VIRTIO_NET_DEVICE_MODERN 0x1041U
#define ORYN_VIRTIO_NET_MAX_CONTROLLERS 8U
#define ORYN_VIRTIO_NET_MAC_BYTES 6U

typedef struct OrynVirtioNetControllerRecord
{
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    unsigned int VendorId;
    unsigned int DeviceId;
    unsigned int ModernDevice;
    unsigned int TransitionalDevice;
    unsigned int MsiCapable;
    unsigned int MsixCapable;
    unsigned int AssignedVector;
    unsigned int MmioBase;
    unsigned int IoBase;
    unsigned int RxQueueSize;
    unsigned int TxQueueSize;
    unsigned int FeaturesNegotiated;
    unsigned int RxVirtqueueReady;
    unsigned int TxVirtqueueReady;
    unsigned int ControlVirtqueueReady;
    unsigned int DmaBufferReady;
    unsigned int MacAddressValid;
    unsigned char MacAddress[ORYN_VIRTIO_NET_MAC_BYTES];
} OrynVirtioNetControllerRecord;

typedef struct OrynVirtioNetState
{
    unsigned int Initialized;
    unsigned int PciNetworkDiscoveryConsumed;
    unsigned int ControllersFound;
    unsigned int ControllersRecorded;
    unsigned int FeatureNegotiationReady;
    unsigned int RxVirtqueueFoundationReady;
    unsigned int TxVirtqueueFoundationReady;
    unsigned int ControlVirtqueueFoundationReady;
    unsigned int PacketBufferFoundationReady;
    unsigned int MacAddressFoundationReady;
    unsigned int InterruptFoundationReady;
    unsigned int LinkStatusFoundationReady;
    unsigned int PacketsReceived;
    unsigned int PacketsTransmitted;
    unsigned int ProofSyntheticDevicePassed;
    OrynVirtioNetControllerRecord Controllers[ORYN_VIRTIO_NET_MAX_CONTROLLERS];
} OrynVirtioNetState;

void OrynVirtioNetInitFromPci(void);
const OrynVirtioNetState* OrynVirtioNetGetState(void);
int OrynVirtioNetSelfTest(void);
void OrynVirtioNetPrintProof(void);

#endif
