#include "KernelFat32Internal.h"

static void Fat32WriteLabel(uint8_t* sector, const char* label)
{
    uint32_t index;
    for (index = 0; index < 11U; ++index)
    {
        sector[71U + index] = (uint8_t)' ';
    }
    for (index = 0; label != 0 && label[index] != 0 && index < 11U; ++index)
    {
        sector[71U + index] = (uint8_t)label[index];
    }
}

int OrynFat32FormatMemoryImage(uint8_t* image, uint32_t sector_count, const char* volume_label)
{
    uint8_t* boot = image;
    uint8_t* fsinfo = image + ORYN_FAT32_SECTOR_SIZE;
    uint8_t* fat0;
    uint8_t* fat1;
    uint32_t reserved = 32U;
    uint32_t fat_size = 16U;
    uint32_t fat_count = 2U;
    uint32_t first_data;
    uint32_t root_cluster = 2U;

    if (image == 0 || sector_count < 128U)
    {
        return 0;
    }

    OrynMemset(image, 0, (size_t)sector_count * ORYN_FAT32_SECTOR_SIZE);
    first_data = reserved + (fat_count * fat_size);
    boot[0] = 0xEBU;
    boot[1] = 0x58U;
    boot[2] = 0x90U;
    boot[3] = 'O'; boot[4] = 'R'; boot[5] = 'Y'; boot[6] = 'N';
    boot[7] = 'F'; boot[8] = 'S'; boot[9] = ' '; boot[10] = ' ';
    Fat32Write16(boot, 11U, ORYN_FAT32_SECTOR_SIZE);
    boot[13] = 1U;
    Fat32Write16(boot, 14U, (uint16_t)reserved);
    boot[16] = (uint8_t)fat_count;
    Fat32Write32(boot, 32U, sector_count);
    Fat32Write32(boot, 36U, fat_size);
    Fat32Write32(boot, 44U, root_cluster);
    Fat32Write16(boot, 48U, 1U);
    Fat32Write16(boot, 50U, 6U);
    boot[64] = 0x80U;
    boot[66] = 0x29U;
    Fat32Write32(boot, 67U, 0x20260630U);
    Fat32WriteLabel(boot, volume_label);
    boot[82]='F'; boot[83]='A'; boot[84]='T'; boot[85]='3'; boot[86]='2';
    boot[510] = 0x55U;
    boot[511] = 0xAAU;

    Fat32Write32(fsinfo, 0U, 0x41615252U);
    Fat32Write32(fsinfo, 484U, 0x61417272U);
    Fat32Write32(fsinfo, 488U, sector_count - first_data - 1U);
    Fat32Write32(fsinfo, 492U, 3U);
    fsinfo[510] = 0x55U;
    fsinfo[511] = 0xAAU;

    fat0 = image + (reserved * ORYN_FAT32_SECTOR_SIZE);
    fat1 = fat0 + (fat_size * ORYN_FAT32_SECTOR_SIZE);
    Fat32Write32(fat0, 0U, 0x0FFFFFF8U);
    Fat32Write32(fat0, 4U, 0x0FFFFFFFU);
    Fat32Write32(fat0, 8U, FAT32_EOC);
    OrynMemcpy(fat1, fat0, fat_size * ORYN_FAT32_SECTOR_SIZE);
    (void)first_data;
    return 1;
}
