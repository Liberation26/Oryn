#include "stdarg.h"
#include "stddef.h"
#include "stdio.h"

#define ORYN_PRINTF_BUFFER_SIZE 1024U
#define ORYN_PRINTF_SERIAL_COM1 0x3F8U
#define ORYN_PRINTF_QEMU_DEBUG_PORT 0xE9U

static inline void OrynPrintfOut8(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char OrynPrintfIn8(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void OrynPrintfSerialInit(void)
{
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 1U, 0x00U);
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 3U, 0x80U);
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 0U, 0x03U);
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 1U, 0x00U);
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 3U, 0x03U);
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 2U, 0xC7U);
    OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1 + 4U, 0x0BU);
}

static void OrynPrintfSerialWriteChar(char value)
{
    unsigned int timeout = 1000000U;

    while (((OrynPrintfIn8(ORYN_PRINTF_SERIAL_COM1 + 5U) & 0x20U) == 0U) && timeout > 0U)
    {
        --timeout;
    }

    if (timeout > 0U)
    {
        OrynPrintfOut8(ORYN_PRINTF_SERIAL_COM1, (unsigned char)value);
    }
}

static void OrynPrintfWriteChar(char value)
{
    if (value == '\n')
    {
        OrynPrintfOut8(ORYN_PRINTF_QEMU_DEBUG_PORT, (unsigned char)'\r');
        OrynPrintfSerialWriteChar('\r');
    }

    OrynPrintfOut8(ORYN_PRINTF_QEMU_DEBUG_PORT, (unsigned char)value);
    OrynPrintfSerialWriteChar(value);
}

static void OrynPrintfWriteText(const char* text)
{
    if (text == 0)
    {
        return;
    }

    while (*text != 0)
    {
        OrynPrintfWriteChar(*text);
        ++text;
    }
}

int vprintf(const char* restrict format, va_list args)
{
    char buffer[ORYN_PRINTF_BUFFER_SIZE];
    int result;

    OrynPrintfSerialInit();

    result = vsnprintf(buffer, sizeof(buffer), format, args);
    if (result < 0)
    {
        return result;
    }

    OrynPrintfWriteText(buffer);
    return result;
}
