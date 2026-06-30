#ifndef ORYN_KERNEL_BLOCK_DEVICE_H
#define ORYN_KERNEL_BLOCK_DEVICE_H

#include "OrynStdDef.h"

typedef struct OrynKernelBlockDevice OrynKernelBlockDevice;

typedef int (*OrynKernelBlockRead)(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, void* buffer);
typedef int (*OrynKernelBlockWrite)(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, const void* buffer);

struct OrynKernelBlockDevice
{
    uint32_t BytesPerSector;
    uint32_t SectorCount;
    void* Context;
    OrynKernelBlockRead Read;
    OrynKernelBlockWrite Write;
};

int OrynKernelBlockReadSector(OrynKernelBlockDevice* device, uint32_t lba, void* buffer);
int OrynKernelBlockWriteSector(OrynKernelBlockDevice* device, uint32_t lba, const void* buffer);

#endif
