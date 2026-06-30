#include "KernelFat32Internal.h"

uint16_t Fat32Read16(const uint8_t* data, uint32_t offset)
{
    return (uint16_t)data[offset] | ((uint16_t)data[offset + 1U] << 8U);
}

uint32_t Fat32Read32(const uint8_t* data, uint32_t offset)
{
    return (uint32_t)data[offset] |
        ((uint32_t)data[offset + 1U] << 8U) |
        ((uint32_t)data[offset + 2U] << 16U) |
        ((uint32_t)data[offset + 3U] << 24U);
}

void Fat32Write16(uint8_t* data, uint32_t offset, uint16_t value)
{
    data[offset] = (uint8_t)(value & 0xFFU);
    data[offset + 1U] = (uint8_t)((value >> 8U) & 0xFFU);
}

void Fat32Write32(uint8_t* data, uint32_t offset, uint32_t value)
{
    data[offset] = (uint8_t)(value & 0xFFU);
    data[offset + 1U] = (uint8_t)((value >> 8U) & 0xFFU);
    data[offset + 2U] = (uint8_t)((value >> 16U) & 0xFFU);
    data[offset + 3U] = (uint8_t)((value >> 24U) & 0xFFU);
}
