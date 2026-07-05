#include "KernelIo.h"
#include "stdarg.h"
#include "stdio.h"

#define ORYN_VPRINTF_BUFFER_SIZE 1024U

int vprintf(const char* restrict format, va_list args)
{
    char buffer[ORYN_VPRINTF_BUFFER_SIZE];
    int result;

    result = vsnprintf(buffer, sizeof(buffer), format, args);
    if (result < 0)
    {
        return result;
    }

    KernelIoWriteString(buffer);
    return result;
}
