#include "KernelIo.h"
#include "KernelConsole.h"
#include "KernelScreenReport.h"

#define SERIAL_COM1 0x3F8
#define QEMU_DEBUG_PORT 0xE9
#define QEMU_EXIT_PORT 0xF4
#define OUTPUT_COLOR_RESET "\033[0m"
#define OUTPUT_COLOR_KERNEL "\033[36m"
#define OUTPUT_COLOR_PASS "\033[32m"
#define OUTPUT_COLOR_WARN "\033[33m"
#define OUTPUT_COLOR_FAIL "\033[31m"
#define OUTPUT_COLOR_STEP "\033[35m"
#define OUTPUT_COLOR_PCI "\033[96m"

typedef struct KernelIoLineStyle
{
    const char* AnsiColour;
    unsigned int ConsoleColour;
} KernelIoLineStyle;

static int gOutputLineColored = 0;
static int gOutputAtLineStart = 1;

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

static void DebugWriteRaw(const char* text)
{
    while (*text != 0)
    {
        Out8(QEMU_DEBUG_PORT, (unsigned char)*text);
        ++text;
    }
}

static void RawAnsiWrite(const char* text)
{
    DebugWriteRaw(text);
    SerialWriteRaw(text);
}

static KernelIoLineStyle LineStyleForText(const char* text)
{
    KernelIoLineStyle style;
    style.AnsiColour = "";
    style.ConsoleColour = KCONSOLE_COLOUR_DEFAULT;

    if (StartsWith(text, "[KERNEL] PASS") || StartsWith(text, "[PASS]") ||
        StartsWith(text, "[ OK ]") || StartsWith(text, "[OK]"))
    {
        style.AnsiColour = OUTPUT_COLOR_PASS;
        style.ConsoleColour = KCONSOLE_COLOUR_PASS;
        return style;
    }

    if (StartsWith(text, "[KERNEL] FAIL") || StartsWith(text, "[KERNEL] EXCEPTION") ||
        StartsWith(text, "[FAIL]"))
    {
        style.AnsiColour = OUTPUT_COLOR_FAIL;
        style.ConsoleColour = KCONSOLE_COLOUR_FAIL;
        return style;
    }

    if (StartsWith(text, "[KERNEL] WARN") || StartsWith(text, "[WARN]"))
    {
        style.AnsiColour = OUTPUT_COLOR_WARN;
        style.ConsoleColour = KCONSOLE_COLOUR_WARN;
        return style;
    }

    if (StartsWith(text, "[KERNEL] PCI") || StartsWith(text, "[PCI]"))
    {
        style.AnsiColour = OUTPUT_COLOR_PCI;
        style.ConsoleColour = KCONSOLE_COLOUR_PCI;
        return style;
    }

    if (StartsWith(text, "[KERNEL] Virtual memory") || StartsWith(text, "[KERNEL] GDT") ||
        StartsWith(text, "[KERNEL] IDT") || StartsWith(text, "[KERNEL] CPU") ||
        StartsWith(text, "[KERNEL] PIC") || StartsWith(text, "[KERNEL] APIC") ||
        StartsWith(text, "[KERNEL] HPET") || StartsWith(text, "[KERNEL] SMP") || StartsWith(text, "[STEP]"))
    {
        style.AnsiColour = OUTPUT_COLOR_STEP;
        style.ConsoleColour = KCONSOLE_COLOUR_STEP;
        return style;
    }

    if (StartsWith(text, "[KERNEL]") || StartsWith(text, "[INFO]"))
    {
        style.AnsiColour = OUTPUT_COLOR_KERNEL;
        style.ConsoleColour = KCONSOLE_COLOUR_INFO;
        return style;
    }

    return style;
}

static void BeginColoredLine(const char* text)
{
    KernelIoLineStyle style;

    if (!gOutputAtLineStart)
    {
        return;
    }

    style = LineStyleForText(text);
    OrynKernelScreenReportObserve(text);
    KConsoleSetForegroundColour(style.ConsoleColour);
    if (style.AnsiColour[0] != 0)
    {
        RawAnsiWrite(style.AnsiColour);
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

    KConsoleResetForegroundColour();
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
        EndColoredLine();
        Out8(QEMU_DEBUG_PORT, '\r');
        SerialWriteChar('\r');
    }

    Out8(QEMU_DEBUG_PORT, (unsigned char)value);
    SerialWriteChar(value);
    if (value == '\n')
    {
        gOutputAtLineStart = 1;
    }
    else
    {
        gOutputAtLineStart = 0;
    }
}

void KernelIoWriteString(const char* text)
{
    char normalizedStatusLine[384];
    if (gOutputAtLineStart &&
        OrynKernelScreenReportNormalizeStatusLine(text, normalizedStatusLine, sizeof(normalizedStatusLine)))
    {
        text = normalizedStatusLine;
    }

    BeginColoredLine(text);

    while (*text != 0)
    {
        KernelIoWriteChar(*text);
        ++text;

        if (gOutputAtLineStart)
        {
            BeginColoredLine(text);
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

void KernelIoExitQemuFailure(void)
{
    KernelIoWriteString("[KERNEL] Requesting QEMU debug-exit failure.\n");
    Out32(QEMU_EXIT_PORT, 0x11U);
}
