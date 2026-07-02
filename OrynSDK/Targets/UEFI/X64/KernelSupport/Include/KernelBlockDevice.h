#ifndef ORYN_KERNEL_BLOCK_DEVICE_H
#define ORYN_KERNEL_BLOCK_DEVICE_H

#include "OrynStdDef.h"

#define ORYN_KERNEL_BLOCK_MAX_DEVICES 32U

typedef enum OrynKernelBlockDeviceType
{
    OrynKernelBlockDeviceTypeUnknown = 0,
    OrynKernelBlockDeviceTypeMemory = 1,
    OrynKernelBlockDeviceTypeIde = 2,
    OrynKernelBlockDeviceTypeAhci = 3,
    OrynKernelBlockDeviceTypeNvme = 4,
    OrynKernelBlockDeviceTypeVirtio = 5
} OrynKernelBlockDeviceType;

typedef struct OrynKernelBlockDevice OrynKernelBlockDevice;

typedef int (*OrynKernelBlockRead)(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, void* buffer);
typedef int (*OrynKernelBlockWrite)(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, const void* buffer);

struct OrynKernelBlockDevice
{
    uint32_t BytesPerSector;
    uint32_t SectorCount;
    uint32_t DeviceId;
    uint32_t Flags;
    OrynKernelBlockDeviceType Type;
    const char* Name;
    void* Context;
    OrynKernelBlockRead Read;
    OrynKernelBlockWrite Write;
};

typedef struct OrynKernelBlockRegistryState
{
    uint32_t Initialized;
    uint32_t DeviceCount;
    uint32_t RegisterAttempts;
    uint32_t RegisterFailures;
    uint32_t ReadRequests;
    uint32_t WriteRequests;
    uint32_t BoundsFailures;
    OrynKernelBlockDevice* Devices[ORYN_KERNEL_BLOCK_MAX_DEVICES];
} OrynKernelBlockRegistryState;

void OrynKernelBlockDeviceRegistryInit(void);
int OrynKernelBlockRegisterDevice(OrynKernelBlockDevice* device);
OrynKernelBlockDevice* OrynKernelBlockGetDevice(uint32_t index);
const OrynKernelBlockRegistryState* OrynKernelBlockGetRegistryState(void);
int OrynKernelBlockValidateRange(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count);
int OrynKernelBlockReadSectors(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, void* buffer);
int OrynKernelBlockWriteSectors(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, const void* buffer);
int OrynKernelBlockReadSector(OrynKernelBlockDevice* device, uint32_t lba, void* buffer);
int OrynKernelBlockWriteSector(OrynKernelBlockDevice* device, uint32_t lba, const void* buffer);

#endif
