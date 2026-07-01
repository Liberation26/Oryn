#include "stdarg.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"

static void PutChar(char* target, size_t size, size_t* used, char value)
{
    if (*used + 1U < size) { target[*used] = value; }
    ++(*used);
}

static void PutText(char* target, size_t size, size_t* used, const char* text)
{
    if (text == 0) { text = "(null)"; }
    while (*text != 0) { PutChar(target, size, used, *text++); }
}

static void PutUnsigned(char* target, size_t size, size_t* used,
    unsigned long long value, unsigned base, int upper)
{
    char digits[32];
    size_t count = 0U;
    const char* alphabet = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (value == 0ULL) { PutChar(target, size, used, '0'); return; }
    while (value != 0ULL)
    {
        digits[count++] = alphabet[value % base];
        value /= base;
    }
    while (count > 0U) { PutChar(target, size, used, digits[--count]); }
}

static void PutSigned(char* target, size_t size, size_t* used, long long value)
{
    unsigned long long magnitude;
    if (value < 0)
    {
        PutChar(target, size, used, '-');
        magnitude = (unsigned long long)(-(value + 1LL)) + 1ULL;
    }
    else
    {
        magnitude = (unsigned long long)value;
    }
    PutUnsigned(target, size, used, magnitude, 10U, 0);
}

int vsnprintf(char* restrict target, size_t size, const char* restrict format, va_list args)
{
    size_t used = 0U;
    while (*format != 0)
    {
        int long_count = 0;
        if (*format != '%') { PutChar(target, size, &used, *format++); continue; }
        ++format;
        if (*format == '%') { PutChar(target, size, &used, *format++); continue; }
        while (*format == 'l') { ++long_count; ++format; }
        switch (*format++)
        {
            case 'c': PutChar(target, size, &used, (char)va_arg(args, int)); break;
            case 's': PutText(target, size, &used, va_arg(args, const char*)); break;
            case 'd': case 'i':
                PutSigned(target, size, &used,
                    long_count >= 2 ? va_arg(args, long long) :
                    long_count == 1 ? va_arg(args, long) : va_arg(args, int));
                break;
            case 'u':
                PutUnsigned(target, size, &used,
                    long_count >= 2 ? va_arg(args, unsigned long long) :
                    long_count == 1 ? va_arg(args, unsigned long) : va_arg(args, unsigned int), 10U, 0);
                break;
            case 'x': case 'X':
                PutUnsigned(target, size, &used,
                    long_count >= 2 ? va_arg(args, unsigned long long) :
                    long_count == 1 ? va_arg(args, unsigned long) : va_arg(args, unsigned int),
                    16U, format[-1] == 'X');
                break;
            case 'p':
                PutText(target, size, &used, "0x");
                PutUnsigned(target, size, &used, (uintptr_t)va_arg(args, void*), 16U, 0);
                break;
            default: PutChar(target, size, &used, '?'); break;
        }
    }
    if (size != 0U)
    {
        target[(used < size) ? used : (size - 1U)] = 0;
    }
    return (int)used;
}
