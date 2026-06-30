#include "KernelIo.h"
#include "KernelConsole.h"

#define SERIAL_COM1 0x3F8
#define QEMU_DEBUG_PORT 0xE9
#define QEMU_EXIT_PORT 0xF4
#define SERIAL_COLOR_RESET "\033[0m"
#define SERIAL_COLOR_KERNEL "\033[36m"
#define SERIAL_COLOR_PASS "\033[32m"
#define SERIAL_COLOR_WARN "\033[33m"
#define SERIAL_COLOR_FAIL "\033[31m"
#define SERIAL_COLOR_STEP "\033[35m"

static int gSerialLineColored = 0;
static int gSerialAtLineStart = 1;

static inline void Out8(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void Out32(unsigned short port, unsigned int value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char In8(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
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
    unsigned int timeout = 1000000U;
    while (((In8(SERIAL_COM1 + 5) & 0x20U) == 0U) && timeout > 0U)
    {
        --timeout;
    }

    if (timeout > 0U)
    {
        Out8(SERIAL_COM1, (unsigned char)value);
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

static const char* SerialColorForLine(const char* text)
{
    if (StartsWith(text, "[KERNEL] PASS"))
    {
        return SERIAL_COLOR_PASS;
    }

    if (StartsWith(text, "[KERNEL] FAIL") || StartsWith(text, "[FAIL]"))
    {
        return SERIAL_COLOR_FAIL;
    }

    if (StartsWith(text, "[KERNEL] WARN") || StartsWith(text, "[WARN]"))
    {
        return SERIAL_COLOR_WARN;
    }

    if (StartsWith(text, "[KERNEL] Virtual memory") || StartsWith(text, "[STEP]"))
    {
        return SERIAL_COLOR_STEP;
    }

    if (StartsWith(text, "[KERNEL]") || StartsWith(text, "[INFO]"))
    {
        return SERIAL_COLOR_KERNEL;
    }

    return "";
}

static void SerialBeginColoredLine(const char* text)
{
    const char* color;

    if (!gSerialAtLineStart)
    {
        return;
    }

    color = SerialColorForLine(text);
    if (color[0] != 0)
    {
        SerialWriteRaw(color);
        gSerialLineColored = 1;
    }
}

static void SerialEndColoredLine(void)
{
    if (gSerialLineColored)
    {
        SerialWriteRaw(SERIAL_COLOR_RESET);
        gSerialLineColored = 0;
    }
}

void KernelIoInit(void)
{
    Out8(SERIAL_COM1 + 1, 0x00);
    Out8(SERIAL_COM1 + 3, 0x80);
    Out8(SERIAL_COM1 + 0, 0x03);
    Out8(SERIAL_COM1 + 1, 0x00);
    Out8(SERIAL_COM1 + 3, 0x03);
    Out8(SERIAL_COM1 + 2, 0xC7);
    Out8(SERIAL_COM1 + 4, 0x0B);
}

void KernelIoWriteChar(char value)
{
    if (value == '\n')
    {
        SerialEndColoredLine();
        Out8(QEMU_DEBUG_PORT, '\r');
        SerialWriteChar('\r');
    }

    Out8(QEMU_DEBUG_PORT, (unsigned char)value);
    SerialWriteChar(value);
    KConsoleWriteChar(value);

    if (value == '\n')
    {
        gSerialAtLineStart = 1;
    }
    else
    {
        gSerialAtLineStart = 0;
    }
}

void KernelIoWriteString(const char* text)
{
    SerialBeginColoredLine(text);

    while (*text != 0)
    {
        KernelIoWriteChar(*text);
        ++text;

        if (gSerialAtLineStart)
        {
            SerialBeginColoredLine(text);
        }
    }
}

void KernelIoWriteHex64(unsigned long long value)
{
    static const char* digits = "0123456789ABCDEF";
    KernelIoWriteString("0x");
    for (int index = 0; index < 16; ++index)
    {
        unsigned int shift = (unsigned int)(60 - (index * 4));
        KernelIoWriteChar(digits[(value >> shift) & 0xFULL]);
    }
}

void KernelIoWriteDec64(unsigned long long value)
{
    char buffer[32];
    int index = 0;

    if (value == 0ULL)
    {
        KernelIoWriteChar('0');
        return;
    }

    while (value != 0ULL && index < (int)sizeof(buffer))
    {
        buffer[index++] = (char)('0' + (value % 10ULL));
        value /= 10ULL;
    }

    while (index > 0)
    {
        KernelIoWriteChar(buffer[--index]);
    }
}

void KernelIoExitQemuSuccess(void)
{
    KernelIoWriteString("[KERNEL] Requesting QEMU debug-exit success.\n");
    Out32(QEMU_EXIT_PORT, 0x10U);
}
