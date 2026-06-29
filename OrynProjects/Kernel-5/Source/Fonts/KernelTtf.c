#include "KernelTtf.h"

static void ClearFont(OrynTtfFont* font)
{
    unsigned char* bytes = (unsigned char*)font;
    for (unsigned int index = 0U; index < sizeof(*font); ++index)
    {
        bytes[index] = 0U;
    }
}

static int RangeOk(const OrynTtfFont* font, unsigned int offset, unsigned int length)
{
    if (offset > font->Size)
    {
        return 0;
    }

    if (length > font->Size - offset)
    {
        return 0;
    }

    return 1;
}

static int TagEquals(const unsigned char* data, unsigned int offset, const char* tag)
{
    return data[offset] == (unsigned char)tag[0] &&
        data[offset + 1U] == (unsigned char)tag[1] &&
        data[offset + 2U] == (unsigned char)tag[2] &&
        data[offset + 3U] == (unsigned char)tag[3];
}

static void StoreTable(OrynTtfFont* font, const char* tag, unsigned int offset, unsigned int length)
{
    if (tag[0] == 'c' && tag[1] == 'm' && tag[2] == 'a' && tag[3] == 'p')
    {
        font->CmapOffset = offset;
        font->CmapLength = length;
    }
    else if (tag[0] == 'g' && tag[1] == 'l' && tag[2] == 'y' && tag[3] == 'f')
    {
        font->GlyfOffset = offset;
        font->GlyfLength = length;
    }
    else if (tag[0] == 'h' && tag[1] == 'e' && tag[2] == 'a' && tag[3] == 'd')
    {
        font->HeadOffset = offset;
        font->HeadLength = length;
    }
    else if (tag[0] == 'h' && tag[1] == 'h' && tag[2] == 'e' && tag[3] == 'a')
    {
        font->HheaOffset = offset;
        font->HheaLength = length;
    }
    else if (tag[0] == 'h' && tag[1] == 'm' && tag[2] == 't' && tag[3] == 'x')
    {
        font->HmtxOffset = offset;
        font->HmtxLength = length;
    }
    else if (tag[0] == 'l' && tag[1] == 'o' && tag[2] == 'c' && tag[3] == 'a')
    {
        font->LocaOffset = offset;
        font->LocaLength = length;
    }
    else if (tag[0] == 'm' && tag[1] == 'a' && tag[2] == 'x' && tag[3] == 'p')
    {
        font->MaxpOffset = offset;
        font->MaxpLength = length;
    }
}

static int ReadTableDirectory(OrynTtfFont* font)
{
    if (!RangeOk(font, 0U, 12U))
    {
        return 0;
    }

    unsigned int scaler = OrynTtfReadU32(font->Data, 0U);
    if (scaler != 0x00010000U && scaler != 0x74727565U)
    {
        return 0;
    }

    unsigned short tableCount = OrynTtfReadU16(font->Data, 4U);
    if (!RangeOk(font, 12U, (unsigned int)tableCount * 16U))
    {
        return 0;
    }

    for (unsigned int index = 0U; index < tableCount; ++index)
    {
        unsigned int entry = 12U + (index * 16U);
        unsigned int offset = OrynTtfReadU32(font->Data, entry + 8U);
        unsigned int length = OrynTtfReadU32(font->Data, entry + 12U);
        if (!RangeOk(font, offset, length))
        {
            continue;
        }

        if (TagEquals(font->Data, entry, "cmap")) { StoreTable(font, "cmap", offset, length); }
        if (TagEquals(font->Data, entry, "glyf")) { StoreTable(font, "glyf", offset, length); }
        if (TagEquals(font->Data, entry, "head")) { StoreTable(font, "head", offset, length); }
        if (TagEquals(font->Data, entry, "hhea")) { StoreTable(font, "hhea", offset, length); }
        if (TagEquals(font->Data, entry, "hmtx")) { StoreTable(font, "hmtx", offset, length); }
        if (TagEquals(font->Data, entry, "loca")) { StoreTable(font, "loca", offset, length); }
        if (TagEquals(font->Data, entry, "maxp")) { StoreTable(font, "maxp", offset, length); }
    }

    return font->CmapOffset != 0U && font->GlyfOffset != 0U && font->HeadOffset != 0U &&
        font->HheaOffset != 0U && font->HmtxOffset != 0U && font->LocaOffset != 0U &&
        font->MaxpOffset != 0U;
}

int OrynTtfLoadFromBootInfo(const OrynBootInfo* bootInfo, OrynTtfFont* font)
{
    if (font == 0)
    {
        return 0;
    }

    ClearFont(font);
    if (bootInfo == 0 || !KernelBootInfoHasFlag(bootInfo, ORYN_BOOTINFO_FLAG_FONT))
    {
        return 0;
    }

    if (bootInfo->FontBase == 0ULL || bootInfo->FontSize < 64ULL || bootInfo->FontSize > 8ULL * 1024ULL * 1024ULL)
    {
        return 0;
    }

    font->Data = (const unsigned char*)(unsigned long long)bootInfo->FontBase;
    font->Size = (unsigned int)bootInfo->FontSize;
    if (!ReadTableDirectory(font))
    {
        ClearFont(font);
        return 0;
    }

    font->UnitsPerEm = OrynTtfReadU16(font->Data, font->HeadOffset + 18U);
    font->IndexToLocFormat = OrynTtfReadS16(font->Data, font->HeadOffset + 50U);
    font->NumGlyphs = OrynTtfReadU16(font->Data, font->MaxpOffset + 4U);
    font->NumberOfHMetrics = OrynTtfReadU16(font->Data, font->HheaOffset + 34U);
    if (font->UnitsPerEm == 0U || font->NumGlyphs == 0U)
    {
        ClearFont(font);
        return 0;
    }

    font->Loaded = 1U;
    return 1;
}

int OrynTtfIsLoaded(const OrynTtfFont* font)
{
    return font != 0 && font->Loaded != 0U;
}
