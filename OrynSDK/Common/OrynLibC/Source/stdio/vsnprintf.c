#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void OrynLibCBufferPut(char* target, size_t size, size_t* used, char ch)
{
    if (used == 0)
    {
        return;
    }

    if (target != 0 && size > 0U && *used + 1U < size)
    {
        target[*used] = ch;
    }

    *used = *used + 1U;
}

static void OrynLibCBufferPutString(char* target, size_t size, size_t* used, const char* text)
{
    if (text == 0)
    {
        text = "(null)";
    }

    while (*text != '\0')
    {
        OrynLibCBufferPut(target, size, used, *text);
        ++text;
    }
}

static void OrynLibCBufferPutUnsigned(char* target, size_t size, size_t* used, unsigned long long value, unsigned int base, int uppercase)
{
    char digits_lower[] = "0123456789abcdef";
    char digits_upper[] = "0123456789ABCDEF";
    char buffer[32];
    unsigned int index = 0U;
    const char* digits = uppercase ? digits_upper : digits_lower;

    if (base < 2U)
    {
        base = 10U;
    }

    if (value == 0ULL)
    {
        OrynLibCBufferPut(target, size, used, '0');
        return;
    }

    while (value != 0ULL && index < (unsigned int)sizeof(buffer))
    {
        buffer[index] = digits[value % base];
        value = value / base;
        ++index;
    }

    while (index > 0U)
    {
        --index;
        OrynLibCBufferPut(target, size, used, buffer[index]);
    }
}

static void OrynLibCBufferPutSigned(char* target, size_t size, size_t* used, long long value)
{
    unsigned long long unsigned_value;

    if (value < 0)
    {
        OrynLibCBufferPut(target, size, used, '-');
        unsigned_value = (unsigned long long)(0ULL - (unsigned long long)value);
    }
    else
    {
        unsigned_value = (unsigned long long)value;
    }

    OrynLibCBufferPutUnsigned(target, size, used, unsigned_value, 10U, 0);
}

int vsnprintf(char* restrict target, size_t size, const char* restrict format, va_list args)
{
    size_t used = 0U;

    if (format == 0)
    {
        if (target != 0 && size > 0U)
        {
            target[0] = '\0';
        }

        return -1;
    }

    while (*format != '\0')
    {
        int long_count = 0;

        if (*format != '%')
        {
            OrynLibCBufferPut(target, size, &used, *format);
            ++format;
            continue;
        }

        ++format;

        if (*format == '%')
        {
            OrynLibCBufferPut(target, size, &used, '%');
            ++format;
            continue;
        }

        while (*format == 'l')
        {
            ++long_count;
            ++format;
        }

        switch (*format)
        {
            case 'c':
                OrynLibCBufferPut(target, size, &used, (char)va_arg(args, int));
                break;
            case 's':
                OrynLibCBufferPutString(target, size, &used, va_arg(args, const char*));
                break;
            case 'd':
            case 'i':
                if (long_count >= 2)
                {
                    OrynLibCBufferPutSigned(target, size, &used, va_arg(args, long long));
                }
                else if (long_count == 1)
                {
                    OrynLibCBufferPutSigned(target, size, &used, va_arg(args, long));
                }
                else
                {
                    OrynLibCBufferPutSigned(target, size, &used, va_arg(args, int));
                }
                break;
            case 'u':
                if (long_count >= 2)
                {
                    OrynLibCBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long long), 10U, 0);
                }
                else if (long_count == 1)
                {
                    OrynLibCBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long), 10U, 0);
                }
                else
                {
                    OrynLibCBufferPutUnsigned(target, size, &used, va_arg(args, unsigned int), 10U, 0);
                }
                break;
            case 'x':
            case 'X':
                if (long_count >= 2)
                {
                    OrynLibCBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long long), 16U, *format == 'X');
                }
                else if (long_count == 1)
                {
                    OrynLibCBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long), 16U, *format == 'X');
                }
                else
                {
                    OrynLibCBufferPutUnsigned(target, size, &used, va_arg(args, unsigned int), 16U, *format == 'X');
                }
                break;
            case 'p':
                OrynLibCBufferPutString(target, size, &used, "0x");
                OrynLibCBufferPutUnsigned(target, size, &used, (uintptr_t)va_arg(args, void*), 16U, 0);
                break;
            default:
                OrynLibCBufferPut(target, size, &used, '%');
                if (*format != '\0')
                {
                    OrynLibCBufferPut(target, size, &used, *format);
                }
                break;
        }

        if (*format != '\0')
        {
            ++format;
        }
    }

    if (target != 0 && size > 0U)
    {
        if (used >= size)
        {
            target[size - 1U] = '\0';
        }
        else
        {
            target[used] = '\0';
        }
    }

    return (int)used;
}
