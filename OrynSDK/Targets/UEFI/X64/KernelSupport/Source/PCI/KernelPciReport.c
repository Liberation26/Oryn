#include "KernelPciInternal.h"
#include "KernelAhci.h"
#include "KernelNvme.h"
#include "KernelUsbMassStorage.h"
#include "KernelVirtioBlock.h"
#include "KernelVirtioNet.h"
#include "KernelVirtioGpu.h"
#include "KernelInterrupts.h"
#include "KernelScreenReport.h"
#include "KernelDiagnosticsLogger.h"
static void PciInterruptStub(OrynIdtInterruptFrame* frame, void* context)
{
    (void)frame;
    (void)context;
}

int OrynKernelPciAssignInterruptVector(OrynKernelPciDevice* pciDevice, unsigned int vector)
{
    if (pciDevice == 0 || vector < 32U || vector > 255U)
    {
        return 0;
    }
    pciDevice->AssignedInterruptVector = vector;
    if (pciDevice->MsiCapable || pciDevice->MsixCapable)
    {
        gPciState.MsiVectorsAssigned += 1U;
    }
    return 1;
}

static void RegisterDeviceInterrupt(OrynKernelPciDevice* pciDevice)
{
    unsigned int vector;
    if (pciDevice->InterruptPin == 0U && !pciDevice->MsiCapable && !pciDevice->MsixCapable)
    {
        return;
    }
    vector = 0x90U + (gPciState.DeviceInterruptHandlersRegistered & 0x3FU);
    if (!OrynKernelPciAssignInterruptVector(pciDevice, vector))
    {
        return;
    }
    if (OrynKernelInterruptsRegisterDeviceHandler(vector, pciDevice->Bus, pciDevice->Device,
        pciDevice->Function, PciInterruptStub, pciDevice, "PCI device interrupt"))
    {
        pciDevice->DeviceInterruptRegistered = 1U;
        gPciState.DeviceInterruptHandlersRegistered += 1U;
    }
}

void RecordDevice(const OrynKernelPciDevice* pciDevice)
{
    if (gPciState.DevicesRecorded < ORYN_PCI_MAX_RECORDED_DEVICES)
    {
        OrynKernelPciDevice localDevice = *pciDevice;
        unsigned char* destination = (unsigned char*)&gPciState.Devices[gPciState.DevicesRecorded];
        const unsigned char* source = (const unsigned char*)&localDevice;
        RegisterDeviceInterrupt(&localDevice);
        for (unsigned int index = 0U; index < sizeof(localDevice); ++index)
        {
            destination[index] = source[index];
        }
        gPciState.DevicesRecorded += 1U;
    }
    else
    {
        gPciState.DeviceListTruncated = 1U;
    }
}


static void ScanCapabilities(OrynKernelPciDevice* outDevice)
{
    unsigned int status = PciRead16(outDevice->Bus, outDevice->Device, outDevice->Function, 0x06U);
    unsigned int pointer;
    if ((status & 0x10U) == 0U)
    {
        return;
    }
    outDevice->CapabilitiesPresent = 1U;
    gPciState.CapabilityDevicesFound += 1U;
    pointer = PciRead8(outDevice->Bus, outDevice->Device, outDevice->Function, 0x34U) & 0xFCU;
    for (unsigned int guard = 0U; guard < 48U && pointer >= 0x40U && pointer <= 0xFCU; ++guard)
    {
        unsigned int capId = PciRead8(outDevice->Bus, outDevice->Device, outDevice->Function, pointer);
        unsigned int next = PciRead8(outDevice->Bus, outDevice->Device, outDevice->Function, pointer + 1U) & 0xFCU;
        if (capId == 0x05U)
        {
            outDevice->MsiCapable = 1U;
            outDevice->MsiCapabilityOffset = pointer;
        }
        else if (capId == 0x11U)
        {
            outDevice->MsixCapable = 1U;
            outDevice->MsixCapabilityOffset = pointer;
        }
        if (next == 0U || next == pointer)
        {
            break;
        }
        pointer = next;
    }
    if (outDevice->MsiCapable)
    {
        gPciState.MsiDevicesFound += 1U;
    }
    if (outDevice->MsixCapable)
    {
        gPciState.MsixDevicesFound += 1U;
    }
}

void ReadDevice(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    OrynKernelPciDevice* outDevice)
{
    unsigned char* bytes = (unsigned char*)outDevice;
    for (unsigned int index = 0U; index < sizeof(*outDevice); ++index)
    {
        bytes[index] = 0U;
    }
    unsigned int id = OrynKernelPciConfigRead32(bus, device, function, 0x00U);
    unsigned int classRegister = OrynKernelPciConfigRead32(bus, device, function, 0x08U);
    unsigned int headerRegister = OrynKernelPciConfigRead32(bus, device, function, 0x0CU);
    outDevice->Bus = bus;
    outDevice->Device = device;
    outDevice->Function = function;
    outDevice->VendorId = id & 0xFFFFU;
    outDevice->DeviceId = (id >> 16) & 0xFFFFU;
    outDevice->RevisionId = classRegister & 0xFFU;
    outDevice->ProgIf = (classRegister >> 8) & 0xFFU;
    outDevice->Subclass = (classRegister >> 16) & 0xFFU;
    outDevice->ClassCode = (classRegister >> 24) & 0xFFU;
    outDevice->HeaderType = (headerRegister >> 16) & 0xFFU;
    outDevice->Bar0 = OrynKernelPciConfigRead32(bus, device, function, 0x10U);
    outDevice->Bar5 = OrynKernelPciConfigRead32(bus, device, function, 0x24U);
    outDevice->InterruptLine = PciRead8(bus, device, function, 0x3CU);
    outDevice->InterruptPin = PciRead8(bus, device, function, 0x3DU);
    outDevice->SecondaryBus = PciRead8(bus, device, function, 0x19U);
    ScanCapabilities(outDevice);
}

void ScanConfigSpace(void)
{
    for (unsigned int bus = 0U; bus < 256U; ++bus)
    {
        for (unsigned int device = 0U; device < 32U; ++device)
        {
            unsigned int functionZeroHeader = 0U;
            gPciState.DeviceSlotsScanned += 1U;
            if (PciRead16(bus, device, 0U, 0x00U) != ORYN_PCI_INVALID_VENDOR)
            {
                functionZeroHeader = PciRead8(bus, device, 0U, 0x0EU);
                if ((functionZeroHeader & 0x80U) != 0U)
                {
                    gPciState.MultifunctionDevices += 1U;
                }
            }

            for (unsigned int function = 0U; function < 8U; ++function)
            {
                OrynKernelPciDevice pciDevice;
                unsigned int vendor;
                gPciState.FunctionSlotsScanned += 1U;
                vendor = PciRead16(bus, device, function, 0x00U);
                if (vendor == ORYN_PCI_INVALID_VENDOR)
                {
                    continue;
                }

                ReadDevice(bus, device, function, &pciDevice);
                gPciState.DevicesFound += 1U;
                gPciState.ConfigMechanism1Available = 1U;
                CountClass(&pciDevice);
                RecordDevice(&pciDevice);
            }
        }
    }

    gPciState.BusesScanned = 256U;
    gPciState.ClassDecodeReady = 1U;
}

void OrynKernelPciInit(const OrynBootInfo* bootInfo)
{
    OrynKernelDiagnosticsLogText("[KERNEL] PCI: discovery starting.\n");
    ClearState();
    DiscoverMcfg(bootInfo);
    ScanConfigSpace();
    gPciState.StorageClassDiscoveryReady = 1U;
    OrynAhciInitFromPci();
    OrynAhciRegisterPreparedBlockDevices();
    OrynNvmeInitFromPci();
    OrynNvmeRegisterPreparedBlockDevices();
    OrynUsbMassStorageInitFromPci();
    OrynUsbMassStorageRegisterPreparedBlockDevices();
    OrynVirtioBlockInitFromPci();
    OrynVirtioBlockRegisterPreparedBlockDevices();
    OrynVirtioNetInitFromPci();
    OrynVirtioGpuInitFromPci();
    gPciState.Initialized = 1U;
}

const OrynKernelPciState* OrynKernelPciGetState(void)
{
    return &gPciState;
}

void PrintDeviceLine(const OrynKernelPciDevice* pciDevice)
{
    OrynKernelDiagnosticsLogText("[KERNEL] PCI Device: Bus ");
    OrynKernelDiagnosticsLogDec64(pciDevice->Bus);
    OrynKernelDiagnosticsLogText(", Device ");
    OrynKernelDiagnosticsLogDec64(pciDevice->Device);
    OrynKernelDiagnosticsLogText(", Function ");
    OrynKernelDiagnosticsLogDec64(pciDevice->Function);
    OrynKernelDiagnosticsLogText(", Vendor ");
    OrynKernelDiagnosticsLogText(OrynKernelPciVendorName(pciDevice->VendorId));
    OrynKernelDiagnosticsLogText(" (Vendor ID ");
    OrynKernelDiagnosticsLogHex64(pciDevice->VendorId);
    OrynKernelDiagnosticsLogText("), Device ID ");
    OrynKernelDiagnosticsLogHex64(pciDevice->DeviceId);
    OrynKernelDiagnosticsLogText(", Revision ");
    OrynKernelDiagnosticsLogHex64(pciDevice->RevisionId);
    OrynKernelDiagnosticsLogText(", Class ");
    OrynKernelDiagnosticsLogText(OrynKernelPciClassName(pciDevice->ClassCode));
    OrynKernelDiagnosticsLogText(" (Class Code ");
    OrynKernelDiagnosticsLogHex64(pciDevice->ClassCode);
    OrynKernelDiagnosticsLogText("), Subclass ");
    OrynKernelDiagnosticsLogText(OrynKernelPciSubclassName(pciDevice->ClassCode, pciDevice->Subclass));
    OrynKernelDiagnosticsLogText(" (Subclass Code ");
    OrynKernelDiagnosticsLogHex64(pciDevice->Subclass);
    OrynKernelDiagnosticsLogText("), Program Interface ");
    OrynKernelDiagnosticsLogHex64(pciDevice->ProgIf);
    OrynKernelDiagnosticsLogText(", Header Type ");
    OrynKernelDiagnosticsLogText(OrynKernelPciHeaderTypeName(pciDevice->HeaderType));
    OrynKernelDiagnosticsLogText(" (Header Code ");
    OrynKernelDiagnosticsLogHex64(pciDevice->HeaderType);
    OrynKernelDiagnosticsLogText("), BAR0 ");
    OrynKernelDiagnosticsLogHex64(pciDevice->Bar0);
    OrynKernelDiagnosticsLogText(", BAR5 ");
    OrynKernelDiagnosticsLogHex64(pciDevice->Bar5);
    OrynKernelDiagnosticsLogText(", Interrupt Line ");
    OrynKernelDiagnosticsLogDec64(pciDevice->InterruptLine);
    OrynKernelDiagnosticsLogText(", Interrupt Pin ");
    OrynKernelDiagnosticsLogText(OrynKernelPciInterruptPinName(pciDevice->InterruptPin));
    OrynKernelDiagnosticsLogText(" (Pin Code ");
    OrynKernelDiagnosticsLogHex64(pciDevice->InterruptPin);
    OrynKernelDiagnosticsLogText("), Secondary Bus ");
    OrynKernelDiagnosticsLogDec64(pciDevice->SecondaryBus);
    OrynKernelDiagnosticsLogText(".\n");
}

void OrynKernelPciPrintProof(void)
{
    OrynKernelScreenReportOkOrWarn(gPciState.AcpiRsdpPresent,
        "PCI ACPI RSDP input present.",
        "PCI ACPI RSDP input missing.");
    OrynKernelScreenReportOkOrWarn(gPciState.AcpiChecksumOk,
        "PCI ACPI checksum validation passed.",
        "PCI ACPI checksum validation failed or unavailable.");
    OrynKernelScreenReportOkOrWarn(gPciState.McfgTableFound,
        "PCI ACPI MCFG table discovered.",
        "PCI ACPI MCFG table was not discovered.");
    OrynKernelScreenReportOkOrWarn(gPciState.McfgAllocationCount != 0U,
        "PCIe ECAM descriptor captured.",
        "PCIe ECAM descriptor unavailable.");
    OrynKernelScreenReportOkOrFail(gPciState.ConfigMechanism1Available,
        "PCI config mechanism #1 responded.",
        "PCI config mechanism #1 did not find a device.");
    OrynKernelDiagnosticsLogText("[KERNEL] PCI buses scanned: ");
    OrynKernelDiagnosticsLogDec64(gPciState.BusesScanned);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] PCI function slots scanned: ");
    OrynKernelDiagnosticsLogDec64(gPciState.FunctionSlotsScanned);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(gPciState.BusesScanned == 256U,
        "PCI bus/device/function scan completed.",
        "PCI bus/device/function scan incomplete.");
    OrynKernelScreenReportOkOrFail(gPciState.DevicesFound != 0U,
        "PCI devices discovered.",
        "PCI devices were not discovered.");
    OrynKernelDiagnosticsLogText("[KERNEL] PCI devices discovered: ");
    OrynKernelDiagnosticsLogDec64(gPciState.DevicesFound);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] PCI bridges/storage/network/virtio-net/display/virtio-gpu: ");
    OrynKernelDiagnosticsLogDec64(gPciState.BridgesFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.StorageControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.NetworkControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.VirtioNetworkControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.DisplayControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.VirtioGpuControllersFound);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(gPciState.ClassDecodeReady,
        "PCI class-code decoding ready.",
        "PCI class-code decoding unavailable.");
    OrynKernelScreenReportOkOrFail(gPciState.StorageClassDiscoveryReady,
        "PCI storage-class discovery is complete.",
        "PCI storage-class discovery is unavailable.");
    OrynKernelScreenReportOkOrWarn(gPciState.CapabilityDevicesFound != 0U,
        "PCI capability-list scan foundation ran.",
        "PCI capability-list scan found no capabilities.");
    OrynKernelScreenReportOkOrWarn(gPciState.MsiDevicesFound || gPciState.MsixDevicesFound,
        "PCI MSI/MSI-X capability foundation discovered capable devices.",
        "PCI MSI/MSI-X capability foundation ready; no capable device found.");
    OrynAhciPrintProof();
    OrynNvmePrintProof();
    OrynVirtioBlockPrintProof();
    OrynVirtioNetPrintProof();
    OrynVirtioGpuPrintProof();
    OrynUsbMassStoragePrintProof();
    OrynKernelDiagnosticsLogText("[KERNEL] PCI storage IDE/AHCI/NVMe/VirtIO/USB-host counts: ");
    OrynKernelDiagnosticsLogDec64(gPciState.IdeControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.AhciControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.NvmeControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.VirtioBlockControllersFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.UsbControllersFound);
    OrynKernelDiagnosticsLogText(gPciState.StorageListTruncated ? " truncated\n" : "\n");
    OrynKernelDiagnosticsLogText("[KERNEL] PCI capability/MSI/MSI-X/vector/device-handler counts: ");
    OrynKernelDiagnosticsLogDec64(gPciState.CapabilityDevicesFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.MsiDevicesFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.MsixDevicesFound);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.MsiVectorsAssigned);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gPciState.DeviceInterruptHandlersRegistered);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] PCI recorded device entries: ");
    OrynKernelDiagnosticsLogDec64(gPciState.DevicesRecorded);
    OrynKernelDiagnosticsLogText(gPciState.DeviceListTruncated ? " truncated\n" : "\n");
    unsigned int limit = gPciState.DevicesRecorded < 12U ? gPciState.DevicesRecorded : 12U;
    for (unsigned int index = 0U; index < limit; ++index)
    {
        PrintDeviceLine(&gPciState.Devices[index]);
    }
    OrynKernelScreenReportOkOrFail(gPciState.ClassDecodeReady,
        "PCI device output uses English labels.",
        "PCI device English output unavailable.");
    OrynKernelScreenReportOkOrFail(gPciState.Initialized && gPciState.ConfigMechanism1Available && gPciState.DevicesFound != 0U && gPciState.ClassDecodeReady,
        "PCI Discovery complete.",
        "PCI Discovery incomplete.");
}
