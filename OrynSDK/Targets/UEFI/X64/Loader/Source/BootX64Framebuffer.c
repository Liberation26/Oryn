#include "BootX64Internal.h"

#define ORYN_EFI_PIXEL_RGB_RESERVED_8BIT 0U
#define ORYN_EFI_PIXEL_BGR_RESERVED_8BIT 1U
#define ORYN_EFI_PIXEL_BITMASK 2U
#define ORYN_EFI_PIXEL_BLT_ONLY 3U

static EFI_GUID gGraphicsOutputProtocolGuid =
{
    0x9042A9DEU, 0x23DCU, 0x4A38U,
    { 0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A }
};

static void SetKnownRgbMasks(OrynBootFramebuffer* framebuffer, int redFirst)
{
    if (redFirst)
    {
        framebuffer->RedMask = 0x000000FFU;
        framebuffer->GreenMask = 0x0000FF00U;
        framebuffer->BlueMask = 0x00FF0000U;
        framebuffer->ReservedMask = 0xFF000000U;
    }
    else
    {
        framebuffer->RedMask = 0x00FF0000U;
        framebuffer->GreenMask = 0x0000FF00U;
        framebuffer->BlueMask = 0x000000FFU;
        framebuffer->ReservedMask = 0xFF000000U;
    }
}

static int SetPlainFramebufferFormat(
    OrynBootFramebuffer* framebuffer,
    const EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE_INFORMATION* info)
{
    framebuffer->BytesPerPixel = 4U;

    if (info->PixelFormat == ORYN_EFI_PIXEL_RGB_RESERVED_8BIT)
    {
        framebuffer->PixelFormat = ORYN_FRAMEBUFFER_PIXEL_FORMAT_RGBX8888;
        SetKnownRgbMasks(framebuffer, 1);
        return 1;
    }

    if (info->PixelFormat == ORYN_EFI_PIXEL_BGR_RESERVED_8BIT)
    {
        framebuffer->PixelFormat = ORYN_FRAMEBUFFER_PIXEL_FORMAT_BGRX8888;
        SetKnownRgbMasks(framebuffer, 0);
        return 1;
    }

    if (info->PixelFormat == ORYN_EFI_PIXEL_BITMASK)
    {
        framebuffer->PixelFormat = ORYN_FRAMEBUFFER_PIXEL_FORMAT_BITMASK32;
        framebuffer->RedMask = info->PixelInformation[0];
        framebuffer->GreenMask = info->PixelInformation[1];
        framebuffer->BlueMask = info->PixelInformation[2];
        framebuffer->ReservedMask = info->PixelInformation[3];
        return 1;
    }

    if (info->PixelFormat == ORYN_EFI_PIXEL_BLT_ONLY)
    {
        framebuffer->PixelFormat = ORYN_FRAMEBUFFER_PIXEL_FORMAT_UNKNOWN;
        return 0;
    }

    framebuffer->PixelFormat = ORYN_FRAMEBUFFER_PIXEL_FORMAT_UNKNOWN;
    return 0;
}

int CaptureFramebufferToBootInfo(OrynBootInfo* bootInfo, int selected)
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics = ORYN_NULL;
    EFI_STATUS status = gBootServices->LocateProtocol(&gGraphicsOutputProtocolGuid, ORYN_NULL, (void**)&graphics);
    if (IsError(status) || graphics == ORYN_NULL || graphics->Mode == ORYN_NULL || graphics->Mode->Info == ORYN_NULL)
    {
        Print(selected ?
            "[BOOT] BootInfo framebuffer: not available.\n" :
            "[BOOT] Default screen framebuffer: not available.\n");
        return 0;
    }

    bootInfo->Framebuffer.Base = (UINT64)graphics->Mode->FrameBufferBase;
    bootInfo->Framebuffer.Size = (UINT64)graphics->Mode->FrameBufferSize;
    bootInfo->Framebuffer.Width = graphics->Mode->Info->HorizontalResolution;
    bootInfo->Framebuffer.Height = graphics->Mode->Info->VerticalResolution;
    bootInfo->Framebuffer.PixelsPerScanLine = graphics->Mode->Info->PixelsPerScanLine;

    if (!SetPlainFramebufferFormat(&bootInfo->Framebuffer, graphics->Mode->Info))
    {
        Print("[BOOT] BootInfo framebuffer: unavailable as a linear 32-bit framebuffer.\n");
        return 0;
    }

    if (selected)
    {
        bootInfo->Flags |= ORYN_BOOTINFO_FLAG_FRAMEBUFFER;
        Print("[BOOT] BootInfo linear framebuffer base: ");
    }
    else
    {
        Print("[BOOT] Default screen linear framebuffer base: ");
    }

    PrintHex64(bootInfo->Framebuffer.Base);
    Print("\n");
    return 1;
}
