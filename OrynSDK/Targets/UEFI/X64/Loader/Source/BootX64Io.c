#include "BootX64Internal.h"

#define OUTPUT_COLOR_RESET "\033[0m"
#define OUTPUT_COLOR_BOOT "\033[36m"
#define OUTPUT_COLOR_PASS "\033[32m"
#define OUTPUT_COLOR_WARN "\033[33m"
#define OUTPUT_COLOR_FAIL "\033[31m"
#define OUTPUT_COLOR_STEP "\033[35m"

static int gOutputLineColored = 0;
static int gOutputAtLineStart = 1;

int IsError(EFI_STATUS status)
{
    return (status & EFI_ERROR_BIT) != 0;
}

static inline void Out8(UINT16 port, UINT8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline UINT8 In8(UINT16 port)
{
    UINT8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void InitSerialDebug(void)
{
    Out8(SERIAL_COM1 + 1U, 0x00U);
    Out8(SERIAL_COM1 + 3U, 0x80U);
    Out8(SERIAL_COM1 + 0U, 0x03U);
    Out8(SERIAL_COM1 + 1U, 0x00U);
    Out8(SERIAL_COM1 + 3U, 0x03U);
    Out8(SERIAL_COM1 + 2U, 0xC7U);
    Out8(SERIAL_COM1 + 4U, 0x0BU);
}

static int StartsWith(const char* text, const char* prefix)
{
    while (*prefix != 0)
    {
        if (*text != *prefix)
        {
            return 0;
        }

        ++text;
        ++prefix;
    }

    return 1;
}

static void SerialWriteChar(char value)
{
    UINT32 timeout = 1000000U;
    while (((In8(SERIAL_COM1 + 5U) & 0x20U) == 0U) && timeout > 0U)
    {
        --timeout;
    }

    if (timeout > 0U)
    {
        Out8(SERIAL_COM1, (UINT8)value);
    }
}

static void SerialWriteRaw(const char* text)
{
    while (*text != 0)
    {
        SerialWriteChar(*text);
        ++text;
    }
}

static void DebugWriteChar(char value)
{
    Out8(QEMU_DEBUG_PORT, (UINT8)value);
}

static void DebugWriteRaw(const char* text)
{
    while (*text != 0)
    {
        DebugWriteChar(*text);
        ++text;
    }
}

static void RawAnsiWrite(const char* text)
{
    DebugWriteRaw(text);
    SerialWriteRaw(text);
}

static const char* SerialColorForLine(const char* text)
{
    if (StartsWith(text, "[BOOT] PASS"))
    {
        return OUTPUT_COLOR_PASS;
    }

    if (StartsWith(text, "[BOOT] FAIL") || StartsWith(text, "[FAIL]"))
    {
        return OUTPUT_COLOR_FAIL;
    }

    if (StartsWith(text, "[BOOT] WARN") || StartsWith(text, "[WARN]"))
    {
        return OUTPUT_COLOR_WARN;
    }

    if (StartsWith(text, "[BOOT] Stage"))
    {
        return OUTPUT_COLOR_STEP;
    }

    if (StartsWith(text, "[BOOT]") || StartsWith(text, "[INFO]"))
    {
        return OUTPUT_COLOR_BOOT;
    }

    return "";
}

static void BeginColoredLine(const char* text)
{
    const char* color;

    if (!gOutputAtLineStart)
    {
        return;
    }

    color = SerialColorForLine(text);
    if (color[0] != 0)
    {
        RawAnsiWrite(color);
        gOutputLineColored = 1;
    }
}

static void EndColoredLine(void)
{
    if (gOutputLineColored)
    {
        RawAnsiWrite(OUTPUT_COLOR_RESET);
        gOutputLineColored = 0;
    }
}

void Print(const char* text)
{
    BeginColoredLine(text);

    while (*text != 0)
    {
        if (*text == '\n')
        {
            EndColoredLine();
            DebugWriteChar('\r');
            SerialWriteChar('\r');
            DebugWriteChar('\n');
            SerialWriteChar('\n');
            gOutputAtLineStart = 1;
            ++text;
            BeginColoredLine(text);
            continue;
        }

        DebugWriteChar(*text);
        SerialWriteChar(*text);
        gOutputAtLineStart = 0;
        ++text;
    }
}

void PrintHex64(UINT64 value)
{
    static const char* digits = "0123456789ABCDEF";
    char buffer[19];
    buffer[0] = '0';
    buffer[1] = 'x';
    for (int index = 0; index < 16; ++index)
    {
        UINT64 shift = (UINT64)(60 - (index * 4));
        buffer[2 + index] = digits[(value >> shift) & 0xFULL];
    }
    buffer[18] = 0;
    Print(buffer);
}

void SetMemory(void* target, UINT8 value, UINTN size)
{
    UINT8* out = (UINT8*)target;
    for (UINTN index = 0; index < size; ++index)
    {
        out[index] = value;
    }
}

void CopyMemory(void* target, const void* source, UINTN size)
{
    UINT8* out = (UINT8*)target;
    const UINT8* in = (const UINT8*)source;
    for (UINTN index = 0; index < size; ++index)
    {
        out[index] = in[index];
    }
}
