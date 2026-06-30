#include "KernelBlockDevice.h"

int OrynKernelBlockReadSector(OrynKernelBlockDevice* device, uint32_t lba, void* buffer)
{
    if (device == 0 || buffer == 0 || device->Read == 0 || device->BytesPerSector == 0U)
    {
        return 0;
    }

    if (lba >= device->SectorCount)
    {
        return 0;
    }

    return device->Read(device, lba, 1U, buffer);
}

int OrynKernelBlockWriteSector(OrynKernelBlockDevice* device, uint32_t lba, const void* buffer)
{
    if (device == 0 || buffer == 0 || device->Write == 0 || device->BytesPerSector == 0U)
    {
        return 0;
    }

    if (lba >= device->SectorCount)
    {
        return 0;
    }

    return device->Write(device, lba, 1U, buffer);
}
