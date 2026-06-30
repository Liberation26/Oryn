#ifndef ORYN_KERNEL_TTF_H
#define ORYN_KERNEL_TTF_H

#include "KernelBootInfo.h"

#define ORYN_TTF_MAX_POINTS 512U
#define ORYN_TTF_MAX_CONTOURS 64U
#define ORYN_TTF_MAX_INTERSECTIONS 128U

typedef struct OrynTtfFont
{
    const unsigned char* Data;
    unsigned int Size;
    unsigned int Loaded;
    unsigned int UnitsPerEm;
    unsigned int NumGlyphs;
    unsigned int CmapOffset;
    unsigned int CmapLength;
    unsigned int GlyfOffset;
    unsigned int GlyfLength;
    unsigned int HeadOffset;
    unsigned int HeadLength;
    unsigned int HheaOffset;
    unsigned int HheaLength;
    unsigned int HmtxOffset;
    unsigned int HmtxLength;
    unsigned int LocaOffset;
    unsigned int LocaLength;
    unsigned int MaxpOffset;
    unsigned int MaxpLength;
    unsigned short NumberOfHMetrics;
    short IndexToLocFormat;
} OrynTtfFont;

static inline unsigned short OrynTtfReadU16(const unsigned char* data, unsigned int offset)
{
    return (unsigned short)(((unsigned short)data[offset] << 8) | (unsigned short)data[offset + 1U]);
}

static inline short OrynTtfReadS16(const unsigned char* data, unsigned int offset)
{
    return (short)OrynTtfReadU16(data, offset);
}

static inline unsigned int OrynTtfReadU32(const unsigned char* data, unsigned int offset)
{
    return ((unsigned int)data[offset] << 24) |
        ((unsigned int)data[offset + 1U] << 16) |
        ((unsigned int)data[offset + 2U] << 8) |
        (unsigned int)data[offset + 3U];
}

int OrynTtfLoadFromBootInfo(const OrynBootInfo* bootInfo, OrynTtfFont* font);
int OrynTtfIsLoaded(const OrynTtfFont* font);
unsigned short OrynTtfGlyphIndexForChar(const OrynTtfFont* font, unsigned int codepoint);
unsigned int OrynTtfRenderGlyph(
    const OrynTtfFont* font,
    unsigned short glyphIndex,
    volatile unsigned int* framebuffer,
    unsigned long long framebufferSize,
    unsigned int width,
    unsigned int height,
    unsigned int pitch,
    unsigned int originX,
    unsigned int baselineY,
    unsigned int pixelHeight,
    unsigned int colour);
unsigned int OrynTtfRenderAsciiGlyph(
    const OrynTtfFont* font,
    char value,
    volatile unsigned int* framebuffer,
    unsigned long long framebufferSize,
    unsigned int width,
    unsigned int height,
    unsigned int pitch,
    unsigned int originX,
    unsigned int originY,
    unsigned int pixelHeight,
    unsigned int colour);

#endif
