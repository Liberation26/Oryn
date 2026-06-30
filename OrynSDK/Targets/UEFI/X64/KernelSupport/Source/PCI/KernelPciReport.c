#include "KernelPciInternal.h"
#include "KernelScreenReport.h"
void RecordDevice(const OrynKernelPciDevice* pciDevice)
{
    if (gPciState.DevicesRecorded < ORYN_PCI_MAX_RECORDED_DEVICES)
    {
        unsigned char* destination = (unsigned char*)&gPciState.Devices[gPciState.DevicesRecorded];
        const unsigned char* source = (const unsigned char*)pciDevice;
        for (unsigned int index = 0U; index < sizeof(*pciDevice); ++index)
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

void ReadDevice(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    OrynKernelPciDevice* outDevice)
{
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
    outDevice->InterruptLine = PciRead8(bus, device, function, 0x3CU);
    outDevice->InterruptPin = PciRead8(bus, device, function, 0x3DU);
    outDevice->SecondaryBus = PciRead8(bus, device, function, 0x19U);
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
    KernelIoWriteString("[KERNEL] PCI: discovery starting.\n");
    ClearState();
    DiscoverMcfg(bootInfo);
    ScanConfigSpace();
    gPciState.Initialized = 1U;
}

const OrynKernelPciState* OrynKernelPciGetState(void)
{
    return &gPciState;
}

void PrintDeviceLine(const OrynKernelPciDevice* pciDevice)
{
    KernelIoWriteString("[KERNEL] PCI Device: Bus ");
    KernelIoWriteDec64(pciDevice->Bus);
    KernelIoWriteString(", Device ");
    KernelIoWriteDec64(pciDevice->Device);
    KernelIoWriteString(", Function ");
    KernelIoWriteDec64(pciDevice->Function);
    KernelIoWriteString(", Vendor ");
    KernelIoWriteString(OrynKernelPciVendorName(pciDevice->VendorId));
    KernelIoWriteString(" (Vendor ID ");
    KernelIoWriteHex64(pciDevice->VendorId);
    KernelIoWriteString("), Device ID ");
    KernelIoWriteHex64(pciDevice->DeviceId);
    KernelIoWriteString(", Revision ");
    KernelIoWriteHex64(pciDevice->RevisionId);
    KernelIoWriteString(", Class ");
    KernelIoWriteString(OrynKernelPciClassName(pciDevice->ClassCode));
    KernelIoWriteString(" (Class Code ");
    KernelIoWriteHex64(pciDevice->ClassCode);
    KernelIoWriteString("), Subclass ");
    KernelIoWriteString(OrynKernelPciSubclassName(pciDevice->ClassCode, pciDevice->Subclass));
    KernelIoWriteString(" (Subclass Code ");
    KernelIoWriteHex64(pciDevice->Subclass);
    KernelIoWriteString("), Program Interface ");
    KernelIoWriteHex64(pciDevice->ProgIf);
    KernelIoWriteString(", Header Type ");
    KernelIoWriteString(OrynKernelPciHeaderTypeName(pciDevice->HeaderType));
    KernelIoWriteString(" (Header Code ");
    KernelIoWriteHex64(pciDevice->HeaderType);
    KernelIoWriteString("), BAR0 ");
    KernelIoWriteHex64(pciDevice->Bar0);
    KernelIoWriteString(", Interrupt Line ");
    KernelIoWriteDec64(pciDevice->InterruptLine);
    KernelIoWriteString(", Interrupt Pin ");
    KernelIoWriteString(OrynKernelPciInterruptPinName(pciDevice->InterruptPin));
    KernelIoWriteString(" (Pin Code ");
    KernelIoWriteHex64(pciDevice->InterruptPin);
    KernelIoWriteString("), Secondary Bus ");
    KernelIoWriteDec64(pciDevice->SecondaryBus);
    KernelIoWriteString(".\n");
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
    KernelIoWriteString("[KERNEL] PCI buses scanned: ");
    KernelIoWriteDec64(gPciState.BusesScanned);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] PCI function slots scanned: ");
    KernelIoWriteDec64(gPciState.FunctionSlotsScanned);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrFail(gPciState.BusesScanned == 256U,
        "PCI bus/device/function scan completed.",
        "PCI bus/device/function scan incomplete.");
    OrynKernelScreenReportOkOrFail(gPciState.DevicesFound != 0U,
        "PCI devices discovered.",
        "PCI devices were not discovered.");
    KernelIoWriteString("[KERNEL] PCI devices discovered: ");
    KernelIoWriteDec64(gPciState.DevicesFound);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] PCI bridges/storage/network/display: ");
    KernelIoWriteDec64(gPciState.BridgesFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gPciState.StorageControllersFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gPciState.NetworkControllersFound);
    KernelIoWriteString("/");
    KernelIoWriteDec64(gPciState.DisplayControllersFound);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrFail(gPciState.ClassDecodeReady,
        "PCI class-code decoding ready.",
        "PCI class-code decoding unavailable.");
    KernelIoWriteString("[KERNEL] PCI recorded device entries: ");
    KernelIoWriteDec64(gPciState.DevicesRecorded);
    KernelIoWriteString(gPciState.DeviceListTruncated ? " truncated\n" : "\n");
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
