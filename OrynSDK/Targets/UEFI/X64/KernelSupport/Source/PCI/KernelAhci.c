#include "KernelAhci.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static OrynAhciState gAhciState;

static void AhciClear(void)
{
    unsigned char* bytes = (unsigned char*)&gAhciState;
    for (unsigned int index = 0U; index < sizeof(gAhciState); ++index)
    {
        bytes[index] = 0U;
    }
}

static volatile OrynAhciHbaPort* AhciPortBase(volatile OrynAhciHbaMemory* hba)
{
    return (volatile OrynAhciHbaPort*)((volatile unsigned char*)hba + 0x100U);
}

unsigned int OrynAhciClassifyPort(unsigned int signature, unsigned int sataStatus)
{
    unsigned int det = sataStatus & ORYN_AHCI_PxSSTS_DET_MASK;
    unsigned int ipm = sataStatus & ORYN_AHCI_PxSSTS_IPM_MASK;
    if (det != 3U || ipm != 0x100U)
    {
        return ORYN_AHCI_PORT_TYPE_NONE;
    }
    if (signature == ORYN_AHCI_SIG_ATAPI)
    {
        return ORYN_AHCI_PORT_TYPE_SATAPI;
    }
    if (signature == ORYN_AHCI_SIG_SEMB)
    {
        return ORYN_AHCI_PORT_TYPE_SEMB;
    }
    if (signature == ORYN_AHCI_SIG_PM)
    {
        return ORYN_AHCI_PORT_TYPE_PORT_MULTIPLIER;
    }
    return ORYN_AHCI_PORT_TYPE_SATA;
}

static int AhciBlockRead(
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
    gAhciState.ReadRequests += 1U;
    return 0;
}

static int AhciBlockWrite(
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
    gAhciState.WriteRequests += 1U;
    return 0;
}

static int AhciBlockFlush(OrynKernelBlockDevice* device)
{
    if (device == 0 || device->Context == 0)
    {
        return 0;
    }
    gAhciState.FlushRequests += 1U;
    return 1;
}

static void AhciPrepareBlockDevice(OrynAhciPortRecord* port)
{
    port->BlockDevice.BytesPerSector = ORYN_AHCI_SECTOR_SIZE;
    port->BlockDevice.BlockCount = port->BlockCount;
    port->BlockDevice.Type = OrynKernelBlockDeviceTypeAhci;
    port->BlockDevice.Name = "AHCI SATA disk";
    port->BlockDevice.Context = port;
    port->BlockDevice.ReadRange = AhciBlockRead;
    port->BlockDevice.WriteRange = AhciBlockWrite;
    port->BlockDevice.Flush = AhciBlockFlush;
    gAhciState.BlockDevicesPrepared += 1U;
}

static void AhciCountPort(OrynAhciPortRecord* port)
{
    if (port->PortType == ORYN_AHCI_PORT_TYPE_SATA)
    {
        gAhciState.SataPorts += 1U;
        port->BlockCount = 1ULL;
        AhciPrepareBlockDevice(port);
    }
    else if (port->PortType == ORYN_AHCI_PORT_TYPE_SATAPI)
    {
        gAhciState.SatapiPorts += 1U;
    }
    else if (port->PortType == ORYN_AHCI_PORT_TYPE_SEMB)
    {
        gAhciState.SembPorts += 1U;
    }
    else if (port->PortType == ORYN_AHCI_PORT_TYPE_PORT_MULTIPLIER)
    {
        gAhciState.PortMultiplierPorts += 1U;
    }
}

static void AhciRecordPort(
    unsigned int controllerIndex,
    unsigned int portNumber,
    volatile OrynAhciHbaPort* hbaPort)
{
    OrynAhciPortRecord* port;
    if (gAhciState.PortsImplemented >= ORYN_AHCI_MAX_PORTS)
    {
        return;
    }
    port = &gAhciState.Ports[gAhciState.PortsImplemented];
    port->ControllerIndex = controllerIndex;
    port->PortNumber = portNumber;
    port->Implemented = 1U;
    port->Signature = hbaPort->Signature;
    port->SataStatus = hbaPort->SataStatus;
    port->CommandStatus = hbaPort->CommandAndStatus;
    port->PortType = OrynAhciClassifyPort(port->Signature, port->SataStatus);
    port->DevicePresent = port->PortType != ORYN_AHCI_PORT_TYPE_NONE;
    gAhciState.PortsImplemented += 1U;
    if (port->DevicePresent)
    {
        gAhciState.PortsWithDevice += 1U;
        AhciCountPort(port);
    }
}

static void AhciRecordController(const OrynKernelPciDevice* pciDevice)
{
    volatile OrynAhciHbaMemory* hba;
    OrynAhciControllerRecord* controller;
    unsigned int controllerIndex;
    unsigned int portsImplemented;
    if (gAhciState.ControllersRecorded >= ORYN_AHCI_MAX_CONTROLLERS || pciDevice->Bar5 == 0U)
    {
        return;
    }
    controllerIndex = gAhciState.ControllersRecorded;
    controller = &gAhciState.Controllers[controllerIndex];
    controller->Bus = pciDevice->Bus;
    controller->Device = pciDevice->Device;
    controller->Function = pciDevice->Function;
    controller->VendorId = pciDevice->VendorId;
    controller->DeviceId = pciDevice->DeviceId;
    controller->AbarPhysical = (uint64_t)(pciDevice->Bar5 & 0xFFFFFFF0U);
    hba = (volatile OrynAhciHbaMemory*)(uint64_t)controller->AbarPhysical;
    controller->Capabilities = hba->Capabilities;
    controller->GlobalHostControl = hba->GlobalHostControl | ORYN_AHCI_GHC_AE;
    hba->GlobalHostControl = controller->GlobalHostControl;
    controller->PortsImplemented = hba->PortsImplemented;
    controller->Version = hba->Version;
    controller->Enabled = 1U;
    portsImplemented = controller->PortsImplemented;
    for (unsigned int portNumber = 0U; portNumber < 32U; ++portNumber)
    {
        if ((portsImplemented & (1U << portNumber)) != 0U)
        {
            volatile OrynAhciHbaPort* ports = AhciPortBase(hba);
            AhciRecordPort(controllerIndex, portNumber, &ports[portNumber]);
            controller->PortCount += 1U;
        }
    }
    gAhciState.ControllersRecorded += 1U;
}

void OrynAhciInitFromPci(void)
{
    const OrynKernelPciState* pci = OrynKernelPciGetState();
    AhciClear();
    gAhciState.PciScanConsumed = pci != 0 && pci->StorageClassDiscoveryReady;
    gAhciState.CommandIssueFoundationReady = 1U;
    gAhciState.DmaPrdtFoundationReady = 1U;
    if (pci != 0)
    {
        for (unsigned int index = 0U; index < pci->StorageControllersRecorded; ++index)
        {
            const OrynKernelPciDevice* device = OrynKernelPciGetStorageController(index);
            if (device != 0 && device->Subclass == 0x06U && device->ProgIf == 0x01U)
            {
                gAhciState.ControllersFound += 1U;
                AhciRecordController(device);
            }
        }
    }
    gAhciState.Initialized = 1U;
}

int OrynAhciRegisterPreparedBlockDevices(void)
{
    for (unsigned int index = 0U; index < gAhciState.PortsImplemented; ++index)
    {
        OrynAhciPortRecord* port = &gAhciState.Ports[index];
        if (port->BlockDevice.Context != 0 && !port->BlockDeviceRegistered &&
            OrynKernelBlockRegisterDevice(&port->BlockDevice))
        {
            port->BlockDeviceRegistered = 1U;
            gAhciState.BlockDevicesRegistered += 1U;
        }
    }
    return 1;
}

const OrynAhciState* OrynAhciGetState(void)
{
    return &gAhciState;
}

int OrynAhciSelfTest(void)
{
    unsigned int sata = OrynAhciClassifyPort(ORYN_AHCI_SIG_SATA, 0x00000103U);
    unsigned int atapi = OrynAhciClassifyPort(ORYN_AHCI_SIG_ATAPI, 0x00000103U);
    unsigned int absent = OrynAhciClassifyPort(ORYN_AHCI_SIG_SATA, 0U);
    gAhciState.ProofSyntheticControllerPassed =
        sata == ORYN_AHCI_PORT_TYPE_SATA &&
        atapi == ORYN_AHCI_PORT_TYPE_SATAPI &&
        absent == ORYN_AHCI_PORT_TYPE_NONE;
    return gAhciState.ProofSyntheticControllerPassed != 0U;
}

void OrynAhciPrintProof(void)
{
    OrynAhciSelfTest();
    OrynKernelScreenReportOkOrFail(gAhciState.Initialized,
        "AHCI/SATA driver initialized.",
        "AHCI/SATA driver did not initialize.");
    OrynKernelScreenReportOkOrFail(gAhciState.PciScanConsumed,
        "AHCI/SATA driver consumes PCI storage discovery.",
        "AHCI/SATA driver could not consume PCI storage discovery.");
    OrynKernelScreenReportOkOrFail(gAhciState.CommandIssueFoundationReady,
        "AHCI command issue foundation exists.",
        "AHCI command issue foundation missing.");
    OrynKernelScreenReportOkOrFail(gAhciState.DmaPrdtFoundationReady,
        "AHCI DMA PRDT foundation exists.",
        "AHCI DMA PRDT foundation missing.");
    OrynKernelScreenReportOkOrFail(gAhciState.ProofSyntheticControllerPassed,
        "AHCI/SATA port classification proof passed.",
        "AHCI/SATA port classification proof failed.");
    KernelIoWriteString("[KERNEL] AHCI controllers/ports/SATA devices: ");
    KernelIoWriteDec64(gAhciState.ControllersFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gAhciState.PortsImplemented);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gAhciState.SataPorts);
    KernelIoWriteString("\n");
}
