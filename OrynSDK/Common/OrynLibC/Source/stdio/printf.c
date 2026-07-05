#include <stdarg.h>
#include <stdio.h>

static void OrynSerialWriteByte(char value)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)value), "Nd"((unsigned short)0x3F8));
#else
    (void)value;
#endif
}

static void OrynSerialWriteText(const char* text)
{
    const char* current = text;

    if (current == 0)
    {
        return;
    }

    while (*current != '\0')
    {
        if (*current == '\n')
        {
            OrynSerialWriteByte('\r');
        }

        OrynSerialWriteByte(*current);
        ++current;
    }
}

int vprintf(const char* restrict format, va_list args)
{
    char buffer[1024];
    int result = vsnprintf(buffer, sizeof(buffer), format, args);

    if (result >= 0)
    {
        OrynSerialWriteText(buffer);
    }

    return result;
}

int printf(const char* restrict format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);

    return result;
}
