#include "KernelHpet.h"
#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

#define ORYN_ACPI_SIG_RSDT 0x54445352U
#define ORYN_ACPI_SIG_XSDT 0x54445358U
#define ORYN_ACPI_SIG_HPET 0x54455048U
#define ORYN_HPET_GENERAL_CAPABILITIES 0x000U
#define ORYN_HPET_GENERAL_CONFIGURATION 0x010U
#define ORYN_HPET_MAIN_COUNTER 0x0F0U
#define ORYN_HPET_ENABLE_CNF 0x1ULL
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

typedef struct OrynAcpiGenericAddress
{
    unsigned char AddressSpaceId;
    unsigned char RegisterBitWidth;
    unsigned char RegisterBitOffset;
    unsigned char AccessSize;
    unsigned long long Address;
} ORYN_PACKED OrynAcpiGenericAddress;

typedef struct OrynAcpiHpetTable
{
    OrynAcpiSdtHeader Header;
    unsigned int EventTimerBlockId;
    OrynAcpiGenericAddress BaseAddress;
    unsigned char HpetNumber;
    unsigned short MinimumTick;
    unsigned char PageProtection;
} ORYN_PACKED OrynAcpiHpetTable;

static OrynKernelHpetState gHpetState;

static void BusyDelay(void)
{
    for (volatile unsigned int index = 0U; index < 200000U; ++index)
    {
    }
}

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gHpetState;
    for (unsigned int index = 0U; index < sizeof(gHpetState); ++index)
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

static OrynAcpiHpetTable* FindHpetTable(const OrynAcpiRsdp20* rsdp)
{
    OrynAcpiSdtHeader* root;
    if (rsdp->Revision >= 2U && rsdp->XsdtAddress != 0ULL)
    {
        root = (OrynAcpiSdtHeader*)rsdp->XsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_XSDT)
        {
            return (OrynAcpiHpetTable*)FindInXsdt(root, ORYN_ACPI_SIG_HPET);
        }
    }

    if (rsdp->RsdtAddress != 0U)
    {
        root = (OrynAcpiSdtHeader*)(unsigned long long)rsdp->RsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_RSDT)
        {
            return (OrynAcpiHpetTable*)FindInRsdt(root, ORYN_ACPI_SIG_HPET);
        }
    }

    return 0;
}

static volatile unsigned long long* HpetRegister(unsigned long long offset)
{
    return (volatile unsigned long long*)(gHpetState.BaseAddress + offset);
}

static unsigned long long HpetRead(unsigned long long offset)
{
    return *HpetRegister(offset);
}

static void HpetWrite(unsigned long long offset, unsigned long long value)
{
    *HpetRegister(offset) = value;
}

static void EnableAndProbeCounter(void)
{
    gHpetState.Capabilities = HpetRead(ORYN_HPET_GENERAL_CAPABILITIES);
    gHpetState.Configuration = HpetRead(ORYN_HPET_GENERAL_CONFIGURATION);
    HpetWrite(ORYN_HPET_GENERAL_CONFIGURATION,
        gHpetState.Configuration | ORYN_HPET_ENABLE_CNF);
    gHpetState.Configuration = HpetRead(ORYN_HPET_GENERAL_CONFIGURATION);
    gHpetState.Enabled =
        ((gHpetState.Configuration & ORYN_HPET_ENABLE_CNF) != 0ULL) ? 1U : 0U;
    gHpetState.TimerCount = (unsigned int)(((gHpetState.Capabilities >> 8) & 0x1FULL) + 1ULL);
    gHpetState.Counter64Bit =
        ((gHpetState.Capabilities & (1ULL << 13)) != 0ULL) ? 1U : 0U;
    gHpetState.LegacyReplacementRoute =
        ((gHpetState.Capabilities & (1ULL << 15)) != 0ULL) ? 1U : 0U;
    gHpetState.CounterPeriodFemtoSeconds =
        (gHpetState.Capabilities >> 32) & 0xFFFFFFFFULL;
    gHpetState.CounterBefore = HpetRead(ORYN_HPET_MAIN_COUNTER);
    BusyDelay();
    gHpetState.CounterAfter = HpetRead(ORYN_HPET_MAIN_COUNTER);
    gHpetState.CounterAdvanced =
        (gHpetState.CounterAfter != gHpetState.CounterBefore) ? 1U : 0U;
}

int OrynKernelHpetInit(const OrynBootInfo* bootInfo)
{
    const OrynAcpiRsdp20* rsdp;
    OrynAcpiHpetTable* hpet;
    ClearState();
    if (bootInfo == 0 || !KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP) ||
        bootInfo->Rsdp == 0ULL)
    {
        return 0;
    }

    gHpetState.RsdpPresent = 1U;
    rsdp = (const OrynAcpiRsdp20*)bootInfo->Rsdp;
    gHpetState.AcpiChecksumOk = ValidateRsdp(rsdp) ? 1U : 0U;
    if (gHpetState.AcpiChecksumOk == 0U)
    {
        return 0;
    }

    hpet = FindHpetTable(rsdp);
    if (hpet == 0 || hpet->BaseAddress.Address == 0ULL)
    {
        return 0;
    }

    gHpetState.HpetTableFound = 1U;
    gHpetState.HpetTable = (unsigned long long)hpet;
    gHpetState.BaseAddress = hpet->BaseAddress.Address;
    gHpetState.HpetNumber = hpet->HpetNumber;
    gHpetState.MinimumTick = hpet->MinimumTick;
    EnableAndProbeCounter();
    gHpetState.Initialized = gHpetState.Enabled;
    return gHpetState.Initialized ? 1 : 0;
}

const OrynKernelHpetState* OrynKernelHpetGetState(void)
{
    return &gHpetState;
}

unsigned long long OrynKernelHpetReadCounter(void)
{
    if (gHpetState.Initialized == 0U || gHpetState.BaseAddress == 0ULL)
    {
        return 0ULL;
    }

    return HpetRead(ORYN_HPET_MAIN_COUNTER);
}

void OrynKernelHpetPrintProof(void)
{
    OrynKernelScreenReportOkOrWarn(gHpetState.RsdpPresent,
        "HPET ACPI RSDP input present.",
        "HPET ACPI RSDP input missing.");
    OrynKernelScreenReportOkOrWarn(gHpetState.AcpiChecksumOk,
        "HPET ACPI checksum validation passed.",
        "HPET ACPI checksum validation failed or was unavailable.");
    OrynKernelScreenReportOkOrWarn(gHpetState.HpetTableFound,
        "HPET ACPI table discovered.",
        "HPET ACPI table was not discovered.");
    OrynKernelScreenReportOkOrWarn(gHpetState.Enabled,
        "HPET main counter enabled.",
        "HPET main counter not enabled.");
    KernelIoWriteString("[KERNEL] HPET MMIO base: ");
    KernelIoWriteHex64(gHpetState.BaseAddress);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] HPET timers/period(fs): ");
    KernelIoWriteHex64(gHpetState.TimerCount);
    KernelIoWriteString(" / ");
    KernelIoWriteDec64(gHpetState.CounterPeriodFemtoSeconds);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrWarn(gHpetState.CounterAdvanced,
        "HPET counter advanced in probe.",
        "HPET counter did not advance in probe.");
}
