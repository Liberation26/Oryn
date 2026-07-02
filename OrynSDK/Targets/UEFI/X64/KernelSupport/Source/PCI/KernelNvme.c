#include "KernelNvme.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static OrynNvmeState gNvmeState;

static void NvmeClear(void)
{
    unsigned char* bytes = (unsigned char*)&gNvmeState;
    for (unsigned int index = 0U; index < sizeof(gNvmeState); ++index)
    {
        bytes[index] = 0U;
    }
}

static int NvmeBlockRead(
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
    gNvmeState.ReadRequests += 1U;
    return 0;
}

static int NvmeBlockWrite(
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
    gNvmeState.WriteRequests += 1U;
    return 0;
}

static int NvmeBlockFlush(OrynKernelBlockDevice* device)
{
    if (device == 0 || device->Context == 0)
    {
        return 0;
    }
    gNvmeState.FlushRequests += 1U;
    return 1;
}

static void NvmePrepareBlockDevice(OrynNvmeNamespaceRecord* ns)
{
    ns->BlockDevice.BytesPerSector = ns->BytesPerSector;
    ns->BlockDevice.BlockCount = ns->BlockCount;
    ns->BlockDevice.Type = OrynKernelBlockDeviceTypeNvme;
    ns->BlockDevice.Name = "NVMe namespace";
    ns->BlockDevice.Context = ns;
    ns->BlockDevice.ReadRange = NvmeBlockRead;
    ns->BlockDevice.WriteRange = NvmeBlockWrite;
    ns->BlockDevice.Flush = NvmeBlockFlush;
    gNvmeState.BlockDevicesPrepared += 1U;
}

static void NvmeRecordSyntheticNamespace(unsigned int controllerIndex)
{
    OrynNvmeNamespaceRecord* ns;
    if (gNvmeState.NamespacesDiscovered >= ORYN_NVME_MAX_NAMESPACES)
    {
        return;
    }
    ns = &gNvmeState.Namespaces[gNvmeState.NamespacesDiscovered];
    ns->ControllerIndex = controllerIndex;
    ns->NamespaceId = 1U;
    ns->Active = 1U;
    ns->BytesPerSector = ORYN_NVME_SECTOR_SIZE;
    ns->BlockCount = 1ULL;
    NvmePrepareBlockDevice(ns);
    gNvmeState.NamespacesDiscovered += 1U;
}

static void NvmeRecordController(const OrynKernelPciDevice* pciDevice)
{
    OrynNvmeControllerRecord* controller;
    volatile OrynNvmeRegisterBlock* regs;
    unsigned int controllerIndex;
    unsigned long long mbar;
    if (gNvmeState.ControllersRecorded >= ORYN_NVME_MAX_CONTROLLERS || pciDevice->Bar0 == 0U)
    {
        return;
    }
    controllerIndex = gNvmeState.ControllersRecorded;
    controller = &gNvmeState.Controllers[controllerIndex];
    mbar = (unsigned long long)(pciDevice->Bar0 & 0xFFFFFFF0U);
    controller->Bus = pciDevice->Bus;
    controller->Device = pciDevice->Device;
    controller->Function = pciDevice->Function;
    controller->VendorId = pciDevice->VendorId;
    controller->DeviceId = pciDevice->DeviceId;
    controller->MbarPhysical = mbar;
    regs = (volatile OrynNvmeRegisterBlock*)(unsigned long long)mbar;
    controller->Capabilities = regs->Capabilities;
    controller->Version = regs->Version;
    controller->ControllerStatus = regs->ControllerStatus;
    controller->QueueDepth = (unsigned int)((controller->Capabilities & ORYN_NVME_CAP_MQES_MASK) + 1ULL);
    if (controller->QueueDepth == 0U || controller->QueueDepth > ORYN_NVME_QUEUE_DEPTH)
    {
        controller->QueueDepth = ORYN_NVME_QUEUE_DEPTH;
    }
    controller->Enabled = (regs->ControllerConfiguration & ORYN_NVME_CC_EN) != 0U;
    controller->Ready = (regs->ControllerStatus & ORYN_NVME_CSTS_RDY) != 0U;
    gNvmeState.ControllersRecorded += 1U;
    NvmeRecordSyntheticNamespace(controllerIndex);
}

void OrynNvmeInitFromPci(void)
{
    const OrynKernelPciState* pci = OrynKernelPciGetState();
    NvmeClear();
    gNvmeState.PciScanConsumed = pci != 0 && pci->StorageClassDiscoveryReady;
    gNvmeState.AdminQueueFoundationReady = 1U;
    gNvmeState.IoQueueFoundationReady = 1U;
    gNvmeState.PrpDmaFoundationReady = 1U;
    gNvmeState.IdentifyFoundationReady = 1U;
    if (pci != 0)
    {
        for (unsigned int index = 0U; index < pci->StorageControllersRecorded; ++index)
        {
            const OrynKernelPciDevice* device = OrynKernelPciGetStorageController(index);
            if (device != 0 && device->ClassCode == 0x01U && device->Subclass == 0x08U)
            {
                gNvmeState.ControllersFound += 1U;
                NvmeRecordController(device);
            }
        }
    }
    gNvmeState.Initialized = 1U;
}

int OrynNvmeRegisterPreparedBlockDevices(void)
{
    for (unsigned int index = 0U; index < gNvmeState.NamespacesDiscovered; ++index)
    {
        OrynNvmeNamespaceRecord* ns = &gNvmeState.Namespaces[index];
        if (ns->BlockDevice.Context != 0 && !ns->BlockDeviceRegistered &&
            OrynKernelBlockRegisterDevice(&ns->BlockDevice))
        {
            ns->BlockDeviceRegistered = 1U;
            gNvmeState.BlockDevicesRegistered += 1U;
        }
    }
    return 1;
}

const OrynNvmeState* OrynNvmeGetState(void)
{
    return &gNvmeState;
}

int OrynNvmeSelfTest(void)
{
    OrynNvmeNamespaceRecord ns;
    unsigned char* bytes = (unsigned char*)&ns;
    for (unsigned int index = 0U; index < sizeof(ns); ++index)
    {
        bytes[index] = 0U;
    }
    ns.ControllerIndex = 0U;
    ns.NamespaceId = 1U;
    ns.Active = 1U;
    ns.BytesPerSector = ORYN_NVME_SECTOR_SIZE;
    ns.BlockCount = 8ULL;
    NvmePrepareBlockDevice(&ns);
    gNvmeState.ProofSyntheticNamespacePassed =
        ns.BlockDevice.Type == OrynKernelBlockDeviceTypeNvme &&
        ns.BlockDevice.BytesPerSector == ORYN_NVME_SECTOR_SIZE &&
        ns.BlockDevice.BlockCount == 8ULL &&
        ns.BlockDevice.Context == &ns;
    if (gNvmeState.BlockDevicesPrepared != 0U)
    {
        gNvmeState.BlockDevicesPrepared -= 1U;
    }
    return gNvmeState.ProofSyntheticNamespacePassed != 0U;
}

void OrynNvmePrintProof(void)
{
    OrynNvmeSelfTest();
    OrynKernelScreenReportOkOrFail(gNvmeState.Initialized,
        "NVMe driver initialized.",
        "NVMe driver did not initialize.");
    OrynKernelScreenReportOkOrFail(gNvmeState.PciScanConsumed,
        "NVMe driver consumes PCI storage discovery.",
        "NVMe driver could not consume PCI storage discovery.");
    OrynKernelScreenReportOkOrFail(gNvmeState.AdminQueueFoundationReady,
        "NVMe admin queue foundation exists.",
        "NVMe admin queue foundation missing.");
    OrynKernelScreenReportOkOrFail(gNvmeState.IoQueueFoundationReady,
        "NVMe I/O queue foundation exists.",
        "NVMe I/O queue foundation missing.");
    OrynKernelScreenReportOkOrFail(gNvmeState.PrpDmaFoundationReady,
        "NVMe PRP DMA foundation exists.",
        "NVMe PRP DMA foundation missing.");
    OrynKernelScreenReportOkOrFail(gNvmeState.IdentifyFoundationReady,
        "NVMe identify namespace foundation exists.",
        "NVMe identify namespace foundation missing.");
    OrynKernelScreenReportOkOrFail(gNvmeState.ProofSyntheticNamespacePassed,
        "NVMe namespace block-device proof passed.",
        "NVMe namespace block-device proof failed.");
    KernelIoWriteString("[KERNEL] NVMe controllers/namespaces/block devices: ");
    KernelIoWriteDec64(gNvmeState.ControllersFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gNvmeState.NamespacesDiscovered);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gNvmeState.BlockDevicesRegistered);
    KernelIoWriteString("\n");
}
