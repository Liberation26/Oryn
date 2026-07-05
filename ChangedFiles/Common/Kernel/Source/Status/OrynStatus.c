#include "OrynStatus.h"

static OrynStatus OrynStatusMake(OrynStatusCode code, const char* message)
{
    OrynStatus status;
    status.Code = code;
    status.Message = message;
    return status;
}

OrynStatus OrynStatusOk(const char* message) { return OrynStatusMake(ORYN_STATUS_OK, message); }
OrynStatus OrynStatusWarn(const char* message) { return OrynStatusMake(ORYN_STATUS_WARN, message); }
OrynStatus OrynStatusFail(const char* message) { return OrynStatusMake(ORYN_STATUS_FAIL, message); }
OrynStatus OrynStatusInvalidArgument(const char* message) { return OrynStatusMake(ORYN_STATUS_INVALID_ARGUMENT, message); }
OrynStatus OrynStatusNotFound(const char* message) { return OrynStatusMake(ORYN_STATUS_NOT_FOUND, message); }
OrynStatus OrynStatusNotReady(const char* message) { return OrynStatusMake(ORYN_STATUS_NOT_READY, message); }
OrynStatus OrynStatusAlreadyStarted(const char* message) { return OrynStatusMake(ORYN_STATUS_ALREADY_STARTED, message); }
OrynStatus OrynStatusUnsupported(const char* message) { return OrynStatusMake(ORYN_STATUS_UNSUPPORTED, message); }
OrynStatus OrynStatusOutOfMemory(const char* message) { return OrynStatusMake(ORYN_STATUS_OUT_OF_MEMORY, message); }
OrynStatus OrynStatusIoError(const char* message) { return OrynStatusMake(ORYN_STATUS_IO_ERROR, message); }
OrynStatus OrynStatusPermissionDenied(const char* message) { return OrynStatusMake(ORYN_STATUS_PERMISSION_DENIED, message); }
OrynStatus OrynStatusTimeout(const char* message) { return OrynStatusMake(ORYN_STATUS_TIMEOUT, message); }
OrynStatus OrynStatusCorrupt(const char* message) { return OrynStatusMake(ORYN_STATUS_CORRUPT, message); }

const char* OrynStatusCodeName(OrynStatusCode code)
{
    switch (code)
    {
        case ORYN_STATUS_OK: return "OK";
        case ORYN_STATUS_WARN: return "WARN";
        case ORYN_STATUS_FAIL: return "FAIL";
        case ORYN_STATUS_INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case ORYN_STATUS_NOT_FOUND: return "NOT_FOUND";
        case ORYN_STATUS_NOT_READY: return "NOT_READY";
        case ORYN_STATUS_ALREADY_STARTED: return "ALREADY_STARTED";
        case ORYN_STATUS_UNSUPPORTED: return "UNSUPPORTED";
        case ORYN_STATUS_OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case ORYN_STATUS_IO_ERROR: return "IO_ERROR";
        case ORYN_STATUS_PERMISSION_DENIED: return "PERMISSION_DENIED";
        case ORYN_STATUS_TIMEOUT: return "TIMEOUT";
        case ORYN_STATUS_CORRUPT: return "CORRUPT";
        default: return "UNKNOWN";
    }
}
