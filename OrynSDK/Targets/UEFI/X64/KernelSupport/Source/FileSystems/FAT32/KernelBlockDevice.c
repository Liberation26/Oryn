#include "KernelBlockDevice.h"

static OrynKernelBlockRegistryState gBlockRegistry;

static void ClearRegistry(void)
{
    unsigned char* bytes = (unsigned char*)&gBlockRegistry;
    for (uint32_t index = 0U; index < sizeof(gBlockRegistry); ++index)
    {
        bytes[index] = 0U;
    }
}

void OrynKernelBlockDeviceRegistryInit(void)
{
    ClearRegistry();
    gBlockRegistry.Initialized = 1U;
}

int OrynKernelBlockValidateRange(
    OrynKernelBlockDevice* device,
    uint32_t lba,
    uint32_t sector_count)
{
    if (device == 0 || device->BytesPerSector == 0U || sector_count == 0U)
    {
        gBlockRegistry.BoundsFailures += 1U;
        return 0;
    }
    if (lba >= device->SectorCount || sector_count > device->SectorCount - lba)
    {
        gBlockRegistry.BoundsFailures += 1U;
        return 0;
    }
    return 1;
}

int OrynKernelBlockRegisterDevice(OrynKernelBlockDevice* device)
{
    gBlockRegistry.RegisterAttempts += 1U;
    if (!OrynKernelBlockValidateRange(device, 0U, 1U) || device->Read == 0)
    {
        gBlockRegistry.RegisterFailures += 1U;
        return 0;
    }
    if (gBlockRegistry.DeviceCount >= ORYN_KERNEL_BLOCK_MAX_DEVICES)
    {
        gBlockRegistry.RegisterFailures += 1U;
        return 0;
    }
    device->DeviceId = gBlockRegistry.DeviceCount + 1U;
    gBlockRegistry.Devices[gBlockRegistry.DeviceCount] = device;
    gBlockRegistry.DeviceCount += 1U;
    return 1;
}

OrynKernelBlockDevice* OrynKernelBlockGetDevice(uint32_t index)
{
    if (index >= gBlockRegistry.DeviceCount)
    {
        return 0;
    }
    return gBlockRegistry.Devices[index];
}

const OrynKernelBlockRegistryState* OrynKernelBlockGetRegistryState(void)
{
    return &gBlockRegistry;
}

int OrynKernelBlockReadSectors(
    OrynKernelBlockDevice* device,
    uint32_t lba,
    uint32_t sector_count,
    void* buffer)
{
    if (buffer == 0 || device == 0 || device->Read == 0 ||
        !OrynKernelBlockValidateRange(device, lba, sector_count))
    {
        return 0;
    }
    gBlockRegistry.ReadRequests += 1U;
    return device->Read(device, lba, sector_count, buffer);
}

int OrynKernelBlockWriteSectors(
    OrynKernelBlockDevice* device,
    uint32_t lba,
    uint32_t sector_count,
    const void* buffer)
{
    if (buffer == 0 || device == 0 || device->Write == 0 ||
        !OrynKernelBlockValidateRange(device, lba, sector_count))
    {
        return 0;
    }
    gBlockRegistry.WriteRequests += 1U;
    return device->Write(device, lba, sector_count, buffer);
}

int OrynKernelBlockReadSector(OrynKernelBlockDevice* device, uint32_t lba, void* buffer)
{
    return OrynKernelBlockReadSectors(device, lba, 1U, buffer);
}

int OrynKernelBlockWriteSector(OrynKernelBlockDevice* device, uint32_t lba, const void* buffer)
{
    return OrynKernelBlockWriteSectors(device, lba, 1U, buffer);
}
