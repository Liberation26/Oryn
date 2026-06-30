#include "KernelPci.h"
#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "KernelPortIo.h"

#define ORYN_PCI_CONFIG_ADDRESS 0xCF8U
#define ORYN_PCI_CONFIG_DATA 0xCFCU
#define ORYN_PCI_ENABLE 0x80000000U
#define ORYN_PCI_INVALID_VENDOR 0xFFFFU
#define ORYN_ACPI_SIG_RSDT 0x54445352U
#define ORYN_ACPI_SIG_XSDT 0x54445358U
#define ORYN_ACPI_SIG_MCFG 0x4746434DU
#define ORYN_PACKED __attribute__((packed))

typedef struct OrynAcpiRsdp20
{
    char Signature[8];
    unsigned char Checksum;
    char OemId[6];
    unsigned char Revision;
    unsigned int RsdtAddress;
    unsigned int Length;
    unsigned long long XsdtAddress;
    unsigned char ExtendedChecksum;
    unsigned char Reserved[3];
} ORYN_PACKED OrynAcpiRsdp20;

typedef struct OrynAcpiSdtHeader
{
    unsigned int Signature;
    unsigned int Length;
    unsigned char Revision;
    unsigned char Checksum;
    char OemId[6];
    char OemTableId[8];
    unsigned int OemRevision;
    unsigned int CreatorId;
    unsigned int CreatorRevision;
} ORYN_PACKED OrynAcpiSdtHeader;

typedef struct OrynAcpiMcfgTable
{
    OrynAcpiSdtHeader Header;
    unsigned long long Reserved;
} ORYN_PACKED OrynAcpiMcfgTable;

typedef struct OrynAcpiMcfgAllocation
{
    unsigned long long BaseAddress;
    unsigned short PciSegmentGroup;
    unsigned char StartBus;
    unsigned char EndBus;
    unsigned int Reserved;
} ORYN_PACKED OrynAcpiMcfgAllocation;

static OrynKernelPciState gPciState;

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gPciState;
    for (unsigned int index = 0U; index < sizeof(gPciState); ++index)
    {
        bytes[index] = 0U;
    }
}

static unsigned int MakePciAddress(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset)
{
    return ORYN_PCI_ENABLE |
        ((bus & 0xFFU) << 16) |
        ((device & 0x1FU) << 11) |
        ((function & 0x7U) << 8) |
        (offset & 0xFCU);
}

unsigned int OrynKernelPciConfigRead32(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset)
{
    OrynPortOut32(ORYN_PCI_CONFIG_ADDRESS, MakePciAddress(bus, device, function, offset));
    return OrynPortIn32(ORYN_PCI_CONFIG_DATA);
}

void OrynKernelPciConfigWrite32(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset,
    unsigned int value)
{
    OrynPortOut32(ORYN_PCI_CONFIG_ADDRESS, MakePciAddress(bus, device, function, offset));
    OrynPortOut32(ORYN_PCI_CONFIG_DATA, value);
}

static unsigned int PciRead16(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset)
{
    unsigned int value = OrynKernelPciConfigRead32(bus, device, function, offset);
    return (value >> ((offset & 2U) * 8U)) & 0xFFFFU;
}

static unsigned int PciRead8(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset)
{
    unsigned int value = OrynKernelPciConfigRead32(bus, device, function, offset);
    return (value >> ((offset & 3U) * 8U)) & 0xFFU;
}

static unsigned char ChecksumBytes(const void* data, unsigned int size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    unsigned char sum = 0U;
    for (unsigned int index = 0U; index < size; ++index)
    {
        sum = (unsigned char)(sum + bytes[index]);
    }

    return sum;
}

static int SignatureMatches(const char* signature)
{
    return signature[0] == 'R' && signature[1] == 'S' && signature[2] == 'D' &&
        signature[3] == ' ' && signature[4] == 'P' && signature[5] == 'T' &&
        signature[6] == 'R' && signature[7] == ' ';
}

static int ValidateRsdp(const OrynAcpiRsdp20* rsdp)
{
    if (rsdp == 0 || !SignatureMatches(rsdp->Signature))
    {
        return 0;
    }

    if (ChecksumBytes(rsdp, 20U) != 0U)
    {
        return 0;
    }

    if (rsdp->Revision >= 2U && rsdp->Length >= sizeof(OrynAcpiRsdp20))
    {
        return ChecksumBytes(rsdp, rsdp->Length) == 0U;
    }

    return 1;
}

static int ValidateSdt(const OrynAcpiSdtHeader* header)
{
    if (header == 0 || header->Length < sizeof(OrynAcpiSdtHeader))
    {
        return 0;
    }

    return ChecksumBytes(header, header->Length) == 0U;
}

static OrynAcpiSdtHeader* FindInXsdt(const OrynAcpiSdtHeader* xsdt, unsigned int signature)
{
    unsigned int entries = (xsdt->Length - sizeof(OrynAcpiSdtHeader)) / 8U;
    const unsigned long long* table =
        (const unsigned long long*)((const unsigned char*)xsdt + sizeof(OrynAcpiSdtHeader));
    for (unsigned int index = 0U; index < entries; ++index)
    {
        OrynAcpiSdtHeader* header = (OrynAcpiSdtHeader*)table[index];
        if (ValidateSdt(header) && header->Signature == signature)
        {
            return header;
        }
    }

    return 0;
}

static OrynAcpiSdtHeader* FindInRsdt(const OrynAcpiSdtHeader* rsdt, unsigned int signature)
{
    unsigned int entries = (rsdt->Length - sizeof(OrynAcpiSdtHeader)) / 4U;
    const unsigned int* table =
        (const unsigned int*)((const unsigned char*)rsdt + sizeof(OrynAcpiSdtHeader));
    for (unsigned int index = 0U; index < entries; ++index)
    {
        OrynAcpiSdtHeader* header = (OrynAcpiSdtHeader*)(unsigned long long)table[index];
        if (ValidateSdt(header) && header->Signature == signature)
        {
            return header;
        }
    }

    return 0;
}

static OrynAcpiMcfgTable* FindMcfgTable(const OrynAcpiRsdp20* rsdp)
{
    OrynAcpiSdtHeader* root;
    if (rsdp->Revision >= 2U && rsdp->XsdtAddress != 0ULL)
    {
        root = (OrynAcpiSdtHeader*)rsdp->XsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_XSDT)
        {
            return (OrynAcpiMcfgTable*)FindInXsdt(root, ORYN_ACPI_SIG_MCFG);
        }
    }

    if (rsdp->RsdtAddress != 0U)
    {
        root = (OrynAcpiSdtHeader*)(unsigned long long)rsdp->RsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_RSDT)
        {
            return (OrynAcpiMcfgTable*)FindInRsdt(root, ORYN_ACPI_SIG_MCFG);
        }
    }

    return 0;
}

static void DiscoverMcfg(const OrynBootInfo* bootInfo)
{
    const OrynAcpiRsdp20* rsdp;
    OrynAcpiMcfgTable* mcfg;
    unsigned int bytes;
    const OrynAcpiMcfgAllocation* allocations;

    if (bootInfo == 0 || !KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP) ||
        bootInfo->Rsdp == 0ULL)
    {
        return;
    }

    gPciState.AcpiRsdpPresent = 1U;
    rsdp = (const OrynAcpiRsdp20*)bootInfo->Rsdp;
    gPciState.AcpiChecksumOk = ValidateRsdp(rsdp) ? 1U : 0U;
    if (gPciState.AcpiChecksumOk == 0U)
    {
        return;
    }

    mcfg = FindMcfgTable(rsdp);
    if (mcfg == 0 || mcfg->Header.Length < sizeof(OrynAcpiMcfgTable))
    {
        return;
    }

    bytes = mcfg->Header.Length - sizeof(OrynAcpiMcfgTable);
    gPciState.McfgAllocationCount = bytes / sizeof(OrynAcpiMcfgAllocation);
    gPciState.McfgTableFound = 1U;
    if (gPciState.McfgAllocationCount != 0U)
    {
        allocations = (const OrynAcpiMcfgAllocation*)((const unsigned char*)mcfg +
            sizeof(OrynAcpiMcfgTable));
        gPciState.FirstMcfgBase = allocations[0].BaseAddress;
        gPciState.FirstMcfgSegment = allocations[0].PciSegmentGroup;
        gPciState.FirstMcfgStartBus = allocations[0].StartBus;
        gPciState.FirstMcfgEndBus = allocations[0].EndBus;
    }
}

const char* OrynKernelPciClassName(unsigned int classCode)
{
    switch (classCode)
    {
        case 0x00U: return "Unclassified";
        case 0x01U: return "Mass storage";
        case 0x02U: return "Network";
        case 0x03U: return "Display";
        case 0x04U: return "Multimedia";
        case 0x05U: return "Memory";
        case 0x06U: return "Bridge";
        case 0x07U: return "Communication";
        case 0x08U: return "System peripheral";
        case 0x09U: return "Input";
        case 0x0AU: return "Docking";
        case 0x0BU: return "Processor";
        case 0x0CU: return "Serial bus";
        default: return "Other";
    }
}

static void CountClass(const OrynKernelPciDevice* pciDevice)
{
    if (pciDevice->ClassCode == 0x01U)
    {
        gPciState.StorageControllersFound += 1U;
    }
    else if (pciDevice->ClassCode == 0x02U)
    {
        gPciState.NetworkControllersFound += 1U;
    }
    else if (pciDevice->ClassCode == 0x03U)
    {
        gPciState.DisplayControllersFound += 1U;
    }
    else if (pciDevice->ClassCode == 0x06U)
    {
        gPciState.BridgesFound += 1U;
    }
}

static void RecordDevice(const OrynKernelPciDevice* pciDevice)
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

static void ReadDevice(
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

static void ScanConfigSpace(void)
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

static void PrintDeviceLine(const OrynKernelPciDevice* pciDevice)
{
    KernelIoWriteString("[KERNEL] PCI device: bus ");
    KernelIoWriteHex64(pciDevice->Bus);
    KernelIoWriteString(" dev ");
    KernelIoWriteHex64(pciDevice->Device);
    KernelIoWriteString(" func ");
    KernelIoWriteHex64(pciDevice->Function);
    KernelIoWriteString(" vendor/device ");
    KernelIoWriteHex64(pciDevice->VendorId);
    KernelIoWriteString("/");
    KernelIoWriteHex64(pciDevice->DeviceId);
    KernelIoWriteString(" class ");
    KernelIoWriteString(OrynKernelPciClassName(pciDevice->ClassCode));
    KernelIoWriteString(" ");
    KernelIoWriteHex64(pciDevice->ClassCode);
    KernelIoWriteString(":" );
    KernelIoWriteHex64(pciDevice->Subclass);
    KernelIoWriteString(" progIF ");
    KernelIoWriteHex64(pciDevice->ProgIf);
    KernelIoWriteString(" irq ");
    KernelIoWriteHex64(pciDevice->InterruptLine);
    KernelIoWriteString("/pin ");
    KernelIoWriteHex64(pciDevice->InterruptPin);
    KernelIoWriteString("\n");
}

void OrynKernelPciPrintProof(void)
{
    KernelIoWriteString(gPciState.AcpiRsdpPresent ?
        "[KERNEL] PASS: PCI ACPI RSDP input present.\n" :
        "[KERNEL] WARN: PCI ACPI RSDP input missing.\n");
    KernelIoWriteString(gPciState.AcpiChecksumOk ?
        "[KERNEL] PASS: PCI ACPI checksum validation passed.\n" :
        "[KERNEL] WARN: PCI ACPI checksum validation failed or unavailable.\n");
    KernelIoWriteString(gPciState.McfgTableFound ?
        "[KERNEL] PASS: PCI ACPI MCFG table discovered.\n" :
        "[KERNEL] WARN: PCI ACPI MCFG table was not discovered.\n");
    KernelIoWriteString(gPciState.McfgAllocationCount != 0U ?
        "[KERNEL] PASS: PCIe ECAM descriptor captured.\n" :
        "[KERNEL] WARN: PCIe ECAM descriptor unavailable.\n");
    KernelIoWriteString(gPciState.ConfigMechanism1Available ?
        "[KERNEL] PASS: PCI config mechanism #1 responded.\n" :
        "[KERNEL] FAIL: PCI config mechanism #1 did not find a device.\n");
    KernelIoWriteString("[KERNEL] PCI buses scanned: ");
    KernelIoWriteDec64(gPciState.BusesScanned);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] PCI function slots scanned: ");
    KernelIoWriteDec64(gPciState.FunctionSlotsScanned);
    KernelIoWriteString("\n");
    KernelIoWriteString(gPciState.BusesScanned == 256U ?
        "[KERNEL] PASS: PCI bus/device/function scan completed.\n" :
        "[KERNEL] FAIL: PCI bus/device/function scan incomplete.\n");
    KernelIoWriteString(gPciState.DevicesFound != 0U ?
        "[KERNEL] PASS: PCI devices discovered.\n" :
        "[KERNEL] FAIL: PCI devices were not discovered.\n");
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
    KernelIoWriteString(gPciState.ClassDecodeReady ?
        "[KERNEL] PASS: PCI class-code decoding ready.\n" :
        "[KERNEL] FAIL: PCI class-code decoding unavailable.\n");
    KernelIoWriteString("[KERNEL] PCI recorded device entries: ");
    KernelIoWriteDec64(gPciState.DevicesRecorded);
    KernelIoWriteString(gPciState.DeviceListTruncated ? " truncated\n" : "\n");
    unsigned int limit = gPciState.DevicesRecorded < 12U ? gPciState.DevicesRecorded : 12U;
    for (unsigned int index = 0U; index < limit; ++index)
    {
        PrintDeviceLine(&gPciState.Devices[index]);
    }
    KernelIoWriteString(gPciState.Initialized && gPciState.ConfigMechanism1Available &&
        gPciState.DevicesFound != 0U && gPciState.ClassDecodeReady ?
        "[KERNEL] PASS: PCI Discovery complete.\n" :
        "[KERNEL] FAIL: PCI Discovery incomplete.\n");
}
