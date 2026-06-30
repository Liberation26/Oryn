#include "KernelFat32Internal.h"

static char Fat32Upper(char value)
{
    if (value >= 'a' && value <= 'z')
    {
        return (char)(value - ('a' - 'A'));
    }
    return value;
}

static void Fat32CopyShortPart(const char* input, uint8_t* output, uint32_t max_count)
{
    uint32_t index;
    for (index = 0; index < max_count && input[index] != 0 && input[index] != '.'; ++index)
    {
        output[index] = (uint8_t)Fat32Upper(input[index]);
    }
}

void Fat32MakeShortName(const char* name, uint8_t output[11])
{
    uint32_t index;
    const char* extension = 0;

    for (index = 0; index < 11U; ++index)
    {
        output[index] = (uint8_t)' ';
    }

    Fat32CopyShortPart(name, output, 8U);
    for (index = 0; name[index] != 0; ++index)
    {
        if (name[index] == '.')
        {
            extension = name + index + 1U;
        }
    }

    if (extension != 0)
    {
        Fat32CopyShortPart(extension, output + 8U, 3U);
    }
}

int Fat32ReadDirectoryEntryName(const uint8_t* entry, char* output, uint32_t output_size)
{
    uint32_t input;
    uint32_t out = 0;
    int has_extension = 0;

    if (output_size == 0U || entry[0] == 0U || entry[0] == 0xE5U)
    {
        return 0;
    }

    for (input = 0; input < 8U && entry[input] != ' '; ++input)
    {
        if (out + 1U < output_size)
        {
            output[out++] = (char)entry[input];
        }
    }

    for (input = 8U; input < 11U; ++input)
    {
        if (entry[input] != ' ')
        {
            has_extension = 1;
        }
    }

    if (has_extension && out + 1U < output_size)
    {
        output[out++] = '.';
    }

    for (input = 8U; input < 11U && entry[input] != ' '; ++input)
    {
        if (out + 1U < output_size)
        {
            output[out++] = (char)entry[input];
        }
    }

    output[out] = 0;
    return 1;
}

int Fat32NamesEqual(const char* left, const char* right)
{
    uint32_t index = 0;
    while (left[index] != 0 || right[index] != 0)
    {
        if (Fat32Upper(left[index]) != Fat32Upper(right[index]))
        {
            return 0;
        }
        ++index;
    }
    return 1;
}

int Fat32NextPathPart(const char** cursor, Fat32PathPart* part)
{
    uint32_t length = 0;
    const char* input;

    if (cursor == 0 || *cursor == 0 || part == 0)
    {
        return 0;
    }

    input = *cursor;
    while (*input == '/')
    {
        ++input;
    }

    if (*input == 0)
    {
        *cursor = input;
        return 0;
    }

    while (input[length] != 0 && input[length] != '/')
    {
        if (length + 1U < sizeof(part->Text))
        {
            part->Text[length] = input[length];
        }
        ++length;
    }

    if (length >= sizeof(part->Text))
    {
        return 0;
    }

    part->Text[length] = 0;
    input += length;
    while (*input == '/')
    {
        ++input;
    }
    part->IsLast = (*input == 0);
    *cursor = input;
    return 1;
}
