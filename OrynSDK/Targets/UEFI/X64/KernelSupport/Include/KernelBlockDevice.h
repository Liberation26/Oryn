#ifndef ORYN_KERNEL_BLOCK_DEVICE_H
#define ORYN_KERNEL_BLOCK_DEVICE_H

#include "OrynStdDef.h"

#define ORYN_KERNEL_BLOCK_MAX_DEVICES 32U
#define ORYN_KERNEL_BLOCK_MAX_SECTOR_SIZE 4096U

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
typedef int (*OrynKernelBlockReadRange)(OrynKernelBlockDevice* device, uint64_t lba, uint32_t sector_count, void* buffer);
typedef int (*OrynKernelBlockWriteRange)(OrynKernelBlockDevice* device, uint64_t lba, uint32_t sector_count, const void* buffer);
typedef int (*OrynKernelBlockFlush)(OrynKernelBlockDevice* device);

struct OrynKernelBlockDevice
{
    uint32_t BytesPerSector;
    uint32_t SectorCount;
    uint64_t BlockCount;
    uint32_t DeviceId;
    uint32_t Flags;
    OrynKernelBlockDeviceType Type;
    const char* Name;
    void* Context;
    OrynKernelBlockRead Read;
    OrynKernelBlockWrite Write;
    OrynKernelBlockReadRange ReadRange;
    OrynKernelBlockWriteRange WriteRange;
    OrynKernelBlockFlush Flush;
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
    uint32_t FlushRequests;
    uint32_t FlushFailures;
    uint32_t ProductionReady;
    OrynKernelBlockDevice* Devices[ORYN_KERNEL_BLOCK_MAX_DEVICES];
} OrynKernelBlockRegistryState;

void OrynKernelBlockDeviceRegistryInit(void);
int OrynKernelBlockRegisterDevice(OrynKernelBlockDevice* device);
OrynKernelBlockDevice* OrynKernelBlockGetDevice(uint32_t index);
const OrynKernelBlockRegistryState* OrynKernelBlockGetRegistryState(void);
uint64_t OrynKernelBlockGetBlockCount(const OrynKernelBlockDevice* device);
int OrynKernelBlockIsProductionReady(const OrynKernelBlockDevice* device);
int OrynKernelBlockValidateRange64(OrynKernelBlockDevice* device, uint64_t lba, uint32_t sector_count);
int OrynKernelBlockValidateRange(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count);
int OrynKernelBlockReadSectors64(OrynKernelBlockDevice* device, uint64_t lba, uint32_t sector_count, void* buffer);
int OrynKernelBlockWriteSectors64(OrynKernelBlockDevice* device, uint64_t lba, uint32_t sector_count, const void* buffer);
int OrynKernelBlockReadSectors(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, void* buffer);
int OrynKernelBlockWriteSectors(OrynKernelBlockDevice* device, uint32_t lba, uint32_t sector_count, const void* buffer);
int OrynKernelBlockFlushDevice(OrynKernelBlockDevice* device);
int OrynKernelBlockReadSector(OrynKernelBlockDevice* device, uint32_t lba, void* buffer);
int OrynKernelBlockWriteSector(OrynKernelBlockDevice* device, uint32_t lba, const void* buffer);

#endif
