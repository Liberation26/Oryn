#ifndef ORYN_KERNEL_NVME_H
#define ORYN_KERNEL_NVME_H

#include "KernelBlockDevice.h"
#include "KernelPci.h"

#define ORYN_NVME_MAX_CONTROLLERS 8U
#define ORYN_NVME_MAX_NAMESPACES 32U
#define ORYN_NVME_SECTOR_SIZE 512U
#define ORYN_NVME_CAP_MQES_MASK 0xFFFFULL
#define ORYN_NVME_CC_EN 0x00000001U
#define ORYN_NVME_CSTS_RDY 0x00000001U
#define ORYN_NVME_QUEUE_DEPTH 16U

typedef struct OrynNvmeRegisterBlock
{
    volatile unsigned long long Capabilities;
    volatile unsigned int Version;
    volatile unsigned int InterruptMaskSet;
    volatile unsigned int InterruptMaskClear;
    volatile unsigned int ControllerConfiguration;
    volatile unsigned int Reserved0;
    volatile unsigned int ControllerStatus;
    volatile unsigned int Reserved1;
    volatile unsigned int AdminQueueAttributes;
    volatile unsigned long long AdminSubmissionQueueBase;
    volatile unsigned long long AdminCompletionQueueBase;
} OrynNvmeRegisterBlock;

typedef struct OrynNvmeControllerRecord
{
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    unsigned int VendorId;
    unsigned int DeviceId;
    unsigned long long MbarPhysical;
    unsigned long long Capabilities;
    unsigned int Version;
    unsigned int ControllerStatus;
    unsigned int QueueDepth;
    unsigned int Enabled;
    unsigned int Ready;
} OrynNvmeControllerRecord;

typedef struct OrynNvmeNamespaceRecord
{
    unsigned int ControllerIndex;
    unsigned int NamespaceId;
    unsigned int Active;
    unsigned int BlockDeviceRegistered;
    unsigned long long BlockCount;
    unsigned int BytesPerSector;
    OrynKernelBlockDevice BlockDevice;
} OrynNvmeNamespaceRecord;

typedef struct OrynNvmeState
{
    unsigned int Initialized;
    unsigned int PciScanConsumed;
    unsigned int ControllersFound;
    unsigned int ControllersRecorded;
    unsigned int NamespacesDiscovered;
    unsigned int BlockDevicesPrepared;
    unsigned int BlockDevicesRegistered;
    unsigned int ReadRequests;
    unsigned int WriteRequests;
    unsigned int FlushRequests;
    unsigned int AdminQueueFoundationReady;
    unsigned int IoQueueFoundationReady;
    unsigned int PrpDmaFoundationReady;
    unsigned int IdentifyFoundationReady;
    unsigned int ProofSyntheticNamespacePassed;
    OrynNvmeControllerRecord Controllers[ORYN_NVME_MAX_CONTROLLERS];
    OrynNvmeNamespaceRecord Namespaces[ORYN_NVME_MAX_NAMESPACES];
} OrynNvmeState;

void OrynNvmeInitFromPci(void);
const OrynNvmeState* OrynNvmeGetState(void);
int OrynNvmeRegisterPreparedBlockDevices(void);
int OrynNvmeSelfTest(void);
void OrynNvmePrintProof(void);

#endif
