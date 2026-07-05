#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

static void OrynLibCSerialWriteByte(char value)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)value), "Nd"((unsigned short)0x3F8));
#else
    (void)value;
#endif
}

static void OrynLibCSerialWriteString(const char* text)
{
    if (text == 0)
    {
        return;
    }

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            OrynLibCSerialWriteByte('\r');
        }

        OrynLibCSerialWriteByte(*text);
        ++text;
    }
}

int vprintf(const char* restrict format, va_list args)
{
    char buffer[1024];
    int result;

    result = vsnprintf(buffer, sizeof(buffer), format, args);
    if (result < 0)
    {
        return result;
    }

    OrynLibCSerialWriteString(buffer);
    return result;
}
