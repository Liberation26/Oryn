#ifndef ORYN_LIBC_STDIO_H
#define ORYN_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

int snprintf(char* restrict target, size_t size, const char* restrict format, ...);

static inline void OrynLibCPrintfOutByte(unsigned char value)
{
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"((unsigned short)0x3F8));
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"((unsigned short)0xE9));
#else
    (void)value;
#endif
}

static inline void OrynLibCPrintfSerialInit(void)
{
#if defined(__x86_64__) || defined(__i386__)
    static int initialized = 0;

    if (initialized)
    {
        return;
    }

    initialized = 1;
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x00), "Nd"((unsigned short)0x3F9));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x80), "Nd"((unsigned short)0x3FB));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x03), "Nd"((unsigned short)0x3F8));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x00), "Nd"((unsigned short)0x3F9));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x03), "Nd"((unsigned short)0x3FB));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0xC7), "Nd"((unsigned short)0x3FA));
    __asm__ __volatile__("outb %0, %1" : : "a"((unsigned char)0x0B), "Nd"((unsigned short)0x3FC));
#endif
}

static inline int OrynLibCPrintfPutChar(int ch)
{
    OrynLibCPrintfSerialInit();

    if (ch == '\n')
    {
        OrynLibCPrintfOutByte((unsigned char)'\r');
    }

    OrynLibCPrintfOutByte((unsigned char)ch);
    return 1;
}

static inline int OrynLibCPrintfWriteText(const char* text)
{
    int written = 0;

    if (text == 0)
    {
        text = "(null)";
    }

    while (*text != '\0')
    {
        written += OrynLibCPrintfPutChar((unsigned char)*text);
        ++text;
    }

    return written;
}

static inline int OrynLibCPrintfWriteUnsigned(unsigned long long value, unsigned int base, int uppercase)
{
    char buffer[32];
    unsigned int index = 0U;
    int written = 0;
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (base < 2U || base > 16U)
    {
        return 0;
    }

    if (value == 0ULL)
    {
        return OrynLibCPrintfPutChar('0');
    }

    while (value != 0ULL && index < (unsigned int)sizeof(buffer))
    {
        buffer[index++] = digits[value % base];
        value /= base;
    }

    while (index > 0U)
    {
        --index;
        written += OrynLibCPrintfPutChar(buffer[index]);
    }

    return written;
}

static inline int OrynLibCPrintfWriteSigned(long long value)
{
    if (value < 0LL)
    {
        int written = OrynLibCPrintfPutChar('-');
        unsigned long long magnitude = (unsigned long long)(-(value + 1LL)) + 1ULL;
        written += OrynLibCPrintfWriteUnsigned(magnitude, 10U, 0);
        return written;
    }

    return OrynLibCPrintfWriteUnsigned((unsigned long long)value, 10U, 0);
}

static inline int vprintf(const char* restrict format, va_list args)
{
    int written = 0;

    if (format == 0)
    {
        return OrynLibCPrintfWriteText("(null)");
    }

    while (*format != '\0')
    {
        if (*format != '%')
        {
            written += OrynLibCPrintfPutChar((unsigned char)*format);
            ++format;
            continue;
        }

        ++format;

        if (*format == '%')
        {
            written += OrynLibCPrintfPutChar('%');
        }
        else if (*format == 's')
        {
            written += OrynLibCPrintfWriteText(va_arg(args, const char*));
        }
        else if (*format == 'c')
        {
            written += OrynLibCPrintfPutChar(va_arg(args, int));
        }
        else if (*format == 'd' || *format == 'i')
        {
            written += OrynLibCPrintfWriteSigned((long long)va_arg(args, int));
        }
        else if (*format == 'u')
        {
            written += OrynLibCPrintfWriteUnsigned((unsigned long long)va_arg(args, unsigned int), 10U, 0);
        }
        else if (*format == 'x')
        {
            written += OrynLibCPrintfWriteUnsigned((unsigned long long)va_arg(args, unsigned int), 16U, 0);
        }
        else if (*format == 'X')
        {
            written += OrynLibCPrintfWriteUnsigned((unsigned long long)va_arg(args, unsigned int), 16U, 1);
        }
        else if (*format == 'p')
        {
            written += OrynLibCPrintfWriteText("0x");
            written += OrynLibCPrintfWriteUnsigned((unsigned long long)(uintptr_t)va_arg(args, void*), 16U, 0);
        }
        else if (*format == 'l')
        {
            ++format;

            if (*format == 'l')
            {
                ++format;
            }

            if (*format == 'd' || *format == 'i')
            {
                written += OrynLibCPrintfWriteSigned(va_arg(args, long long));
            }
            else if (*format == 'u')
            {
                written += OrynLibCPrintfWriteUnsigned(va_arg(args, unsigned long long), 10U, 0);
            }
            else if (*format == 'x')
            {
                written += OrynLibCPrintfWriteUnsigned(va_arg(args, unsigned long long), 16U, 0);
            }
            else if (*format == 'X')
            {
                written += OrynLibCPrintfWriteUnsigned(va_arg(args, unsigned long long), 16U, 1);
            }
            else
            {
                written += OrynLibCPrintfPutChar('%');
                written += OrynLibCPrintfPutChar('l');
                written += OrynLibCPrintfPutChar((unsigned char)*format);
            }
        }
        else
        {
            written += OrynLibCPrintfPutChar('%');
            written += OrynLibCPrintfPutChar((unsigned char)*format);
        }

        if (*format != '\0')
        {
            ++format;
        }
    }

    return written;
}

static inline int printf(const char* restrict format, ...)
{
    int written;
    va_list args;

    va_start(args, format);
    written = vprintf(format, args);
    va_end(args);

    return written;
}

static inline int putchar(int ch)
{
    return OrynLibCPrintfPutChar(ch);
}

static inline int puts(const char* text)
{
    int written = OrynLibCPrintfWriteText(text);
    written += OrynLibCPrintfPutChar('\n');
    return written;
}

#endif
