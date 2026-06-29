#include "KernelTtf.h"

typedef struct OrynTtfGlyphWork
{
    short X[ORYN_TTF_MAX_POINTS];
    short Y[ORYN_TTF_MAX_POINTS];
    unsigned char Flags[ORYN_TTF_MAX_POINTS];
    unsigned short Ends[ORYN_TTF_MAX_CONTOURS];
    int PixelX[ORYN_TTF_MAX_POINTS];
    int PixelY[ORYN_TTF_MAX_POINTS];
    unsigned int PointCount;
    unsigned int ContourCount;
    short XMin;
    short YMin;
    short XMax;
    short YMax;
} OrynTtfGlyphWork;

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

static unsigned int GlyphOffset(const OrynTtfFont* font, unsigned int glyphIndex)
{
    if (font->IndexToLocFormat == 0)
    {
        unsigned int offset = font->LocaOffset + (glyphIndex * 2U);
        if (!RangeOk(font, offset, 2U)) { return 0U; }
        return font->GlyfOffset + ((unsigned int)OrynTtfReadU16(font->Data, offset) * 2U);
    }

    unsigned int offset = font->LocaOffset + (glyphIndex * 4U);
    if (!RangeOk(font, offset, 4U)) { return 0U; }
    return font->GlyfOffset + OrynTtfReadU32(font->Data, offset);
}

static unsigned int GlyphAdvance(const OrynTtfFont* font, unsigned int glyphIndex, unsigned int pixelHeight)
{
    unsigned int metricIndex = glyphIndex;
    if (metricIndex >= font->NumberOfHMetrics && font->NumberOfHMetrics != 0U)
    {
        metricIndex = font->NumberOfHMetrics - 1U;
    }

    unsigned int metric = font->HmtxOffset + (metricIndex * 4U);
    if (!RangeOk(font, metric, 2U) || font->UnitsPerEm == 0U)
    {
        return pixelHeight / 2U;
    }

    unsigned int advance = OrynTtfReadU16(font->Data, metric);
    unsigned int pixels = (advance * pixelHeight) / font->UnitsPerEm;
    return pixels == 0U ? pixelHeight / 2U : pixels;
}

static int ReadSimpleGlyphHeader(const OrynTtfFont* font, unsigned int glyph, OrynTtfGlyphWork* work)
{
    unsigned int glyphEnd = GlyphOffset(font, glyph + 1U);
    unsigned int glyphStart = GlyphOffset(font, glyph);
    if (glyphStart == 0U || glyphEnd <= glyphStart || !RangeOk(font, glyphStart, 10U))
    {
        return 0;
    }

    short contourCount = OrynTtfReadS16(font->Data, glyphStart);
    if (contourCount <= 0 || contourCount > (short)ORYN_TTF_MAX_CONTOURS)
    {
        return 0;
    }

    work->ContourCount = (unsigned int)contourCount;
    work->XMin = OrynTtfReadS16(font->Data, glyphStart + 2U);
    work->YMin = OrynTtfReadS16(font->Data, glyphStart + 4U);
    work->XMax = OrynTtfReadS16(font->Data, glyphStart + 6U);
    work->YMax = OrynTtfReadS16(font->Data, glyphStart + 8U);

    unsigned int ends = glyphStart + 10U;
    for (unsigned int index = 0U; index < work->ContourCount; ++index)
    {
        work->Ends[index] = OrynTtfReadU16(font->Data, ends + (index * 2U));
    }

    work->PointCount = (unsigned int)work->Ends[work->ContourCount - 1U] + 1U;
    if (work->PointCount > ORYN_TTF_MAX_POINTS)
    {
        return 0;
    }

    return 1;
}

static int ReadFlags(const OrynTtfFont* font, unsigned int* cursor, OrynTtfGlyphWork* work)
{
    unsigned int point = 0U;
    while (point < work->PointCount)
    {
        if (!RangeOk(font, *cursor, 1U)) { return 0; }
        unsigned char flag = font->Data[*cursor];
        *cursor += 1U;
        work->Flags[point++] = flag;
        if ((flag & 0x08U) != 0U)
        {
            if (!RangeOk(font, *cursor, 1U)) { return 0; }
            unsigned int repeat = font->Data[*cursor];
            *cursor += 1U;
            for (unsigned int copy = 0U; copy < repeat && point < work->PointCount; ++copy)
            {
                work->Flags[point++] = flag;
            }
        }
    }

    return 1;
}

static int ReadCoordinates(const OrynTtfFont* font, unsigned int* cursor, OrynTtfGlyphWork* work, int yAxis)
{
    int value = 0;
    for (unsigned int index = 0U; index < work->PointCount; ++index)
    {
        unsigned char flag = work->Flags[index];
        unsigned char shortFlag = yAxis ? 0x04U : 0x02U;
        unsigned char sameFlag = yAxis ? 0x20U : 0x10U;
        int delta = 0;
        if ((flag & shortFlag) != 0U)
        {
            if (!RangeOk(font, *cursor, 1U)) { return 0; }
            delta = font->Data[*cursor];
            *cursor += 1U;
            if ((flag & sameFlag) == 0U) { delta = -delta; }
        }
        else if ((flag & sameFlag) == 0U)
        {
            if (!RangeOk(font, *cursor, 2U)) { return 0; }
            delta = OrynTtfReadS16(font->Data, *cursor);
            *cursor += 2U;
        }

        value += delta;
        if (yAxis) { work->Y[index] = (short)value; }
        else { work->X[index] = (short)value; }
    }

    return 1;
}

static int LoadGlyph(const OrynTtfFont* font, unsigned int glyph, OrynTtfGlyphWork* work)
{
    unsigned int glyphStart = GlyphOffset(font, glyph);
    if (!ReadSimpleGlyphHeader(font, glyph, work))
    {
        return 0;
    }

    unsigned int cursor = glyphStart + 10U + (work->ContourCount * 2U);
    if (!RangeOk(font, cursor, 2U)) { return 0; }
    unsigned int instructionLength = OrynTtfReadU16(font->Data, cursor);
    cursor += 2U + instructionLength;
    return ReadFlags(font, &cursor, work) &&
        ReadCoordinates(font, &cursor, work, 0) &&
        ReadCoordinates(font, &cursor, work, 1);
}

static void PutPixel(
    volatile unsigned int* framebuffer,
    unsigned long long framebufferSize,
    unsigned int width,
    unsigned int height,
    unsigned int pitch,
    int x,
    int y,
    unsigned int colour)
{
    if (x < 0 || y < 0 || (unsigned int)x >= width || (unsigned int)y >= height)
    {
        return;
    }

    unsigned long long index = ((unsigned long long)(unsigned int)y * pitch) + (unsigned int)x;
    if ((index * 4ULL) + 3ULL >= framebufferSize)
    {
        return;
    }

    framebuffer[index] = colour;
}

static void TransformGlyph(OrynTtfGlyphWork* work, unsigned int originX, unsigned int originY, unsigned int pixelHeight)
{
    int glyphHeight = (int)work->YMax - (int)work->YMin;
    if (glyphHeight <= 0) { glyphHeight = 1; }
    for (unsigned int index = 0U; index < work->PointCount; ++index)
    {
        work->PixelX[index] = (int)originX + (((int)work->X[index] - (int)work->XMin) * (int)pixelHeight) / glyphHeight;
        work->PixelY[index] = (int)originY + (((int)work->YMax - (int)work->Y[index]) * (int)pixelHeight) / glyphHeight;
    }
}

static void SortInts(int* values, unsigned int count)
{
    for (unsigned int i = 1U; i < count; ++i)
    {
        int value = values[i];
        unsigned int j = i;
        while (j > 0U && values[j - 1U] > value)
        {
            values[j] = values[j - 1U];
            --j;
        }
        values[j] = value;
    }
}

static void FillContourScanline(const OrynTtfGlyphWork* work, unsigned int first, unsigned int last, int y, int* xs, unsigned int* xCount)
{
    for (unsigned int point = first; point <= last; ++point)
    {
        unsigned int next = point == last ? first : point + 1U;
        int x1 = work->PixelX[point];
        int y1 = work->PixelY[point];
        int x2 = work->PixelX[next];
        int y2 = work->PixelY[next];
        if (y1 == y2) { continue; }
        if (!((y >= y1 && y < y2) || (y >= y2 && y < y1))) { continue; }
        if (*xCount >= ORYN_TTF_MAX_INTERSECTIONS) { return; }
        xs[*xCount] = x1 + ((y - y1) * (x2 - x1)) / (y2 - y1);
        *xCount += 1U;
    }
}

static void RasterGlyph(
    const OrynTtfGlyphWork* source,
    volatile unsigned int* framebuffer,
    unsigned long long framebufferSize,
    unsigned int width,
    unsigned int height,
    unsigned int pitch,
    unsigned int colour)
{
    const OrynTtfGlyphWork* work = source;
    int top = 2147483647;
    int bottom = -2147483647;
    for (unsigned int index = 0U; index < work->PointCount; ++index)
    {
        if (work->PixelY[index] < top) { top = work->PixelY[index]; }
        if (work->PixelY[index] > bottom) { bottom = work->PixelY[index]; }
    }

    for (int y = top; y <= bottom; ++y)
    {
        int xs[ORYN_TTF_MAX_INTERSECTIONS];
        unsigned int xCount = 0U;
        unsigned int first = 0U;
        for (unsigned int contour = 0U; contour < work->ContourCount; ++contour)
        {
            unsigned int last = work->Ends[contour];
            FillContourScanline(work, first, last, y, xs, &xCount);
            first = last + 1U;
        }

        SortInts(xs, xCount);
        for (unsigned int index = 0U; index + 1U < xCount; index += 2U)
        {
            for (int x = xs[index]; x <= xs[index + 1U]; ++x)
            {
                PutPixel(framebuffer, framebufferSize, width, height, pitch, x, y, colour);
            }
        }
    }
}

unsigned int OrynTtfRenderGlyph(
    const OrynTtfFont* font,
    unsigned short glyphIndex,
    volatile unsigned int* framebuffer,
    unsigned long long framebufferSize,
    unsigned int width,
    unsigned int height,
    unsigned int pitch,
    unsigned int originX,
    unsigned int originY,
    unsigned int pixelHeight,
    unsigned int colour)
{
    OrynTtfGlyphWork work;
    if (!OrynTtfIsLoaded(font) || glyphIndex == 0U || glyphIndex >= font->NumGlyphs)
    {
        return 0U;
    }

    if (!LoadGlyph(font, glyphIndex, &work))
    {
        return 0U;
    }

    TransformGlyph(&work, originX, originY, pixelHeight);
    RasterGlyph(&work, framebuffer, framebufferSize, width, height, pitch, colour);
    return GlyphAdvance(font, glyphIndex, pixelHeight);
}

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
    unsigned int colour)
{
    unsigned int codepoint = (unsigned char)value;
    unsigned short glyph = OrynTtfGlyphIndexForChar(font, codepoint);
    return OrynTtfRenderGlyph(font, glyph, framebuffer, framebufferSize, width, height, pitch,
        originX, originY, pixelHeight, colour);
}
