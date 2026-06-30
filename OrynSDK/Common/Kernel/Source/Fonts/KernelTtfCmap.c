#include "KernelTtf.h"

static int CmapRangeOk(const OrynTtfFont* font, unsigned int offset, unsigned int length)
{
    if (offset < font->CmapOffset || offset > font->CmapOffset + font->CmapLength)
    {
        return 0;
    }

    if (length > (font->CmapOffset + font->CmapLength) - offset)
    {
        return 0;
    }

    return 1;
}

static unsigned short LookupFormat4(const OrynTtfFont* font, unsigned int subtable, unsigned int codepoint)
{
    const unsigned char* data = font->Data;
    if (!CmapRangeOk(font, subtable, 16U) || OrynTtfReadU16(data, subtable) != 4U)
    {
        return 0U;
    }

    unsigned int length = OrynTtfReadU16(data, subtable + 2U);
    unsigned int segCount = OrynTtfReadU16(data, subtable + 6U) / 2U;
    if (!CmapRangeOk(font, subtable, length) || segCount == 0U)
    {
        return 0U;
    }

    unsigned int endCode = subtable + 14U;
    unsigned int startCode = endCode + (segCount * 2U) + 2U;
    unsigned int idDelta = startCode + (segCount * 2U);
    unsigned int idRangeOffset = idDelta + (segCount * 2U);

    for (unsigned int index = 0U; index < segCount; ++index)
    {
        unsigned int end = OrynTtfReadU16(data, endCode + (index * 2U));
        unsigned int start = OrynTtfReadU16(data, startCode + (index * 2U));
        if (codepoint < start || codepoint > end)
        {
            continue;
        }

        unsigned int rangeOffsetAddress = idRangeOffset + (index * 2U);
        unsigned int rangeOffset = OrynTtfReadU16(data, rangeOffsetAddress);
        int delta = (int)OrynTtfReadS16(data, idDelta + (index * 2U));
        if (rangeOffset == 0U)
        {
            return (unsigned short)((codepoint + (unsigned int)delta) & 0xFFFFU);
        }

        unsigned int glyphAddress = rangeOffsetAddress + rangeOffset + ((codepoint - start) * 2U);
        if (!CmapRangeOk(font, glyphAddress, 2U))
        {
            return 0U;
        }

        unsigned int glyph = OrynTtfReadU16(data, glyphAddress);
        if (glyph == 0U)
        {
            return 0U;
        }

        return (unsigned short)((glyph + (unsigned int)delta) & 0xFFFFU);
    }

    return 0U;
}

unsigned short OrynTtfGlyphIndexForChar(const OrynTtfFont* font, unsigned int codepoint)
{
    if (!OrynTtfIsLoaded(font) || font->CmapLength < 4U)
    {
        return 0U;
    }

    const unsigned char* data = font->Data;
    unsigned int tableCount = OrynTtfReadU16(data, font->CmapOffset + 2U);
    unsigned int best = 0U;
    for (unsigned int index = 0U; index < tableCount; ++index)
    {
        unsigned int record = font->CmapOffset + 4U + (index * 8U);
        if (!CmapRangeOk(font, record, 8U))
        {
            continue;
        }

        unsigned int platform = OrynTtfReadU16(data, record);
        unsigned int encoding = OrynTtfReadU16(data, record + 2U);
        unsigned int offset = font->CmapOffset + OrynTtfReadU32(data, record + 4U);
        if (CmapRangeOk(font, offset, 2U) && OrynTtfReadU16(data, offset) == 4U)
        {
            if ((platform == 3U && (encoding == 1U || encoding == 10U)) || platform == 0U)
            {
                best = offset;
                break;
            }
            best = offset;
        }
    }

    return best == 0U ? 0U : LookupFormat4(font, best, codepoint);
}
