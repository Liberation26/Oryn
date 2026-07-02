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

uint64_t OrynKernelBlockGetBlockCount(const OrynKernelBlockDevice* device)
{
    if (device == 0)
    {
        return 0ULL;
    }
    if (device->BlockCount != 0ULL)
    {
        return device->BlockCount;
    }
    return (uint64_t)device->SectorCount;
}

int OrynKernelBlockIsProductionReady(const OrynKernelBlockDevice* device)
{
    if (device == 0 || device->BytesPerSector == 0U)
    {
        return 0;
    }
    if (OrynKernelBlockGetBlockCount(device) == 0ULL || device->Flush == 0)
    {
        return 0;
    }
    return device->ReadRange != 0 || device->Read != 0;
}

int OrynKernelBlockValidateRange64(
    OrynKernelBlockDevice* device,
    uint64_t lba,
    uint32_t sector_count)
{
    uint64_t block_count = OrynKernelBlockGetBlockCount(device);
    if (device == 0 || device->BytesPerSector == 0U || sector_count == 0U)
    {
        gBlockRegistry.BoundsFailures += 1U;
        return 0;
    }
    if (device->BytesPerSector > ORYN_KERNEL_BLOCK_MAX_SECTOR_SIZE)
    {
        gBlockRegistry.BoundsFailures += 1U;
        return 0;
    }
    if (lba >= block_count || (uint64_t)sector_count > block_count - lba)
    {
        gBlockRegistry.BoundsFailures += 1U;
        return 0;
    }
    return 1;
}

int OrynKernelBlockValidateRange(
    OrynKernelBlockDevice* device,
    uint32_t lba,
    uint32_t sector_count)
{
    return OrynKernelBlockValidateRange64(device, (uint64_t)lba, sector_count);
}

int OrynKernelBlockRegisterDevice(OrynKernelBlockDevice* device)
{
    gBlockRegistry.RegisterAttempts += 1U;
    if (!OrynKernelBlockIsProductionReady(device) ||
        !OrynKernelBlockValidateRange64(device, 0ULL, 1U))
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
    gBlockRegistry.ProductionReady += 1U;
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

int OrynKernelBlockReadSectors64(
    OrynKernelBlockDevice* device,
    uint64_t lba,
    uint32_t sector_count,
    void* buffer)
{
    if (buffer == 0 || !OrynKernelBlockValidateRange64(device, lba, sector_count))
    {
        return 0;
    }
    gBlockRegistry.ReadRequests += 1U;
    if (device->ReadRange != 0)
    {
        return device->ReadRange(device, lba, sector_count, buffer);
    }
    if (device->Read == 0 || lba > 0xFFFFFFFFULL)
    {
        return 0;
    }
    return device->Read(device, (uint32_t)lba, sector_count, buffer);
}

int OrynKernelBlockWriteSectors64(
    OrynKernelBlockDevice* device,
    uint64_t lba,
    uint32_t sector_count,
    const void* buffer)
{
    if (buffer == 0 || !OrynKernelBlockValidateRange64(device, lba, sector_count))
    {
        return 0;
    }
    gBlockRegistry.WriteRequests += 1U;
    if (device->WriteRange != 0)
    {
        return device->WriteRange(device, lba, sector_count, buffer);
    }
    if (device->Write == 0 || lba > 0xFFFFFFFFULL)
    {
        return 0;
    }
    return device->Write(device, (uint32_t)lba, sector_count, buffer);
}

int OrynKernelBlockReadSectors(
    OrynKernelBlockDevice* device,
    uint32_t lba,
    uint32_t sector_count,
    void* buffer)
{
    return OrynKernelBlockReadSectors64(device, (uint64_t)lba, sector_count, buffer);
}

int OrynKernelBlockWriteSectors(
    OrynKernelBlockDevice* device,
    uint32_t lba,
    uint32_t sector_count,
    const void* buffer)
{
    return OrynKernelBlockWriteSectors64(device, (uint64_t)lba, sector_count, buffer);
}

int OrynKernelBlockFlushDevice(OrynKernelBlockDevice* device)
{
    if (device == 0 || device->Flush == 0)
    {
        gBlockRegistry.FlushFailures += 1U;
        return 0;
    }
    gBlockRegistry.FlushRequests += 1U;
    return device->Flush(device);
}

int OrynKernelBlockReadSector(OrynKernelBlockDevice* device, uint32_t lba, void* buffer)
{
    return OrynKernelBlockReadSectors(device, lba, 1U, buffer);
}

int OrynKernelBlockWriteSector(OrynKernelBlockDevice* device, uint32_t lba, const void* buffer)
{
    return OrynKernelBlockWriteSectors(device, lba, 1U, buffer);
}
