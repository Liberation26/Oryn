#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void OrynStdoutBufferPut(char* target, size_t size, size_t* used, char value)
{
    if (target != 0 && size > 0U && *used + 1U < size)
    {
        target[*used] = value;
    }

    *used = *used + 1U;
}

static void OrynStdoutBufferPutString(char* target, size_t size, size_t* used, const char* text)
{
    const char* current = text;

    if (current == 0)
    {
        current = "(null)";
    }

    while (*current != '\0')
    {
        OrynStdoutBufferPut(target, size, used, *current);
        ++current;
    }
}

static void OrynStdoutBufferPutUnsigned(char* target, size_t size, size_t* used, unsigned long long value, unsigned int base, int uppercase)
{
    char buffer[32];
    unsigned int index = 0U;
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";

    if (base < 2U)
    {
        base = 10U;
    }

    if (value == 0ULL)
    {
        OrynStdoutBufferPut(target, size, used, '0');
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
        OrynStdoutBufferPut(target, size, used, buffer[index]);
    }
}

static void OrynStdoutBufferPutSigned(char* target, size_t size, size_t* used, long long value)
{
    unsigned long long unsignedValue;

    if (value < 0LL)
    {
        OrynStdoutBufferPut(target, size, used, '-');
        unsignedValue = (unsigned long long)(0ULL - (unsigned long long)value);
    }
    else
    {
        unsignedValue = (unsigned long long)value;
    }

    OrynStdoutBufferPutUnsigned(target, size, used, unsignedValue, 10U, 0);
}

int vsnprintf(char* restrict target, size_t size, const char* restrict format, va_list args)
{
    size_t used = 0U;
    const char* current = format;

    if (format == 0)
    {
        if (target != 0 && size > 0U)
        {
            target[0] = '\0';
        }

        return -1;
    }

    while (*current != '\0')
    {
        if (*current != '%')
        {
            OrynStdoutBufferPut(target, size, &used, *current);
            ++current;
            continue;
        }

        ++current;

        if (*current == '%')
        {
            OrynStdoutBufferPut(target, size, &used, '%');
        }
        else if (*current == 's')
        {
            OrynStdoutBufferPutString(target, size, &used, va_arg(args, const char*));
        }
        else if (*current == 'c')
        {
            OrynStdoutBufferPut(target, size, &used, (char)va_arg(args, int));
        }
        else if (*current == 'd' || *current == 'i')
        {
            OrynStdoutBufferPutSigned(target, size, &used, (long long)va_arg(args, int));
        }
        else if (*current == 'u')
        {
            OrynStdoutBufferPutUnsigned(target, size, &used, (unsigned long long)va_arg(args, unsigned int), 10U, 0);
        }
        else if (*current == 'x')
        {
            OrynStdoutBufferPutUnsigned(target, size, &used, (unsigned long long)va_arg(args, unsigned int), 16U, 0);
        }
        else if (*current == 'X')
        {
            OrynStdoutBufferPutUnsigned(target, size, &used, (unsigned long long)va_arg(args, unsigned int), 16U, 1);
        }
        else if (*current == 'p')
        {
            OrynStdoutBufferPutString(target, size, &used, "0x");
            OrynStdoutBufferPutUnsigned(target, size, &used, (unsigned long long)(uintptr_t)va_arg(args, void*), 16U, 0);
        }
        else if (*current == 'l')
        {
            ++current;

            if (*current == 'l')
            {
                ++current;
            }

            if (*current == 'd' || *current == 'i')
            {
                OrynStdoutBufferPutSigned(target, size, &used, va_arg(args, long long));
            }
            else if (*current == 'u')
            {
                OrynStdoutBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long long), 10U, 0);
            }
            else if (*current == 'x')
            {
                OrynStdoutBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long long), 16U, 0);
            }
            else if (*current == 'X')
            {
                OrynStdoutBufferPutUnsigned(target, size, &used, va_arg(args, unsigned long long), 16U, 1);
            }
            else
            {
                OrynStdoutBufferPut(target, size, &used, '%');
                OrynStdoutBufferPut(target, size, &used, 'l');
                OrynStdoutBufferPut(target, size, &used, *current);
            }
        }
        else
        {
            OrynStdoutBufferPut(target, size, &used, '%');
            OrynStdoutBufferPut(target, size, &used, *current);
        }

        if (*current != '\0')
        {
            ++current;
        }
    }

    if (target != 0 && size > 0U)
    {
        if (used < size)
        {
            target[used] = '\0';
        }
        else
        {
            target[size - 1U] = '\0';
        }
    }

    return (int)used;
}
