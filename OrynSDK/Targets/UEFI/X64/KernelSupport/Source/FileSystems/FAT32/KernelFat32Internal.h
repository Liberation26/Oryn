#ifndef ORYN_KERNEL_FAT32_INTERNAL_H
#define ORYN_KERNEL_FAT32_INTERNAL_H

#include "KernelFat32.h"
#include "OrynString.h"

#define FAT32_EOC 0x0FFFFFF8U
#define FAT32_FREE 0x00000000U
#define FAT32_BAD 0x0FFFFFF7U
#define FAT32_DIR_ENTRY_SIZE 32U
#define ORYN_FAT32_MAX_CLUSTER_BYTES 4096U
#define FAT32_MAX_PATH_PART 255U
#define FAT32_FSINFO_SIGNATURE1 0x41615252U
#define FAT32_FSINFO_SIGNATURE2 0x61417272U
#define FAT32_FSINFO_SIGNATURE3 0xAA550000U

typedef struct Fat32PathPart
{
    char Text[256];
    int IsLast;
} Fat32PathPart;

uint16_t Fat32Read16(const uint8_t* data, uint32_t offset);
uint32_t Fat32Read32(const uint8_t* data, uint32_t offset);
void Fat32Write16(uint8_t* data, uint32_t offset, uint16_t value);
void Fat32Write32(uint8_t* data, uint32_t offset, uint32_t value);
int Fat32IsEndOfChain(uint32_t value);
uint32_t Fat32ClusterToSector(const OrynFat32Volume* volume, uint32_t cluster);
uint32_t Fat32ClusterSize(const OrynFat32Volume* volume);
int Fat32ReadDirectoryEntryName(const uint8_t* entry, char* output, uint32_t output_size);
void Fat32MakeShortName(const char* name, uint8_t output[11]);
int Fat32NamesEqual(const char* left, const char* right);
int Fat32NextPathPart(const char** cursor, Fat32PathPart* part);
int Fat32FindInDirectory(OrynFat32Volume* volume, uint32_t directory_cluster, const char* name, OrynFat32FileInfo* info);
int Fat32FindFreeDirectoryEntry(OrynFat32Volume* volume, uint32_t directory_cluster, uint32_t* offset, uint8_t* sector);
int Fat32WriteDirectoryEntry(OrynFat32Volume* volume, uint32_t directory_cluster, uint32_t offset, const OrynFat32FileInfo* info);
int Fat32AllocateCluster(OrynFat32Volume* volume, uint32_t* cluster);
int Fat32FreeClusterChain(OrynFat32Volume* volume, uint32_t start_cluster);
int Fat32AppendCluster(OrynFat32Volume* volume, uint32_t chain_start, uint32_t* new_cluster);
int Fat32ResolveParent(OrynFat32Volume* volume, const char* path, uint32_t* parent_cluster, char* leaf, uint32_t leaf_size);
int Fat32SetStatus(OrynFat32Volume* volume, uint32_t status);
int Fat32ClusterIsValid(const OrynFat32Volume* volume, uint32_t cluster);
int Fat32PathIsSafeShortName(const char* path);
int Fat32DirectoryIsEmpty(OrynFat32Volume* volume, uint32_t directory_cluster);
int Fat32TruncateChain(OrynFat32Volume* volume, uint32_t start_cluster, uint32_t keep_count);
int Fat32UpdateFsInfo(OrynFat32Volume* volume);

#endif
