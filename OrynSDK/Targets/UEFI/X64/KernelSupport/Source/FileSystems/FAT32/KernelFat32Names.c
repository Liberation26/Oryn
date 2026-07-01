#include "KernelFat32Internal.h"

static char Fat32Upper(char value)
{
    if (value >= 'a' && value <= 'z')
    {
        return (char)(value - ('a' - 'A'));
    }
    return value;
}

static int Fat32ValidShortChar(char value)
{
    const char* extra = "$%'-_@~`!(){}^#&";
    uint32_t index;
    if ((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') ||
        (value >= '0' && value <= '9'))
    {
        return 1;
    }
    for (index = 0U; extra[index] != 0; ++index)
    {
        if (value == extra[index])
        {
            return 1;
        }
    }
    return 0;
}

static int Fat32PartLength(const char* text, uint32_t* name_length, uint32_t* ext_length)
{
    uint32_t index;
    uint32_t dot = 0xFFFFFFFFU;
    *name_length = 0U;
    *ext_length = 0U;
    if (text == 0 || text[0] == 0 || Fat32NamesEqual(text, ".") || Fat32NamesEqual(text, ".."))
    {
        return 0;
    }
    for (index = 0U; text[index] != 0; ++index)
    {
        if (text[index] == '.')
        {
            if (dot != 0xFFFFFFFFU)
            {
                return 0;
            }
            dot = index;
        }
        else if (!Fat32ValidShortChar(text[index]))
        {
            return 0;
        }
    }
    if (dot == 0xFFFFFFFFU)
    {
        *name_length = index;
    }
    else
    {
        *name_length = dot;
        *ext_length = index - dot - 1U;
    }
    return *name_length > 0U && *name_length <= 8U && *ext_length <= 3U;
}

int Fat32PathIsSafeShortName(const char* path)
{
    const char* cursor = path;
    Fat32PathPart part;
    uint32_t name_length;
    uint32_t ext_length;
    if (path == 0 || path[0] == 0)
    {
        return 0;
    }
    if (Fat32NamesEqual(path, "/"))
    {
        return 1;
    }
    while (Fat32NextPathPart(&cursor, &part))
    {
        if (!Fat32PartLength(part.Text, &name_length, &ext_length))
        {
            return 0;
        }
    }
    return 1;
}

int OrynFat32ValidatePath(const char* path)
{
    return Fat32PathIsSafeShortName(path);
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
