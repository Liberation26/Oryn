#ifndef ORYN_KERNEL_VIRTIO_GPU_H
#define ORYN_KERNEL_VIRTIO_GPU_H

#include "KernelPci.h"

#define ORYN_VIRTIO_GPU_VENDOR_ID 0x1AF4U
#define ORYN_VIRTIO_GPU_DEVICE_TRANSITIONAL 0x1010U
#define ORYN_VIRTIO_GPU_DEVICE_MODERN 0x1050U
#define ORYN_VIRTIO_GPU_MAX_CONTROLLERS 8U
#define ORYN_VIRTIO_GPU_MAX_SCANOUTS 4U

#define ORYN_VIRTIO_GPU_FEATURE_VIRGL 0x00000001U
#define ORYN_VIRTIO_GPU_FEATURE_EDID 0x00000002U
#define ORYN_VIRTIO_GPU_FEATURE_RESOURCE_BLOB 0x00000004U
#define ORYN_VIRTIO_GPU_FEATURE_CONTEXT_INIT 0x00000008U

typedef struct OrynVirtioGpuScanoutRecord
{
    unsigned int Enabled;
    unsigned int Width;
    unsigned int Height;
    unsigned int StrideBytes;
    unsigned long long FramebufferPhysical;
} OrynVirtioGpuScanoutRecord;

typedef struct OrynVirtioGpuControllerRecord
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
    unsigned int FeaturesNegotiated;
    unsigned int ControlVirtqueueReady;
    unsigned int CursorVirtqueueReady;
    unsigned int DisplayOutputReady;
    unsigned int Resource2DReady;
    unsigned int Resource3DReady;
    unsigned int DmaBufferReady;
    unsigned int EdidFoundationReady;
    unsigned int ScanoutsRecorded;
    OrynVirtioGpuScanoutRecord Scanouts[ORYN_VIRTIO_GPU_MAX_SCANOUTS];
} OrynVirtioGpuControllerRecord;

typedef struct OrynVirtioGpuState
{
    unsigned int Initialized;
    unsigned int PciDisplayDiscoveryConsumed;
    unsigned int ControllersFound;
    unsigned int ControllersRecorded;
    unsigned int FeatureNegotiationReady;
    unsigned int ControlVirtqueueFoundationReady;
    unsigned int CursorVirtqueueFoundationReady;
    unsigned int DisplayOutputFoundationReady;
    unsigned int Resource2DFoundationReady;
    unsigned int Resource3DFoundationReady;
    unsigned int DmaBufferFoundationReady;
    unsigned int EdidFoundationReady;
    unsigned int InterruptFoundationReady;
    unsigned int LinkToDisplaySubsystemReady;
    unsigned int ProofSyntheticDevicePassed;
    OrynVirtioGpuControllerRecord Controllers[ORYN_VIRTIO_GPU_MAX_CONTROLLERS];
} OrynVirtioGpuState;

void OrynVirtioGpuInitFromPci(void);
const OrynVirtioGpuState* OrynVirtioGpuGetState(void);
int OrynVirtioGpuSelfTest(void);
void OrynVirtioGpuPrintProof(void);

#endif
