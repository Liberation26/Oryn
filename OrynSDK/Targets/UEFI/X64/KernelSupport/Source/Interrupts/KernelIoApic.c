#include "KernelIoApic.h"
#include "KernelBootInfo.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

#define ORYN_ACPI_SIG_RSDT 0x54445352U
#define ORYN_ACPI_SIG_XSDT 0x54445358U
#define ORYN_ACPI_SIG_APIC 0x43495041U
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

typedef struct OrynAcpiMadt
{
    OrynAcpiSdtHeader Header;
    unsigned int LocalApicAddress;
    unsigned int Flags;
} ORYN_PACKED OrynAcpiMadt;

typedef struct OrynAcpiMadtEntryHeader
{
    unsigned char Type;
    unsigned char Length;
} ORYN_PACKED OrynAcpiMadtEntryHeader;

typedef struct OrynAcpiMadtIoApic
{
    OrynAcpiMadtEntryHeader Header;
    unsigned char IoApicId;
    unsigned char Reserved;
    unsigned int IoApicAddress;
    unsigned int GlobalSystemInterruptBase;
} ORYN_PACKED OrynAcpiMadtIoApic;

typedef struct OrynAcpiMadtIso
{
    OrynAcpiMadtEntryHeader Header;
    unsigned char Bus;
    unsigned char SourceIrq;
    unsigned int GlobalSystemInterrupt;
    unsigned short Flags;
} ORYN_PACKED OrynAcpiMadtIso;

static OrynKernelIoApicState gIoApicState;

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gIoApicState;
    for (unsigned int index = 0U; index < sizeof(gIoApicState); ++index)
    {
        bytes[index] = 0U;
    }
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
    if (rsdp == 0 || !SignatureMatches(rsdp->Signature) || ChecksumBytes(rsdp, 20U) != 0U)
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

static OrynAcpiMadt* FindMadt(const OrynAcpiRsdp20* rsdp)
{
    OrynAcpiSdtHeader* root;
    if (rsdp->Revision >= 2U && rsdp->XsdtAddress != 0ULL)
    {
        root = (OrynAcpiSdtHeader*)rsdp->XsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_XSDT)
        {
            return (OrynAcpiMadt*)FindInXsdt(root, ORYN_ACPI_SIG_APIC);
        }
    }
    if (rsdp->RsdtAddress != 0U)
    {
        root = (OrynAcpiSdtHeader*)(unsigned long long)rsdp->RsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_RSDT)
        {
            return (OrynAcpiMadt*)FindInRsdt(root, ORYN_ACPI_SIG_APIC);
        }
    }
    return 0;
}

static void RecordController(const OrynAcpiMadtIoApic* entry)
{
    OrynKernelIoApicController* controller;
    if (gIoApicState.ControllerCount >= ORYN_KERNEL_IOAPIC_MAX_CONTROLLERS)
    {
        gIoApicState.ControllerListTruncated = 1U;
        return;
    }
    controller = &gIoApicState.Controllers[gIoApicState.ControllerCount++];
    controller->Id = entry->IoApicId;
    controller->Address = entry->IoApicAddress;
    controller->GlobalSystemInterruptBase = entry->GlobalSystemInterruptBase;
    controller->RedirectionEntries = 24U;
}

static void RecordOverride(const OrynAcpiMadtIso* entry)
{
    OrynKernelIoApicOverride* override;
    if (gIoApicState.OverrideCount >= ORYN_KERNEL_IOAPIC_MAX_OVERRIDES)
    {
        gIoApicState.OverrideListTruncated = 1U;
        return;
    }
    override = &gIoApicState.Overrides[gIoApicState.OverrideCount++];
    override->Bus = entry->Bus;
    override->SourceIrq = entry->SourceIrq;
    override->GlobalSystemInterrupt = entry->GlobalSystemInterrupt;
    override->Flags = entry->Flags;
}

static unsigned int ResolveGsi(unsigned int irq, unsigned int* flags)
{
    *flags = 0U;
    for (unsigned int index = 0U; index < gIoApicState.OverrideCount; ++index)
    {
        if (gIoApicState.Overrides[index].SourceIrq == irq)
        {
            *flags = gIoApicState.Overrides[index].Flags;
            return gIoApicState.Overrides[index].GlobalSystemInterrupt;
        }
    }
    return irq;
}

static int FindController(unsigned int gsi, unsigned int* controllerIndex, unsigned int* redirectionIndex)
{
    for (unsigned int index = 0U; index < gIoApicState.ControllerCount; ++index)
    {
        unsigned int base = gIoApicState.Controllers[index].GlobalSystemInterruptBase;
        unsigned int limit = base + gIoApicState.Controllers[index].RedirectionEntries;
        if (gsi >= base && gsi < limit)
        {
            *controllerIndex = index;
            *redirectionIndex = gsi - base;
            return 1;
        }
    }
    return 0;
}

static unsigned int IoApicRead(unsigned int address, unsigned int reg)
{
    volatile unsigned int* select = (volatile unsigned int*)(unsigned long long)address;
    volatile unsigned int* window = (volatile unsigned int*)(unsigned long long)(address + 0x10U);
    *select = reg;
    return *window;
}

static void IoApicWrite(unsigned int address, unsigned int reg, unsigned int value)
{
    volatile unsigned int* select = (volatile unsigned int*)(unsigned long long)address;
    volatile unsigned int* window = (volatile unsigned int*)(unsigned long long)(address + 0x10U);
    *select = reg;
    *window = value;
}

static void ProgramRoute(OrynKernelIoApicRoute* route)
{
    OrynKernelIoApicController* controller = &gIoApicState.Controllers[route->ControllerIndex];
    unsigned int lowReg = 0x10U + (route->RedirectionIndex * 2U);
    unsigned int low = (route->Vector & 0xFFU);
    if ((route->Flags & 0x2U) != 0U)
    {
        low |= 0x2000U;
    }
    if ((route->Flags & 0x8U) != 0U)
    {
        low |= 0x8000U;
    }
    if (controller->Address == 0U)
    {
        return;
    }
    IoApicWrite(controller->Address, lowReg + 1U, 0U);
    IoApicWrite(controller->Address, lowReg, low);
    route->Programmed = (IoApicRead(controller->Address, lowReg) & 0xFFU) == route->Vector;
    if (route->Programmed)
    {
        gIoApicState.RoutesProgrammed += 1U;
    }
}

int OrynKernelIoApicInit(const OrynBootInfo* bootInfo)
{
    const OrynAcpiRsdp20* rsdp;
    const OrynAcpiMadt* madt;
    const unsigned char* cursor;
    const unsigned char* end;
    ClearState();
    if (bootInfo == 0 || !KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP) ||
        bootInfo->Rsdp == 0ULL)
    {
        return 0;
    }
    gIoApicState.AcpiRsdpPresent = 1U;
    rsdp = (const OrynAcpiRsdp20*)bootInfo->Rsdp;
    gIoApicState.AcpiChecksumOk = ValidateRsdp(rsdp) ? 1U : 0U;
    if (gIoApicState.AcpiChecksumOk == 0U)
    {
        return 0;
    }
    madt = FindMadt(rsdp);
    if (madt == 0 || !ValidateSdt(&madt->Header))
    {
        return 0;
    }
    gIoApicState.MadtFound = 1U;
    gIoApicState.LocalApicAddress = madt->LocalApicAddress;
    cursor = (const unsigned char*)madt + sizeof(OrynAcpiMadt);
    end = (const unsigned char*)madt + madt->Header.Length;
    while (cursor + sizeof(OrynAcpiMadtEntryHeader) <= end)
    {
        const OrynAcpiMadtEntryHeader* header = (const OrynAcpiMadtEntryHeader*)cursor;
        if (header->Length < sizeof(OrynAcpiMadtEntryHeader) || cursor + header->Length > end)
        {
            break;
        }
        if (header->Type == 1U && header->Length >= sizeof(OrynAcpiMadtIoApic))
        {
            RecordController((const OrynAcpiMadtIoApic*)cursor);
        }
        else if (header->Type == 2U && header->Length >= sizeof(OrynAcpiMadtIso))
        {
            RecordOverride((const OrynAcpiMadtIso*)cursor);
        }
        cursor += header->Length;
    }
    gIoApicState.Initialized = (gIoApicState.ControllerCount != 0U) ? 1U : 0U;
    return gIoApicState.Initialized ? 1 : 0;
}

const OrynKernelIoApicState* OrynKernelIoApicGetState(void)
{
    return &gIoApicState;
}

int OrynKernelIoApicRouteLegacyIrq(unsigned int legacyIrq, unsigned int vector)
{
    OrynKernelIoApicRoute* route;
    unsigned int flags;
    unsigned int gsi;
    unsigned int controllerIndex;
    unsigned int redirectionIndex;
    if (!gIoApicState.Initialized || legacyIrq >= 16U || vector < 32U ||
        gIoApicState.RoutesRecorded >= ORYN_KERNEL_IOAPIC_MAX_ROUTES)
    {
        return 0;
    }
    gsi = ResolveGsi(legacyIrq, &flags);
    if (!FindController(gsi, &controllerIndex, &redirectionIndex))
    {
        return 0;
    }
    route = &gIoApicState.Routes[gIoApicState.RoutesRecorded++];
    route->LegacyIrq = legacyIrq;
    route->GlobalSystemInterrupt = gsi;
    route->Vector = vector;
    route->Flags = flags;
    route->ControllerIndex = controllerIndex;
    route->RedirectionIndex = redirectionIndex;
    ProgramRoute(route);
    return 1;
}

void OrynKernelIoApicRouteLegacySet(unsigned int firstVector)
{
    for (unsigned int irq = 0U; irq < 16U; ++irq)
    {
        (void)OrynKernelIoApicRouteLegacyIrq(irq, firstVector + irq);
    }
}

void OrynKernelIoApicPrintProof(void)
{
    OrynKernelScreenReportOkOrWarn(gIoApicState.AcpiRsdpPresent,
        "IOAPIC ACPI RSDP input present.", "IOAPIC ACPI RSDP input missing.");
    OrynKernelScreenReportOkOrWarn(gIoApicState.AcpiChecksumOk,
        "IOAPIC ACPI checksum validation passed.", "IOAPIC ACPI checksum failed.");
    OrynKernelScreenReportOkOrWarn(gIoApicState.MadtFound,
        "IOAPIC MADT table discovered.", "IOAPIC MADT table not discovered.");
    OrynKernelScreenReportOkOrWarn(gIoApicState.ControllerCount != 0U,
        "IOAPIC controllers discovered.", "No IOAPIC controllers discovered.");
    OrynKernelDiagnosticsLogText("[KERNEL] IOAPIC controllers/overrides/routes/programmed: ");
    OrynKernelDiagnosticsLogDec64(gIoApicState.ControllerCount);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gIoApicState.OverrideCount);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gIoApicState.RoutesRecorded);
    OrynKernelDiagnosticsLogText("/");
    OrynKernelDiagnosticsLogDec64(gIoApicState.RoutesProgrammed);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrWarn(gIoApicState.RoutesRecorded != 0U,
        "IOAPIC legacy IRQ routing table prepared.", "IOAPIC legacy IRQ routing not prepared.");
}
