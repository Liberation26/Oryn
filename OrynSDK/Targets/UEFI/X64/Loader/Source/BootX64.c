#include "BootX64Internal.h"
#include "OrynBootInfoSelection.h"

#ifndef ORYN_BOOTINFO_WANT_PLATFORM_TABLES
#define ORYN_BOOTINFO_WANT_PLATFORM_TABLES ORYN_BOOTINFO_WANT_FIRMWARE_DATA
#endif

#ifndef ORYN_BOOTINFO_WANT_NVRAM
#define ORYN_BOOTINFO_WANT_NVRAM ORYN_BOOTINFO_WANT_FIRMWARE_DATA
#endif

#ifndef ORYN_BOOTINFO_WANT_RUNTIME_SERVICES
#define ORYN_BOOTINFO_WANT_RUNTIME_SERVICES ORYN_BOOTINFO_WANT_FIRMWARE_DATA
#endif

EFI_SYSTEM_TABLE* gSystemTable;
EFI_BOOT_SERVICES* gBootServices;

EFI_GUID gLoadedImageProtocolGuid =
{
    0x5B1B31A1U, 0x9562U, 0x11D2U,
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
};

EFI_GUID gSimpleFileSystemProtocolGuid =
{
    0x0964E5B22U, 0x6459U, 0x11D2U,
    { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
};

static EFI_GUID gGraphicsOutputProtocolGuid =
{
    0x9042A9DEU, 0x23DCU, 0x4A38U,
    { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A }
};


static void CopyAscii(char* target, UINTN targetSize, const char* source)
{
    UINTN index = 0;
    while (index + 1 < targetSize && source[index] != 0)
    {
        target[index] = source[index];
        ++index;
    }
    target[index] = 0;
}


static void CopyUtf16ToAscii(char* target, UINTN targetSize, const CHAR16* source)
{
    UINTN index = 0;
    if (targetSize == 0)
    {
        return;
    }

    if (source == ORYN_NULL)
    {
        target[0] = 0;
        return;
    }

    while (index + 1 < targetSize && source[index] != 0)
    {
        CHAR16 ch = source[index];
        target[index] = (ch >= 32U && ch <= 126U) ? (char)ch : '?';
        ++index;
    }

    target[index] = 0;
}

static void CaptureFirmwareData(OrynBootInfo* bootInfo)
{
    OrynBootFirmwareData* firmware = &bootInfo->FirmwareData;
    SetMemory(firmware, 0, sizeof(*firmware));
    firmware->Version = 1U;
    firmware->Size = sizeof(*firmware);
    firmware->UefiRevision = gSystemTable->Hdr.Revision;
    firmware->FirmwareRevision = gSystemTable->FirmwareRevision;
    firmware->ConfigurationTableCount = gSystemTable->NumberOfTableEntries;
    firmware->BootTimeTimeZone = 2047;
    CopyUtf16ToAscii(firmware->FirmwareVendor, sizeof(firmware->FirmwareVendor), gSystemTable->FirmwareVendor);

    if (gSystemTable->RuntimeServices != ORYN_NULL && gSystemTable->RuntimeServices->GetTime != ORYN_NULL)
    {
        EFI_TIME time;
        EFI_STATUS status = gSystemTable->RuntimeServices->GetTime(&time, ORYN_NULL);
        if (!IsError(status))
        {
            firmware->BootTimeYear = time.Year;
            firmware->BootTimeMonth = time.Month;
            firmware->BootTimeDay = time.Day;
            firmware->BootTimeHour = time.Hour;
            firmware->BootTimeMinute = time.Minute;
            firmware->BootTimeSecond = time.Second;
            firmware->BootTimeNanosecond = time.Nanosecond;
            firmware->BootTimeTimeZone = time.TimeZone;
            firmware->BootTimeDaylight = time.Daylight;
            firmware->BootTimeValid = 1U;
        }
    }

    bootInfo->Flags |= ORYN_BOOTINFO_FLAG_FIRMWARE_DATA;
    Print("[BOOT] BootInfo firmware vendor: ");
    Print(firmware->FirmwareVendor[0] != 0 ? firmware->FirmwareVendor : "unknown");
    Print("\n");
    Print("[BOOT] BootInfo UEFI revision: ");
    PrintHex64(firmware->UefiRevision);
    Print("\n");
    Print("[BOOT] BootInfo firmware revision: ");
    PrintHex64(firmware->FirmwareRevision);
    Print("\n");
    Print("[BOOT] BootInfo firmware config tables: ");
    PrintHex64(firmware->ConfigurationTableCount);
    Print("\n");
    Print(firmware->BootTimeValid ? "[BOOT] BootInfo firmware boot time: captured.\n" : "[BOOT] BootInfo firmware boot time: unavailable.\n");
}

static void FillBaseBootInfo(
    OrynBootInfo* bootInfo,
    UINT64 kernelBase,
    UINT64 kernelSize,
    void* fontBuffer,
    UINTN fontSize)
{
    SetMemory(bootInfo, 0, sizeof(*bootInfo));
    bootInfo->Signature = ORYN_BOOTINFO_SIGNATURE;
    bootInfo->Version = ORYN_BOOTINFO_VERSION;
    bootInfo->Size = sizeof(*bootInfo);
#if ORYN_BOOTINFO_WANT_KERNEL_RANGE
    bootInfo->KernelPhysicalBase = kernelBase;
    bootInfo->KernelSize = kernelSize;
    bootInfo->Flags |= ORYN_BOOTINFO_FLAG_KERNEL_RANGE;
#else
    (void)kernelBase;
    (void)kernelSize;
#endif
    if (fontBuffer != ORYN_NULL && fontSize != 0)
    {
        bootInfo->FontBase = (UINT64)(UINTN)fontBuffer;
        bootInfo->FontSize = (UINT64)fontSize;
        bootInfo->Flags |= ORYN_BOOTINFO_FLAG_FONT;
        CopyAscii(bootInfo->FontName, sizeof(bootInfo->FontName), "ORYNSANS.TTF");
    }

    CopyAscii(bootInfo->BootLoaderName, sizeof(bootInfo->BootLoaderName), "Oryn BOOTX64.EFI");
    CopyAscii(bootInfo->KernelName, sizeof(bootInfo->KernelName), "Kernel-5");
}

static int CaptureFramebufferToBootInfo(OrynBootInfo* bootInfo, int selected)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics = ORYN_NULL;
    EFI_STATUS status = gBootServices->LocateProtocol(&gGraphicsOutputProtocolGuid, ORYN_NULL, (void**)&graphics);
    if (IsError(status) || graphics == ORYN_NULL || graphics->Mode == ORYN_NULL || graphics->Mode->Info == ORYN_NULL)
    {
        if (selected)
        {
            Print("[BOOT] BootInfo framebuffer: not available.\n");
        }
        else
        {
            Print("[BOOT] Default screen framebuffer: not available.\n");
        }
        return 0;
    }

    bootInfo->FramebufferBase = (UINT64)graphics->Mode->FrameBufferBase;
    bootInfo->FramebufferSize = (UINT64)graphics->Mode->FrameBufferSize;
    bootInfo->FramebufferWidth = graphics->Mode->Info->HorizontalResolution;
    bootInfo->FramebufferHeight = graphics->Mode->Info->VerticalResolution;
    bootInfo->FramebufferPixelsPerScanLine = graphics->Mode->Info->PixelsPerScanLine;
    bootInfo->FramebufferPixelFormat = graphics->Mode->Info->PixelFormat;
    bootInfo->FramebufferMode = graphics->Mode->Mode;
    bootInfo->FramebufferMaxMode = graphics->Mode->MaxMode;
    bootInfo->FramebufferInfoVersion = graphics->Mode->Info->Version;
    bootInfo->FramebufferInfoSize = (UINT32)graphics->Mode->SizeOfInfo;
    bootInfo->FramebufferRedMask = graphics->Mode->Info->PixelInformation[0];
    bootInfo->FramebufferGreenMask = graphics->Mode->Info->PixelInformation[1];
    bootInfo->FramebufferBlueMask = graphics->Mode->Info->PixelInformation[2];
    bootInfo->FramebufferReservedMask = graphics->Mode->Info->PixelInformation[3];

    if (selected)
    {
        bootInfo->Flags |= ORYN_BOOTINFO_FLAG_FRAMEBUFFER;
        Print("[BOOT] BootInfo framebuffer base: ");
    }
    else
    {
        Print("[BOOT] Default screen framebuffer base: ");
    }

    PrintHex64(bootInfo->FramebufferBase);
    Print("\n");
    return 1;
}

static void PrintBootInfoSelection(void)
{
    Print("[BOOT] BootInfo selection: ");
    Print(ORYN_BOOTINFO_SELECTION_NAME);
    Print("\n");
#if ORYN_BOOTINFO_WANT_KERNEL_RANGE
    Print("[BOOT] BootInfo selection kernel range: enabled.\n");
#else
    Print("[BOOT] BootInfo selection kernel range: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_MEMORY_MAP
    Print("[BOOT] BootInfo selection memory map: enabled.\n");
#else
    Print("[BOOT] BootInfo selection memory map: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_FRAMEBUFFER
    Print("[BOOT] BootInfo selection framebuffer: enabled.\n");
#else
    Print("[BOOT] BootInfo selection framebuffer: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_RSDP
    Print("[BOOT] BootInfo selection RSDP: enabled.\n");
#else
    Print("[BOOT] BootInfo selection RSDP: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_FIRMWARE_DATA
    Print("[BOOT] BootInfo selection firmware data: enabled.\n");
#else
    Print("[BOOT] BootInfo selection firmware data: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_PLATFORM_TABLES
    Print("[BOOT] BootInfo selection platform tables: enabled.\n");
#else
    Print("[BOOT] BootInfo selection platform tables: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_NVRAM
    Print("[BOOT] BootInfo selection NVRAM snapshot: enabled.\n");
#else
    Print("[BOOT] BootInfo selection NVRAM snapshot: disabled.\n");
#endif
#if ORYN_BOOTINFO_WANT_RUNTIME_SERVICES
    Print("[BOOT] BootInfo selection RuntimeServices: enabled.\n");
#else
    Print("[BOOT] BootInfo selection RuntimeServices: disabled.\n");
#endif
}

static EFI_STATUS AllocateBootInfo(
    OrynBootInfo** outBootInfo,
    UINT64 kernelBase,
    UINT64 kernelSize,
    void* fontBuffer,
    UINTN fontSize)
{
    OrynBootInfo* bootInfo = ORYN_NULL;
    EFI_STATUS status = gBootServices->AllocatePool(EfiLoaderData, sizeof(OrynBootInfo), (void**)&bootInfo);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not allocate BootInfo.\n");
        return status;
    }

    FillBaseBootInfo(bootInfo, kernelBase, kernelSize, fontBuffer, fontSize);
#if ORYN_BOOTINFO_WANT_FIRMWARE_DATA
    CaptureFirmwareData(bootInfo);
#else
    Print("[BOOT] BootInfo firmware data: disabled by user selection.\n");
#endif
#if ORYN_BOOTINFO_WANT_PLATFORM_TABLES || ORYN_BOOTINFO_WANT_RSDP
    OrynCapturePlatformTables(bootInfo, ORYN_BOOTINFO_WANT_PLATFORM_TABLES, ORYN_BOOTINFO_WANT_RSDP);
#else
    Print("[BOOT] BootInfo platform tables and RSDP: disabled by user selection.\n");
#endif
#if ORYN_BOOTINFO_WANT_NVRAM
    OrynCaptureNvramSnapshot(bootInfo);
#else
    Print("[BOOT] BootInfo NVRAM snapshot: disabled by user selection.\n");
#endif
#if ORYN_BOOTINFO_WANT_RUNTIME_SERVICES
    OrynCaptureRuntimeServices(bootInfo);
#else
    Print("[BOOT] BootInfo RuntimeServices: disabled by user selection.\n");
#endif
#if ORYN_BOOTINFO_WANT_FRAMEBUFFER
    CaptureFramebufferToBootInfo(bootInfo, 1);
#else
    Print("[BOOT] BootInfo framebuffer: disabled by user selection.\n");
    CaptureFramebufferToBootInfo(bootInfo, 0);
#endif

    if (fontBuffer != ORYN_NULL && fontSize != 0)
    {
        Print("[BOOT] BootInfo font: ORYNSANS.TTF at ");
        PrintHex64((UINT64)(UINTN)fontBuffer);
        Print(" size ");
        PrintHex64((UINT64)fontSize);
        Print(" bytes.\n");
    }
    else
    {
        Print("[BOOT] BootInfo font: not supplied.\n");
    }

    Print("[BOOT] BootInfo allocated at: ");
    PrintHex64((UINT64)(UINTN)bootInfo);
    Print("\n");

    *outBootInfo = bootInfo;
    return EFI_SUCCESS;
}

EFI_STATUS OrynUefiLoaderMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
    gSystemTable = systemTable;
    gBootServices = systemTable->BootServices;
    InitSerialDebug();
    gBootServices->SetWatchdogTimer(0, 0, 0, ORYN_NULL);

    Print("[BOOT] UEFI loader entry point: efi_main -> OrynUefiLoaderMain.\n");
    Print("[BOOT] Stage 01: Oryn BOOTX64.EFI started.\n");
    PrintBootInfoSelection();

    EFI_FILE_PROTOCOL* kernelFile = ORYN_NULL;
    EFI_STATUS status = OpenKernelFile(imageHandle, &kernelFile);
    if (IsError(status))
    {
        return status;
    }

    void* kernelBuffer = ORYN_NULL;
    UINTN kernelSize = 0;
    status = ReadKernelFile(kernelFile, &kernelBuffer, &kernelSize);
    if (IsError(status))
    {
        return status;
    }

    void* fontBuffer = ORYN_NULL;
    UINTN fontSize = 0;
    (void)ReadOptionalFontFile(imageHandle, &fontBuffer, &fontSize);

    UINT64 kernelEntry = 0;
    UINT64 kernelBase = 0;
    UINT64 kernelLoadedSize = 0;
    status = LoadElfSegments(kernelBuffer, kernelSize, &kernelEntry, &kernelBase, &kernelLoadedSize);
    if (IsError(status))
    {
        return status;
    }

    OrynBootInfo* bootInfo = ORYN_NULL;
    status = AllocateBootInfo(&bootInfo, kernelBase, kernelLoadedSize, fontBuffer, fontSize);
    if (IsError(status))
    {
        return status;
    }

    Print("[BOOT] Kernel loaded. Preparing BootInfo handoff.\n");
    status = ExitBootServicesWithBootInfo(imageHandle, bootInfo);
    if (IsError(status))
    {
        return status;
    }

    JumpToKernel(kernelEntry, bootInfo);
}
