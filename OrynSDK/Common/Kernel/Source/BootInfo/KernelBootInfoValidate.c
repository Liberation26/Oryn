#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "OrynBootInfoSelection.h"

static void WriteBootInfoField(const char* label, unsigned long long value)
{
    KernelIoWriteString(label);
    KernelIoWriteHex64(value);
    KernelIoWriteString("\n");
}

static void WriteBootInfoDecimalField(const char* label, unsigned long long value)
{
    KernelIoWriteString(label);
    KernelIoWriteDec64(value);
    KernelIoWriteString("\n");
}

static void WriteValidationFail(const char* message, OrynKernelBootInfoStatus* status)
{
    KernelIoWriteString("[KERNEL] FAIL: ");
    KernelIoWriteString(message);
    KernelIoWriteString("\n");
    status->IsValid = 0;
}

static void CheckDisabledFlag(
    const OrynBootInfo* bootInfo,
    unsigned long long flag,
    int wanted,
    const char* name,
    OrynKernelBootInfoStatus* status)
{
    if (!wanted && KernelBootInfoHasFlag(bootInfo, flag))
    {
        KernelIoWriteString("[KERNEL] FAIL: BootInfo supplied disabled field: ");
        KernelIoWriteString(name);
        KernelIoWriteString("\n");
        status->IsValid = 0;
    }
}

static void CheckExpectedFlag(
    const OrynBootInfo* bootInfo,
    unsigned long long flag,
    int wanted,
    const char* name,
    OrynKernelBootInfoStatus* status)
{
    if (wanted && !KernelBootInfoHasFlag(bootInfo, flag))
    {
        KernelIoWriteString("[KERNEL] WARN: Requested BootInfo item was not supplied: ");
        KernelIoWriteString(name);
        KernelIoWriteString("\n");
        status->WarningCount += 1;
    }
}

static void ValidateKernelRange(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE))
    {
        return;
    }

    if (bootInfo->KernelPhysicalBase == 0ULL)
    {
        WriteValidationFail("Kernel physical base is zero.", status);
    }

    if (bootInfo->KernelSize == 0ULL)
    {
        WriteValidationFail("Kernel size is zero.", status);
    }
}

static void ValidateMemoryMap(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP))
    {
        return;
    }

    if (bootInfo->MemoryMap == 0ULL)
    {
        WriteValidationFail("Memory map pointer is zero.", status);
    }

    if (bootInfo->MemoryMapEntryCount == 0ULL)
    {
        WriteValidationFail("Memory map entry count is zero.", status);
    }

    if (bootInfo->MemoryMapEntrySize < sizeof(OrynBootMemoryEntry))
    {
        WriteValidationFail("Memory map entry size is smaller than OrynBootMemoryEntry.", status);
    }
}

static void ValidateFramebuffer(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER))
    {
        return;
    }

    if (bootInfo->Framebuffer.Base == 0ULL || bootInfo->Framebuffer.Size == 0ULL)
    {
        WriteValidationFail("Framebuffer base or size is zero.", status);
    }

    if (bootInfo->Framebuffer.Width == 0U || bootInfo->Framebuffer.Height == 0U)
    {
        WriteValidationFail("Framebuffer resolution is zero.", status);
    }

    if (bootInfo->Framebuffer.PixelsPerScanLine < bootInfo->Framebuffer.Width)
    {
        WriteValidationFail("Framebuffer pitch is smaller than width.", status);
    }

    if (bootInfo->Framebuffer.BytesPerPixel != 4U)
    {
        WriteValidationFail("Framebuffer is not a 32-bit linear pixel buffer.", status);
    }

    if (bootInfo->Framebuffer.PixelFormat == ORYN_FRAMEBUFFER_PIXEL_FORMAT_UNKNOWN)
    {
        WriteValidationFail("Framebuffer pixel format is unknown.", status);
    }
}

static void ValidateRsdp(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP))
    {
        return;
    }

    if (bootInfo->Rsdp == 0ULL)
    {
        WriteValidationFail("RSDP flag is set but RSDP pointer is zero.", status);
    }
}


static void ValidatePlatformTables(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES))
    {
        return;
    }

    if (bootInfo->ConfigurationTableEntryCount == 0U)
    {
        WriteValidationFail("Configuration table flag is set but no entries were copied.", status);
    }

    if (bootInfo->ConfigurationTableEntryCount > ORYN_BOOTINFO_MAX_CONFIGURATION_TABLES)
    {
        WriteValidationFail("Configuration table count exceeds the BootInfo array capacity.", status);
    }

    if (bootInfo->ConfigurationTableEntrySize < sizeof(OrynBootConfigurationTableEntry))
    {
        WriteValidationFail("Configuration table entry size is too small.", status);
    }
}

static void ValidateNvramSnapshot(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    const OrynBootNvramSnapshot* nvram = &bootInfo->Nvram;

    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT))
    {
        return;
    }

    if (nvram->Version != 1U)
    {
        WriteValidationFail("NVRAM snapshot version is unsupported.", status);
    }

    if (nvram->Size < sizeof(OrynBootNvramSnapshot))
    {
        WriteValidationFail("NVRAM snapshot size is smaller than OrynBootNvramSnapshot.", status);
    }

    if ((nvram->Flags & ORYN_BOOT_NVRAM_FLAG_GET_VARIABLE_AVAILABLE) == 0U)
    {
        WriteValidationFail("NVRAM snapshot is flagged but GetVariable was unavailable.", status);
    }

    if (nvram->BootOrderCount > ORYN_BOOTINFO_MAX_BOOT_ORDER_ENTRIES)
    {
        WriteValidationFail("NVRAM BootOrder count exceeds the BootInfo array capacity.", status);
    }
}

static void ValidateRuntimeServices(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    const OrynBootRuntimeServices* runtime = &bootInfo->RuntimeServices;

    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES))
    {
        return;
    }

    if (runtime->Version != 1U)
    {
        WriteValidationFail("RuntimeServices handoff version is unsupported.", status);
    }

    if (runtime->Size < sizeof(OrynBootRuntimeServices))
    {
        WriteValidationFail("RuntimeServices handoff size is smaller than OrynBootRuntimeServices.", status);
    }

    if (runtime->RuntimeServicesTable == 0ULL)
    {
        WriteValidationFail("RuntimeServices table pointer is zero.", status);
    }

    if (runtime->Flags == 0ULL)
    {
        WriteValidationFail("RuntimeServices handoff has no callable service pointers.", status);
    }
}

static void ValidateFirmwareData(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    const OrynBootFirmwareData* firmware = &bootInfo->FirmwareData;

    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FIRMWARE_DATA))
    {
        return;
    }

    if (firmware->Version != 1U)
    {
        WriteValidationFail("Firmware data version is unsupported.", status);
    }

    if (firmware->Size < sizeof(OrynBootFirmwareData))
    {
        WriteValidationFail("Firmware data size is smaller than OrynBootFirmwareData.", status);
    }

    if (firmware->UefiRevision == 0U)
    {
        WriteValidationFail("Firmware data has no UEFI revision.", status);
    }
}

static void ValidateFont(const OrynBootInfo* bootInfo, OrynKernelBootInfoStatus* status)
{
    if (!KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FONT))
    {
        return;
    }

    if (bootInfo->FontBase == 0ULL || bootInfo->FontSize == 0ULL)
    {
        WriteValidationFail("Font flag is set but font base or size is zero.", status);
    }
}

OrynKernelBootInfoStatus KernelBootInfoValidate(const OrynBootInfo* bootInfo)
{
    OrynKernelBootInfoStatus status;
    status.IsValid = 1;
    status.WarningCount = 0;

    if (bootInfo == 0)
    {
        WriteValidationFail("BootInfo pointer is null.", &status);
        return status;
    }

    WriteBootInfoField("[KERNEL] BootInfo pointer: ", (unsigned long long)bootInfo);

    if (bootInfo->Signature != ORYN_BOOTINFO_SIGNATURE)
    {
        WriteValidationFail("BootInfo signature is invalid.", &status);
        WriteBootInfoField("[KERNEL] BootInfo signature: ", bootInfo->Signature);
        return status;
    }

    WriteBootInfoDecimalField("[KERNEL] BootInfo ABI major: ", ORYN_BOOTINFO_ABI_MAJOR);
    WriteBootInfoDecimalField("[KERNEL] BootInfo ABI minor: ", ORYN_BOOTINFO_ABI_MINOR);
    WriteBootInfoDecimalField("[KERNEL] BootInfo ABI version: ", bootInfo->Version);
    WriteBootInfoDecimalField("[KERNEL] BootInfo ABI size: ", bootInfo->Size);

    if (!OrynBootInfoAbiIsVersionCompatible(bootInfo->Version))
    {
        WriteValidationFail("BootInfo ABI version is unsupported.", &status);
        WriteBootInfoDecimalField("[KERNEL] Minimum compatible BootInfo ABI version: ", ORYN_BOOTINFO_ABI_MIN_COMPATIBLE_VERSION);
        WriteBootInfoDecimalField("[KERNEL] Maximum compatible BootInfo ABI version: ", ORYN_BOOTINFO_ABI_CURRENT_VERSION);
        return status;
    }

    if (!OrynBootInfoAbiIsSizeCompatible(bootInfo->Size))
    {
        WriteValidationFail("BootInfo ABI size is smaller than the stable contract.", &status);
        WriteBootInfoDecimalField("[KERNEL] Minimum compatible BootInfo ABI size: ", ORYN_BOOTINFO_ABI_MIN_COMPATIBLE_SIZE);
        return status;
    }

    if (bootInfo->Size > ORYN_BOOTINFO_ABI_CURRENT_SIZE)
    {
        KernelIoWriteString("[KERNEL] WARN: BootInfo ABI size is newer than this kernel; known fields will be used.\n");
        status.WarningCount += 1;
    }

    if (!OrynBootInfoAbiFlagsCompatible(bootInfo->Flags))
    {
        WriteValidationFail("BootInfo contains unknown ABI flag bits.", &status);
        WriteBootInfoField("[KERNEL] Unknown BootInfo flags: ", bootInfo->Flags & ~ORYN_KERNEL_BOOTINFO_KNOWN_FLAGS);
        return status;
    }

    KernelIoWriteString("[KERNEL] PASS: BootInfo ABI compatible.\n");

    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE, ORYN_BOOTINFO_WANT_KERNEL_RANGE, "kernel range", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP, ORYN_BOOTINFO_WANT_MEMORY_MAP, "memory map", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER, ORYN_BOOTINFO_WANT_FRAMEBUFFER, "framebuffer", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP, ORYN_BOOTINFO_WANT_RSDP, "RSDP", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_FIRMWARE_DATA, ORYN_BOOTINFO_WANT_FIRMWARE_DATA, "firmware data", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES, ORYN_BOOTINFO_WANT_PLATFORM_TABLES, "platform configuration tables", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT, ORYN_BOOTINFO_WANT_NVRAM, "NVRAM snapshot", &status);
    CheckDisabledFlag(bootInfo, ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES, ORYN_BOOTINFO_WANT_RUNTIME_SERVICES, "RuntimeServices", &status);

    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_KERNEL_RANGE, ORYN_BOOTINFO_WANT_KERNEL_RANGE, "kernel range", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_MEMORY_MAP, ORYN_BOOTINFO_WANT_MEMORY_MAP, "memory map", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_FRAMEBUFFER, ORYN_BOOTINFO_WANT_FRAMEBUFFER, "framebuffer", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_RSDP, ORYN_BOOTINFO_WANT_RSDP, "RSDP", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_FIRMWARE_DATA, ORYN_BOOTINFO_WANT_FIRMWARE_DATA, "firmware data", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_CONFIGURATION_TABLES, ORYN_BOOTINFO_WANT_PLATFORM_TABLES, "platform configuration tables", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_NVRAM_SNAPSHOT, ORYN_BOOTINFO_WANT_NVRAM, "NVRAM snapshot", &status);
    CheckExpectedFlag(bootInfo, ORYN_BOOTINFO_FLAG_RUNTIME_SERVICES, ORYN_BOOTINFO_WANT_RUNTIME_SERVICES, "RuntimeServices", &status);

    ValidateKernelRange(bootInfo, &status);
    ValidateMemoryMap(bootInfo, &status);
    ValidateFramebuffer(bootInfo, &status);
    ValidateRsdp(bootInfo, &status);
    ValidatePlatformTables(bootInfo, &status);
    ValidateFirmwareData(bootInfo, &status);
    ValidateNvramSnapshot(bootInfo, &status);
    ValidateRuntimeServices(bootInfo, &status);
    ValidateFont(bootInfo, &status);

    if (status.IsValid)
    {
        KernelIoWriteString("[KERNEL] PASS: BootInfo received.\n");
        KernelIoWriteString("[KERNEL] PASS: BootInfo valid.\n");
    }

    return status;
}
