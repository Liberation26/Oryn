#include "stdarg.h"
#include "stddef.h"
#include "stdio.h"

#define ORYN_PRINTF_BUFFER_SIZE 1024U

static inline void OrynPrintfOut8(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void OrynPrintfWriteChar(char value)
{
    if (value == '\n')
    {
        OrynPrintfOut8(0x3F8U, (unsigned char)'\r');
    }

    OrynPrintfOut8(0x3F8U, (unsigned char)value);
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

    result = vsnprintf(buffer, sizeof(buffer), format, args);
    if (result < 0)
    {
        return result;
    }

    OrynPrintfWriteText(buffer);
    return result;
}
