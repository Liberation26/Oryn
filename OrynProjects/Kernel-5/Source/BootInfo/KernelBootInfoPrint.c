#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "OrynBootInfoSelection.h"

static void WriteBootInfoField(const char* label, unsigned long long value)
{
    KernelIoWriteString(label);
    KernelIoWriteHex64(value);
    KernelIoWriteString("\n");
}

static void PrintSelectionLine(const char* label, int enabled)
{
    KernelIoWriteString(label);
    KernelIoWriteString(enabled ? "enabled" : "disabled");
    KernelIoWriteString("\n");
}

static void PrintSignedInt(int value)
{
    if (value < 0)
    {
        KernelIoWriteString("-");
        KernelIoWriteDec64((unsigned long long)(0 - value));
        return;
    }

    KernelIoWriteDec64((unsigned long long)value);
}

static void PrintFirmwareData(const OrynBootInfo* bootInfo)
{
    const OrynBootFirmwareData* firmware = &bootInfo->FirmwareData;

    KernelIoWriteString("[KERNEL] Firmware vendor: ");
    KernelIoWriteString(firmware->FirmwareVendor[0] != 0 ? firmware->FirmwareVendor : "unknown");
    KernelIoWriteString("\n");
    WriteBootInfoField("[KERNEL] UEFI revision: ", firmware->UefiRevision);
    WriteBootInfoField("[KERNEL] Firmware revision: ", firmware->FirmwareRevision);
    KernelIoWriteString("[KERNEL] Firmware config tables reported: ");
    KernelIoWriteDec64(firmware->ConfigurationTableCount);
    KernelIoWriteString("\n");

    if (firmware->BootTimeValid)
    {
        KernelIoWriteString("[KERNEL] Firmware boot time UTC/local as reported by UEFI: ");
        KernelIoWriteDec64(firmware->BootTimeYear);
        KernelIoWriteString("-");
        KernelIoWriteDec64(firmware->BootTimeMonth);
        KernelIoWriteString("-");
        KernelIoWriteDec64(firmware->BootTimeDay);
        KernelIoWriteString(" ");
        KernelIoWriteDec64(firmware->BootTimeHour);
        KernelIoWriteString(":");
        KernelIoWriteDec64(firmware->BootTimeMinute);
        KernelIoWriteString(":");
        KernelIoWriteDec64(firmware->BootTimeSecond);
        KernelIoWriteString(" tz ");
        PrintSignedInt(firmware->BootTimeTimeZone);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] Firmware boot time: not supplied by RuntimeServices.GetTime.\n");
    }
}

static void PrintPlatformTables(const OrynBootInfo* bootInfo)
{
    KernelIoWriteString("[KERNEL] Configuration table entries copied: ");
    KernelIoWriteDec64(bootInfo->ConfigurationTableEntryCount);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] Configuration table entry size: ");
    KernelIoWriteDec64(bootInfo->ConfigurationTableEntrySize);
    KernelIoWriteString("\n");
    WriteBootInfoField("[KERNEL] ACPI 1.0 RSDP: ", bootInfo->Acpi10Rsdp);
    WriteBootInfoField("[KERNEL] ACPI 2.0+ RSDP: ", bootInfo->Acpi20Rsdp);
    WriteBootInfoField("[KERNEL] SMBIOS 32-bit entry point: ", bootInfo->Smbios32);
    WriteBootInfoField("[KERNEL] SMBIOS 64-bit entry point: ", bootInfo->Smbios64);
    WriteBootInfoField("[KERNEL] Flattened Device Tree: ", bootInfo->FlattenedDeviceTree);
}

static void PrintFlagValue(const char* label, unsigned int flags, unsigned int flag, unsigned int value)
{
    KernelIoWriteString(label);
    if ((flags & flag) == 0U)
    {
        KernelIoWriteString("not present\n");
        return;
    }

    KernelIoWriteDec64(value);
    KernelIoWriteString("\n");
}

static void PrintNvramSnapshot(const OrynBootInfo* bootInfo)
{
    const OrynBootNvramSnapshot* nvram = &bootInfo->Nvram;
    WriteBootInfoField("[KERNEL] NVRAM flags: ", nvram->Flags);
    PrintFlagValue("[KERNEL] SecureBoot: ", nvram->Flags,
        ORYN_BOOT_NVRAM_FLAG_SECURE_BOOT_PRESENT, nvram->SecureBoot);
    PrintFlagValue("[KERNEL] SetupMode: ", nvram->Flags,
        ORYN_BOOT_NVRAM_FLAG_SETUP_MODE_PRESENT, nvram->SetupMode);
    PrintFlagValue("[KERNEL] AuditMode: ", nvram->Flags,
        ORYN_BOOT_NVRAM_FLAG_AUDIT_MODE_PRESENT, nvram->AuditMode);
    PrintFlagValue("[KERNEL] DeployedMode: ", nvram->Flags,
        ORYN_BOOT_NVRAM_FLAG_DEPLOYED_MODE_PRESENT, nvram->DeployedMode);

    KernelIoWriteString("[KERNEL] BootOrder entries: ");
    KernelIoWriteDec64(nvram->BootOrderCount);
    KernelIoWriteString("\n");
    if (nvram->BootOrderCount > 0U)
    {
        KernelIoWriteString("[KERNEL] BootOrder first entry: ");
        KernelIoWriteHex64(nvram->BootOrder[0]);
        KernelIoWriteString("\n");
    }

    WriteBootInfoField("[KERNEL] Secure Boot PK size: ", nvram->PkSize);
    WriteBootInfoField("[KERNEL] Secure Boot KEK size: ", nvram->KekSize);
    WriteBootInfoField("[KERNEL] Secure Boot db size: ", nvram->DbSize);
    WriteBootInfoField("[KERNEL] Secure Boot dbx size: ", nvram->DbxSize);
}

static void PrintRuntimeServices(const OrynBootInfo* bootInfo)
{
    const OrynBootRuntimeServices* runtime = &bootInfo->RuntimeServices;
    WriteBootInfoField("[KERNEL] RuntimeServices flags: ", runtime->Flags);
    WriteBootInfoField("[KERNEL] UEFI SystemTable pointer: ", runtime->SystemTable);
    WriteBootInfoField("[KERNEL] RuntimeServices table pointer: ", runtime->RuntimeServicesTable);
    WriteBootInfoField("[KERNEL] Runtime GetTime pointer: ", runtime->GetTime);
    WriteBootInfoField("[KERNEL] Runtime SetTime pointer: ", runtime->SetTime);
    WriteBootInfoField("[KERNEL] Runtime GetVariable pointer: ", runtime->GetVariable);
    WriteBootInfoField("[KERNEL] Runtime SetVariable pointer: ", runtime->SetVariable);
    WriteBootInfoField("[KERNEL] Runtime ResetSystem pointer: ", runtime->ResetSystem);
    WriteBootInfoField("[KERNEL] Runtime SetVirtualAddressMap pointer: ", runtime->SetVirtualAddressMap);
    WriteBootInfoField("[KERNEL] Runtime ConvertPointer pointer: ", runtime->ConvertPointer);
    WriteBootInfoField("[KERNEL] Runtime QueryVariableInfo pointer: ", runtime->QueryVariableInfo);
}

static int BootInfoHasUsableScreenFields(const OrynBootInfo* bootInfo)
{
    if (bootInfo->Framebuffer.Base == 0ULL || bootInfo->Framebuffer.Size == 0ULL ||
        bootInfo->Framebuffer.Width == 0U || bootInfo->Framebuffer.Height == 0U ||
        bootInfo->Framebuffer.PixelsPerScanLine < bootInfo->Framebuffer.Width ||
        bootInfo->Framebuffer.BytesPerPixel == 0U)
    {
        return 0;
    }

    return 1;
}

static void PrintScreenFields(const char* label, const OrynBootInfo* bootInfo)
{
    KernelIoWriteString(label);
    KernelIoWriteDec64(bootInfo->Framebuffer.Width);
    KernelIoWriteString("x");
    KernelIoWriteDec64(bootInfo->Framebuffer.Height);
    KernelIoWriteString(" pitch ");
    KernelIoWriteDec64(bootInfo->Framebuffer.PixelsPerScanLine);
    KernelIoWriteString("\n");
    WriteBootInfoField("[KERNEL] Screen base: ", bootInfo->Framebuffer.Base);
    WriteBootInfoField("[KERNEL] Screen size: ", bootInfo->Framebuffer.Size);
    KernelIoWriteString("[KERNEL] Screen bytes per pixel: ");
    KernelIoWriteDec64(bootInfo->Framebuffer.BytesPerPixel);
    KernelIoWriteString("\n");
    WriteBootInfoField("[KERNEL] Pixel format: ", bootInfo->Framebuffer.PixelFormat);
    WriteBootInfoField("[KERNEL] Pixel red mask: ", bootInfo->Framebuffer.RedMask);
    WriteBootInfoField("[KERNEL] Pixel green mask: ", bootInfo->Framebuffer.GreenMask);
    WriteBootInfoField("[KERNEL] Pixel blue mask: ", bootInfo->Framebuffer.BlueMask);
    WriteBootInfoField("[KERNEL] Pixel reserved mask: ", bootInfo->Framebuffer.ReservedMask);
}

void KernelBootInfoPrintSelection(void)
{
    KernelIoWriteString("[KERNEL] BootInfo selection: ");
    KernelIoWriteString(ORYN_BOOTINFO_SELECTION_NAME);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] BootInfo selection mask: ");
    KernelIoWriteHex64((unsigned long long)ORYN_BOOTINFO_SELECTION_MASK);
    KernelIoWriteString("\n");
    PrintSelectionLine("[KERNEL] Selection kernel range: ", ORYN_BOOTINFO_WANT_KERNEL_RANGE);
    PrintSelectionLine("[KERNEL] Selection memory map: ", ORYN_BOOTINFO_WANT_MEMORY_MAP);
    PrintSelectionLine("[KERNEL] Selection framebuffer: ", ORYN_BOOTINFO_WANT_FRAMEBUFFER);
    PrintSelectionLine("[KERNEL] Selection RSDP: ", ORYN_BOOTINFO_WANT_RSDP);
    PrintSelectionLine("[KERNEL] Selection firmware data: ", ORYN_BOOTINFO_WANT_FIRMWARE_DATA);
    PrintSelectionLine("[KERNEL] Selection platform tables: ", ORYN_BOOTINFO_WANT_PLATFORM_TABLES);
    PrintSelectionLine("[KERNEL] Selection NVRAM snapshot: ", ORYN_BOOTINFO_WANT_NVRAM);
    PrintSelectionLine("[KERNEL] Selection RuntimeServices: ", ORYN_BOOTINFO_WANT_RUNTIME_SERVICES);
}

void KernelBootInfoPrintSummary(const OrynBootInfo* bootInfo)
{
    WriteBootInfoField("[KERNEL] BootInfo flags: ", bootInfo->Flags);

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
    {
        WriteBootInfoField("[KERNEL] Kernel physical base: ", bootInfo->KernelPhysicalBase);
        WriteBootInfoField("[KERNEL] Kernel size: ", bootInfo->KernelSize);
    }
    else
    {
        KernelIoWriteString("[KERNEL] Kernel physical range: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP))
    {
        KernelIoWriteString("[KERNEL] Memory map entries: ");
        KernelIoWriteDec64(bootInfo->MemoryMapEntryCount);
        KernelIoWriteString("\n");
        WriteBootInfoField("[KERNEL] Memory map pointer: ", bootInfo->MemoryMap);
        KernelIoWriteString("[KERNEL] Memory map entry size: ");
        KernelIoWriteDec64(bootInfo->MemoryMapEntrySize);
        KernelIoWriteString("\n");
        KernelIoWriteString("[KERNEL] Memory map version: ");
        KernelIoWriteDec64(bootInfo->MemoryMapVersion);
        KernelIoWriteString("\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] Memory map selected: no\n");
        KernelIoWriteString("[KERNEL] Physical allocator unavailable.\n");
        KernelIoWriteString("[KERNEL] Continuing without memory services.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER))
    {
        PrintScreenFields("[KERNEL] Framebuffer: ", bootInfo);
    }
    else if (BootInfoHasUsableScreenFields(bootInfo))
    {
        KernelIoWriteString("[KERNEL] Framebuffer: not selected.\n");
        PrintScreenFields("[KERNEL] Default screen framebuffer: ", bootInfo);
    }
    else
    {
        KernelIoWriteString("[KERNEL] Framebuffer: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FONT))
    {
        KernelIoWriteString("[KERNEL] TTF font: ");
        KernelIoWriteString(bootInfo->FontName);
        KernelIoWriteString("\n");
        WriteBootInfoField("[KERNEL] TTF font base: ", bootInfo->FontBase);
        WriteBootInfoField("[KERNEL] TTF font size: ", bootInfo->FontSize);
    }
    else
    {
        KernelIoWriteString("[KERNEL] TTF font: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP))
    {
        WriteBootInfoField("[KERNEL] RSDP: ", bootInfo->Rsdp);
    }
    else
    {
        KernelIoWriteString("[KERNEL] RSDP: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES))
    {
        PrintPlatformTables(bootInfo);
    }
    else
    {
        KernelIoWriteString("[KERNEL] Platform configuration tables: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FIRMWARE_DATA))
    {
        PrintFirmwareData(bootInfo);
    }
    else
    {
        KernelIoWriteString("[KERNEL] Firmware data: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT))
    {
        PrintNvramSnapshot(bootInfo);
    }
    else
    {
        KernelIoWriteString("[KERNEL] NVRAM snapshot: not supplied.\n");
    }

    if (KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES))
    {
        PrintRuntimeServices(bootInfo);
    }
    else
    {
        KernelIoWriteString("[KERNEL] RuntimeServices: not supplied.\n");
    }
}
