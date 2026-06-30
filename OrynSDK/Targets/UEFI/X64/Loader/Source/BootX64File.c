#include "BootX64Internal.h"
#include "OrynBootTarget.h"

static EFI_STATUS OpenEspRoot(EFI_HANDLE imageHandle, EFI_FILE_PROTOCOL** outRoot)
{
    EFI_LOADED_IMAGE_PROTOCOL* loadedImage = ORYN_NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem = ORYN_NULL;
    EFI_STATUS status = gBootServices->OpenProtocol(
        imageHandle,
        &gLoadedImageProtocolGuid,
        (void**)&loadedImage,
        imageHandle,
        ORYN_NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not open LoadedImage protocol.\n");
        return status;
    }

    status = gBootServices->OpenProtocol(
        loadedImage->DeviceHandle,
        &gSimpleFileSystemProtocolGuid,
        (void**)&fileSystem,
        imageHandle,
        ORYN_NULL,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not open SimpleFileSystem protocol.\n");
        return status;
    }

    status = fileSystem->OpenVolume(fileSystem, outRoot);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not open ESP volume.\n");
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenKernelFile(EFI_HANDLE imageHandle, EFI_FILE_PROTOCOL** outFile)
{
    EFI_FILE_PROTOCOL* root = ORYN_NULL;
    EFI_STATUS status = OpenEspRoot(imageHandle, &root);
    if (IsError(status))
    {
        return status;
    }

    Print("[BOOT] Stage 02: Opening kernel image: ");
    Print(ORYN_BOOT_TARGET_KERNEL_PATH_TEXT);
    Print(".\n");
    status = root->Open(root, outFile, ORYN_BOOT_TARGET_KERNEL_PATH, EFI_FILE_MODE_READ, 0);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not open kernel image: ");
        Print(ORYN_BOOT_TARGET_KERNEL_PATH_TEXT);
        Print(".\n");
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS ReadKernelFile(EFI_FILE_PROTOCOL* file, void** outBuffer, UINTN* outSize)
{
    void* buffer = ORYN_NULL;
    EFI_STATUS status = gBootServices->AllocatePool(EfiLoaderData, MAX_KERNEL_FILE_SIZE, &buffer);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not allocate kernel file buffer.\n");
        return status;
    }

    Print("[BOOT] Stage 03: Reading kernel image: ");
    Print(ORYN_BOOT_TARGET_KERNEL_FILE);
    Print(".\n");
    UINTN size = MAX_KERNEL_FILE_SIZE;
    status = file->Read(file, &size, buffer);
    if (IsError(status))
    {
        Print("[BOOT] FAIL: Could not read kernel file.\n");
        return status;
    }

    Print("[BOOT] Kernel file size: ");
    PrintHex64((UINT64)size);
    Print(" bytes.\n");

    *outBuffer = buffer;
    *outSize = size;
    return EFI_SUCCESS;
}

static EFI_STATUS ReadOptionalFile(EFI_FILE_PROTOCOL* file, UINTN maxSize, void** outBuffer, UINTN* outSize)
{
    void* buffer = ORYN_NULL;
    EFI_STATUS status = gBootServices->AllocatePool(EfiLoaderData, maxSize, &buffer);
    if (IsError(status))
    {
        return status;
    }

    UINTN size = maxSize;
    status = file->Read(file, &size, buffer);
    if (IsError(status))
    {
        return status;
    }

    *outBuffer = buffer;
    *outSize = size;
    return EFI_SUCCESS;
}

EFI_STATUS ReadOptionalFontFile(EFI_HANDLE imageHandle, void** outBuffer, UINTN* outSize)
{
    EFI_FILE_PROTOCOL* root = ORYN_NULL;
    EFI_FILE_PROTOCOL* fontFile = ORYN_NULL;
    EFI_STATUS status = OpenEspRoot(imageHandle, &root);
    if (IsError(status))
    {
        return status;
    }

    Print("[BOOT] Stage 03F: Looking for \\System\\Fonts\\ORYNSANS.TTF.\n");
    status = root->Open(root, &fontFile, L"\\System\\Fonts\\ORYNSANS.TTF", EFI_FILE_MODE_READ, 0);
    if (IsError(status))
    {
        Print("[BOOT] TTF font: not found. Kernel will use built-in console glyphs.\n");
        *outBuffer = ORYN_NULL;
        *outSize = 0;
        return EFI_NOT_FOUND;
    }

    status = ReadOptionalFile(fontFile, MAX_TTF_FILE_SIZE, outBuffer, outSize);
    if (IsError(status))
    {
        Print("[BOOT] TTF font: found but could not be read.\n");
        *outBuffer = ORYN_NULL;
        *outSize = 0;
        return status;
    }

    Print("[BOOT] TTF font loaded: ORYNSANS.TTF size ");
    PrintHex64((UINT64)*outSize);
    Print(" bytes.\n");
    return EFI_SUCCESS;
}
