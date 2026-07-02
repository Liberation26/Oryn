#include "KernelPciInternal.h"
#include "KernelPci.h"
#include "KernelAhci.h"
#include "KernelUsbMassStorage.h"
#include "KernelVirtioBlock.h"
#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "KernelPortIo.h"

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

OrynKernelPciState gPciState;

void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gPciState;
    for (unsigned int index = 0U; index < sizeof(gPciState); ++index)
    {
        bytes[index] = 0U;
    }
}

unsigned int MakePciAddress(
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

unsigned int PciRead16(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset)
{
    unsigned int value = OrynKernelPciConfigRead32(bus, device, function, offset);
    return (value >> ((offset & 2U) * 8U)) & 0xFFFFU;
}

unsigned int PciRead8(
    unsigned int bus,
    unsigned int device,
    unsigned int function,
    unsigned int offset)
{
    unsigned int value = OrynKernelPciConfigRead32(bus, device, function, offset);
    return (value >> ((offset & 3U) * 8U)) & 0xFFU;
}

unsigned char ChecksumBytes(const void* data, unsigned int size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    unsigned char sum = 0U;
    for (unsigned int index = 0U; index < size; ++index)
    {
        sum = (unsigned char)(sum + bytes[index]);
    }

    return sum;
}

int SignatureMatches(const char* signature)
{
    return signature[0] == 'R' && signature[1] == 'S' && signature[2] == 'D' &&
        signature[3] == ' ' && signature[4] == 'P' && signature[5] == 'T' &&
        signature[6] == 'R' && signature[7] == ' ';
}

int ValidateRsdp(const OrynAcpiRsdp20* rsdp)
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

int ValidateSdt(const OrynAcpiSdtHeader* header)
{
    if (header == 0 || header->Length < sizeof(OrynAcpiSdtHeader))
    {
        return 0;
    }

    return ChecksumBytes(header, header->Length) == 0U;
}

OrynAcpiSdtHeader* FindInXsdt(const OrynAcpiSdtHeader* xsdt, unsigned int signature)
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

OrynAcpiSdtHeader* FindInRsdt(const OrynAcpiSdtHeader* rsdt, unsigned int signature)
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

OrynAcpiMcfgTable* FindMcfgTable(const OrynAcpiRsdp20* rsdp)
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

void DiscoverMcfg(const OrynBootInfo* bootInfo)
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

const char* OrynKernelPciVendorName(unsigned int vendorId)
{
    switch (vendorId)
    {
        case 0x8086U: return "Intel";
        case 0x1234U: return "QEMU";
        case 0x1AF4U: return "Red Hat / VirtIO";
        case 0x1B36U: return "Red Hat / QEMU";
        case 0x10ECU: return "Realtek";
        case 0x1022U: return "AMD";
        case 0x1002U: return "AMD / ATI";
        case 0x10DEU: return "NVIDIA";
        case 0x15ADU: return "VMware";
        case 0x80EEU: return "VirtualBox";
        case 0x1414U: return "Microsoft";
        default: return "Unknown vendor";
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

const char* OrynKernelPciSubclassName(unsigned int classCode, unsigned int subclass)
{
    if (classCode == 0x01U)
    {
        switch (subclass)
        {
            case 0x00U: return "SCSI storage controller";
            case 0x01U: return "IDE storage controller";
            case 0x02U: return "Floppy disk controller";
            case 0x03U: return "IPI storage controller";
            case 0x04U: return "RAID storage controller";
            case 0x05U: return "ATA storage controller";
            case 0x06U: return "SATA storage controller";
            case 0x07U: return "Serial Attached SCSI controller";
            case 0x08U: return "Non-volatile memory controller";
            default: return "Other storage controller";
        }
    }

    if (classCode == 0x02U)
    {
        switch (subclass)
        {
            case 0x00U: return "Ethernet network controller";
            case 0x01U: return "Token ring network controller";
            case 0x02U: return "FDDI network controller";
            case 0x03U: return "ATM network controller";
            case 0x80U: return "Other network controller";
            default: return "Network controller";
        }
    }

    if (classCode == 0x03U)
    {
        switch (subclass)
        {
            case 0x00U: return "VGA compatible display controller";
            case 0x01U: return "XGA display controller";
            case 0x02U: return "3D display controller";
            case 0x80U: return "Other display controller";
            default: return "Display controller";
        }
    }

    if (classCode == 0x06U)
    {
        switch (subclass)
        {
            case 0x00U: return "Host bridge";
            case 0x01U: return "ISA bridge";
            case 0x02U: return "EISA bridge";
            case 0x03U: return "MCA bridge";
            case 0x04U: return "PCI-to-PCI bridge";
            case 0x05U: return "PCMCIA bridge";
            case 0x06U: return "NuBus bridge";
            case 0x07U: return "CardBus bridge";
            default: return "Bridge device";
        }
    }

    if (classCode == 0x0CU)
    {
        switch (subclass)
        {
            case 0x00U: return "FireWire serial bus controller";
            case 0x01U: return "ACCESS.bus controller";
            case 0x02U: return "SSA serial bus controller";
            case 0x03U: return "USB controller";
            case 0x04U: return "Fibre Channel controller";
            case 0x05U: return "SMBus controller";
            default: return "Serial bus controller";
        }
    }

    return "Subclass not decoded yet";
}

const char* OrynKernelPciHeaderTypeName(unsigned int headerType)
{
    switch (headerType & 0x7FU)
    {
        case 0x00U: return "Endpoint device";
        case 0x01U: return "PCI-to-PCI bridge";
        case 0x02U: return "CardBus bridge";
        default: return "Unknown header type";
    }
}

const char* OrynKernelPciInterruptPinName(unsigned int interruptPin)
{
    switch (interruptPin)
    {
        case 0U: return "No interrupt pin";
        case 1U: return "INTA#";
        case 2U: return "INTB#";
        case 3U: return "INTC#";
        case 4U: return "INTD#";
        default: return "Unknown interrupt pin";
    }
}

int OrynKernelPciIsStorageController(const OrynKernelPciDevice* pciDevice)
{
    if (pciDevice == 0)
    {
        return 0;
    }
    return pciDevice->ClassCode == 0x01U ||
        (pciDevice->VendorId == 0x1AF4U && pciDevice->DeviceId >= 0x1001U &&
        pciDevice->DeviceId <= 0x1042U);
}

const OrynKernelPciDevice* OrynKernelPciGetStorageController(unsigned int index)
{
    if (index >= gPciState.StorageControllersRecorded)
    {
        return 0;
    }
    return &gPciState.StorageControllers[index];
}

static void RecordStorageController(const OrynKernelPciDevice* pciDevice)
{
    if (gPciState.StorageControllersRecorded < ORYN_PCI_MAX_STORAGE_CONTROLLERS)
    {
        gPciState.StorageControllers[gPciState.StorageControllersRecorded] = *pciDevice;
        gPciState.StorageControllersRecorded += 1U;
    }
    else
    {
        gPciState.StorageListTruncated = 1U;
    }
}

static void CountStorageKind(const OrynKernelPciDevice* pciDevice)
{
    if (pciDevice->Subclass == 0x01U)
    {
        gPciState.IdeControllersFound += 1U;
    }
    else if (pciDevice->Subclass == 0x06U && pciDevice->ProgIf == 0x01U)
    {
        gPciState.AhciControllersFound += 1U;
    }
    else if (pciDevice->Subclass == 0x08U)
    {
        gPciState.NvmeControllersFound += 1U;
    }
    if (pciDevice->VendorId == 0x1AF4U)
    {
        gPciState.VirtioBlockControllersFound += 1U;
    }
}

void CountClass(const OrynKernelPciDevice* pciDevice)
{
    if (OrynKernelPciIsStorageController(pciDevice))
    {
        gPciState.StorageControllersFound += 1U;
        CountStorageKind(pciDevice);
        RecordStorageController(pciDevice);
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
    else if (pciDevice->ClassCode == 0x0CU && pciDevice->Subclass == 0x03U)
    {
        gPciState.UsbControllersFound += 1U;
    }
}

