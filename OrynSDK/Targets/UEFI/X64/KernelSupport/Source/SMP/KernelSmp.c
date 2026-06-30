#include "KernelSmp.h"
#include "KernelApic.h"
#include "KernelBootInfo.h"
#include "KernelIo.h"

#define ORYN_ACPI_SIG_RSDT 0x54445352U
#define ORYN_ACPI_SIG_XSDT 0x54445358U
#define ORYN_ACPI_SIG_APIC 0x43495041U
#define ORYN_PACKED __attribute__((packed))

#define ORYN_MADT_ENTRY_LOCAL_APIC 0U
#define ORYN_MADT_LAPIC_ENABLED 0x1U
#define ORYN_MADT_LAPIC_ONLINE_CAPABLE 0x2U
#define ORYN_SMP_AP_WAIT_LIMIT 50000000U

extern unsigned char OrynSmpTrampolineStart[];
extern unsigned char OrynSmpTrampolineEnd[];
extern unsigned char OrynSmpTrampolineGdt[];
extern unsigned char OrynSmpTrampolineGdtPointer[];
extern unsigned char OrynSmpTrampolineCr3[];
extern unsigned char OrynSmpTrampolineEntry[];
extern unsigned char OrynSmpTrampolineStackTop[];
extern unsigned char OrynSmpTrampolineApicId[];

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

typedef struct OrynAcpiMadtHeader
{
    OrynAcpiSdtHeader Header;
    unsigned int LocalApicAddress;
    unsigned int Flags;
} ORYN_PACKED OrynAcpiMadtHeader;

typedef struct OrynAcpiMadtEntryHeader
{
    unsigned char Type;
    unsigned char Length;
} ORYN_PACKED OrynAcpiMadtEntryHeader;

typedef struct OrynAcpiMadtLocalApic
{
    unsigned char Type;
    unsigned char Length;
    unsigned char AcpiProcessorUid;
    unsigned char ApicId;
    unsigned int Flags;
} ORYN_PACKED OrynAcpiMadtLocalApic;

static volatile OrynKernelSmpState gSmpState;
static unsigned char gSmpStacks[ORYN_SMP_MAX_CPUS][ORYN_SMP_STACK_SIZE] __attribute__((aligned(16)));

;

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gSmpState;
    for (unsigned int index = 0U; index < sizeof(gSmpState); ++index)
    {
        bytes[index] = 0U;
    }
}

static void CopyBytes(void* destination, const void* source, unsigned int count)
{
    unsigned char* out = (unsigned char*)destination;
    const unsigned char* in = (const unsigned char*)source;
    for (unsigned int index = 0U; index < count; ++index)
    {
        out[index] = in[index];
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

static OrynAcpiMadtHeader* FindMadtTable(const OrynAcpiRsdp20* rsdp)
{
    OrynAcpiSdtHeader* root;
    if (rsdp->Revision >= 2U && rsdp->XsdtAddress != 0ULL)
    {
        root = (OrynAcpiSdtHeader*)rsdp->XsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_XSDT)
        {
            return (OrynAcpiMadtHeader*)FindInXsdt(root, ORYN_ACPI_SIG_APIC);
        }
    }

    if (rsdp->RsdtAddress != 0U)
    {
        root = (OrynAcpiSdtHeader*)(unsigned long long)rsdp->RsdtAddress;
        if (ValidateSdt(root) && root->Signature == ORYN_ACPI_SIG_RSDT)
        {
            return (OrynAcpiMadtHeader*)FindInRsdt(root, ORYN_ACPI_SIG_APIC);
        }
    }

    return 0;
}

static void AddCpu(unsigned int processorUid, unsigned int apicId, unsigned int enabled)
{
    if (gSmpState.LocalApicEntryCount >= ORYN_SMP_MAX_CPUS)
    {
        return;
    }

    unsigned int index = gSmpState.LocalApicEntryCount;
    gSmpState.Cpus[index].ProcessorUid = processorUid;
    gSmpState.Cpus[index].LocalApicId = apicId;
    gSmpState.Cpus[index].Enabled = enabled;
    gSmpState.Cpus[index].IsBootstrapProcessor =
        (apicId == gSmpState.BootstrapApicId) ? 1U : 0U;
    if (gSmpState.Cpus[index].IsBootstrapProcessor)
    {
        gSmpState.BootstrapCpuIndex = index;
        gSmpState.Cpus[index].Started = 1U;
    }

    gSmpState.LocalApicEntryCount += 1U;
    if (enabled)
    {
        gSmpState.EnabledCpuCount += 1U;
    }
}

static void ParseMadt(const OrynAcpiMadtHeader* madt)
{
    const unsigned char* current = (const unsigned char*)madt + sizeof(OrynAcpiMadtHeader);
    const unsigned char* end = (const unsigned char*)madt + madt->Header.Length;
    gSmpState.MadtFound = 1U;
    gSmpState.LocalApicPhysicalBase = madt->LocalApicAddress;
    gSmpState.MadtLocalApicAddressValid = (madt->LocalApicAddress != 0U) ? 1U : 0U;

    while (current + sizeof(OrynAcpiMadtEntryHeader) <= end)
    {
        const OrynAcpiMadtEntryHeader* entry = (const OrynAcpiMadtEntryHeader*)current;
        if (entry->Length < sizeof(OrynAcpiMadtEntryHeader) || current + entry->Length > end)
        {
            break;
        }

        if (entry->Type == ORYN_MADT_ENTRY_LOCAL_APIC && entry->Length >= sizeof(OrynAcpiMadtLocalApic))
        {
            const OrynAcpiMadtLocalApic* lapic = (const OrynAcpiMadtLocalApic*)current;
            unsigned int enabled =
                ((lapic->Flags & (ORYN_MADT_LAPIC_ENABLED | ORYN_MADT_LAPIC_ONLINE_CAPABLE)) != 0U) ? 1U : 0U;
            AddCpu(lapic->AcpiProcessorUid, lapic->ApicId, enabled);
        }

        current += entry->Length;
    }
}

static void DiscoverCpus(const OrynBootInfo* bootInfo)
{
    const OrynAcpiRsdp20* rsdp;
    OrynAcpiMadtHeader* madt;

    if (bootInfo == 0 || !KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP) || bootInfo->Rsdp == 0ULL)
    {
        return;
    }

    gSmpState.RsdpPresent = 1U;
    rsdp = (const OrynAcpiRsdp20*)bootInfo->Rsdp;
    gSmpState.AcpiChecksumOk = ValidateRsdp(rsdp) ? 1U : 0U;
    if (!gSmpState.AcpiChecksumOk)
    {
        return;
    }

    madt = FindMadtTable(rsdp);
    if (madt != 0)
    {
        ParseMadt(madt);
    }
}

static unsigned long long ReadCr3(void)
{
    unsigned long long value;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}

static void Write64(unsigned long long address, unsigned long long value)
{
    volatile unsigned long long* target = (volatile unsigned long long*)address;
    *target = value;
}

static void Write32(unsigned long long address, unsigned int value)
{
    volatile unsigned int* target = (volatile unsigned int*)address;
    *target = value;
}

static unsigned long long OffsetOf(unsigned char* label)
{
    return (unsigned long long)(label - OrynSmpTrampolineStart);
}

static int PrepareTrampoline(void)
{
    unsigned long long size = (unsigned long long)(OrynSmpTrampolineEnd - OrynSmpTrampolineStart);
    unsigned long long base = ORYN_SMP_TRAMPOLINE_BASE;
    if (size == 0ULL || size > 4096ULL)
    {
        gSmpState.TrampolineSize = (unsigned int)size;
        return 0;
    }

    CopyBytes((void*)base, OrynSmpTrampolineStart, (unsigned int)size);
    unsigned long long gdtBase = base + OffsetOf(OrynSmpTrampolineGdt);
    unsigned long long pointer = base + OffsetOf(OrynSmpTrampolineGdtPointer);
    Write32(pointer + 2ULL, (unsigned int)gdtBase);
    gSmpState.TrampolineBase = base;
    gSmpState.TrampolineSize = (unsigned int)size;
    gSmpState.TrampolinePrepared = 1U;
    return 1;
}

static void PatchTrampolineForCpu(unsigned int cpuIndex)
{
    unsigned long long base = ORYN_SMP_TRAMPOLINE_BASE;
    unsigned long long stackTop =
        (unsigned long long)&gSmpStacks[cpuIndex][ORYN_SMP_STACK_SIZE - 16U];
    Write64(base + OffsetOf(OrynSmpTrampolineCr3), gSmpState.CurrentCr3);
    Write64(base + OffsetOf(OrynSmpTrampolineEntry), (unsigned long long)OrynKernelSmpApEntry);
    Write64(base + OffsetOf(OrynSmpTrampolineStackTop), stackTop);
    Write32(base + OffsetOf(OrynSmpTrampolineApicId), gSmpState.Cpus[cpuIndex].LocalApicId);
}

static void WaitAfterIpi(void)
{
    for (volatile unsigned int delay = 0U; delay < 200000U; ++delay)
    {
    }
}

static unsigned int WaitForCpu(unsigned int cpuIndex, unsigned int before)
{
    for (volatile unsigned int delay = 0U; delay < ORYN_SMP_AP_WAIT_LIMIT; ++delay)
    {
        if (gSmpState.Cpus[cpuIndex].Started || gSmpState.StartedCounter != before)
        {
            return 1U;
        }
    }

    return gSmpState.Cpus[cpuIndex].Started ? 1U : 0U;
}

static void StartApplicationProcessors(void)
{
    if (!gSmpState.TrampolinePrepared || !gSmpState.Cr3Below4G || !gSmpState.IpiPathReady)
    {
        return;
    }

    for (unsigned int index = 0U; index < gSmpState.LocalApicEntryCount; ++index)
    {
        if (!gSmpState.Cpus[index].Enabled || gSmpState.Cpus[index].IsBootstrapProcessor)
        {
            continue;
        }

        unsigned int before = gSmpState.StartedCounter;
        gSmpState.Cpus[index].StartupAttempted = 1U;
        gSmpState.StartupAttemptCount += 1U;
        PatchTrampolineForCpu(index);
        if (OrynKernelApicSendInitIpi(gSmpState.Cpus[index].LocalApicId))
        {
            gSmpState.InitIpiCount += 1U;
        }

        WaitAfterIpi();
        if (OrynKernelApicSendStartupIpi(gSmpState.Cpus[index].LocalApicId, ORYN_SMP_TRAMPOLINE_VECTOR))
        {
            gSmpState.StartupIpiCount += 1U;
        }

        WaitAfterIpi();
        if (!WaitForCpu(index, before) &&
            OrynKernelApicSendStartupIpi(gSmpState.Cpus[index].LocalApicId, ORYN_SMP_TRAMPOLINE_VECTOR))
        {
            gSmpState.StartupIpiCount += 1U;
            (void)WaitForCpu(index, before);
        }

        if (gSmpState.Cpus[index].Started)
        {
            gSmpState.StartupSuccessCount += 1U;
        }
    }
}

void OrynKernelSmpApEntry(unsigned int localApicId)
{
    for (unsigned int index = 0U; index < ORYN_SMP_MAX_CPUS; ++index)
    {
        if (gSmpState.Cpus[index].LocalApicId == localApicId)
        {
            gSmpState.Cpus[index].Started = 1U;
            break;
        }
    }

    gSmpState.StartedCounter += 1U;
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}

int OrynKernelSmpInit(const OrynBootInfo* bootInfo)
{
    ClearState();
    gSmpState.Initialized = 1U;
    gSmpState.BootstrapApicId = OrynKernelApicGetState()->LocalApicId;
    gSmpState.CurrentCr3 = ReadCr3();
    gSmpState.Cr3Below4G = (gSmpState.CurrentCr3 < 0x100000000ULL) ? 1U : 0U;
    gSmpState.IpiPathReady = OrynKernelApicCanSendIpi();
    DiscoverCpus(bootInfo);
    if (gSmpState.LocalApicEntryCount == 0U)
    {
        AddCpu(0U, gSmpState.BootstrapApicId, 1U);
    }

    if (gSmpState.EnabledCpuCount > 0U)
    {
        gSmpState.ApplicationProcessorCount = gSmpState.EnabledCpuCount - 1U;
    }

    if (PrepareTrampoline())
    {
        StartApplicationProcessors();
    }

    return gSmpState.Initialized ? 1 : 0;
}

const OrynKernelSmpState* OrynKernelSmpGetState(void)
{
    return (const OrynKernelSmpState*)&gSmpState;
}

void OrynKernelSmpPrintProof(void)
{
    KernelIoWriteString("[KERNEL] SMP: multi-core processing discovery starting.\n");
    KernelIoWriteString(gSmpState.RsdpPresent ?
        "[KERNEL] PASS: SMP ACPI RSDP input present.\n" :
        "[KERNEL] WARN: SMP ACPI RSDP input missing.\n");
    KernelIoWriteString(gSmpState.AcpiChecksumOk ?
        "[KERNEL] PASS: SMP ACPI checksum validation passed.\n" :
        "[KERNEL] WARN: SMP ACPI checksum validation failed or was unavailable.\n");
    KernelIoWriteString(gSmpState.MadtFound ?
        "[KERNEL] PASS: SMP ACPI MADT table discovered.\n" :
        "[KERNEL] WARN: SMP ACPI MADT table was not discovered.\n");
    KernelIoWriteString("[KERNEL] SMP CPU entries discovered: ");
    KernelIoWriteDec64(gSmpState.LocalApicEntryCount);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] SMP enabled CPU count: ");
    KernelIoWriteDec64(gSmpState.EnabledCpuCount);
    KernelIoWriteString("\n");
    KernelIoWriteString(gSmpState.EnabledCpuCount > 1U ?
        "[KERNEL] PASS: SMP multi-core CPU topology discovered.\n" :
        "[KERNEL] WARN: SMP found only one enabled CPU.\n");
    KernelIoWriteString("[KERNEL] SMP bootstrap APIC ID: ");
    KernelIoWriteHex64(gSmpState.BootstrapApicId);
    KernelIoWriteString("\n");
    KernelIoWriteString(gSmpState.TrampolinePrepared ?
        "[KERNEL] PASS: SMP AP startup trampoline prepared below 1MB.\n" :
        "[KERNEL] FAIL: SMP AP startup trampoline was not prepared.\n");
    KernelIoWriteString(gSmpState.Cr3Below4G ?
        "[KERNEL] PASS: SMP startup CR3 is reachable by the AP trampoline.\n" :
        "[KERNEL] WARN: SMP startup CR3 is above 4GB; AP startup skipped.\n");
    KernelIoWriteString(gSmpState.IpiPathReady ?
        "[KERNEL] PASS: SMP Local APIC IPI path ready.\n" :
        "[KERNEL] FAIL: SMP Local APIC IPI path unavailable.\n");
    KernelIoWriteString("[KERNEL] SMP AP startup attempts/successes: ");
    KernelIoWriteDec64(gSmpState.StartupAttemptCount);
    KernelIoWriteString(" / ");
    KernelIoWriteDec64(gSmpState.StartupSuccessCount);
    KernelIoWriteString("\n");
    KernelIoWriteString(gSmpState.InitIpiCount ?
        "[KERNEL] PASS: SMP INIT IPI sent to application processors.\n" :
        "[KERNEL] WARN: SMP INIT IPI was not sent.\n");
    KernelIoWriteString(gSmpState.StartupIpiCount ?
        "[KERNEL] PASS: SMP STARTUP IPI sent to application processors.\n" :
        "[KERNEL] WARN: SMP STARTUP IPI was not sent.\n");
    KernelIoWriteString((gSmpState.ApplicationProcessorCount == gSmpState.StartupSuccessCount) ?
        "[KERNEL] PASS: SMP application processors entered kernel AP loop.\n" :
        "[KERNEL] WARN: SMP application processor startup is incomplete.\n");
    KernelIoWriteString("[KERNEL] PASS: Multi-Core processing initialized.\n");
}
