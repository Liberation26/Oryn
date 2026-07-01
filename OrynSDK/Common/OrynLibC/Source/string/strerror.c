#include "errno.h"
#include "string.h"

char* strerror(int error_number)
{
    switch (error_number)
    {
        case 0: return "No error";
        case EPERM: return "Operation not permitted";
        case ENOENT: return "No such file or directory";
        case EIO: return "Input/output error";
        case ENOMEM: return "Out of memory";
        case EACCES: return "Permission denied";
        case EEXIST: return "File exists";
        case ENODEV: return "No such device";
        case ENOTDIR: return "Not a directory";
        case EISDIR: return "Is a directory";
        case EINVAL: return "Invalid argument";
        case ENOSPC: return "No space left on device";
        case ERANGE: return "Result out of range";
        case ENOSYS: return "Function not implemented";
        default: return "Unknown error";
    }
}
