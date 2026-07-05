#ifndef ORYN_STATUS_H
#define ORYN_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum OrynStatusCode
{
    ORYN_STATUS_OK = 0,
    ORYN_STATUS_WARN = 1,
    ORYN_STATUS_FAIL = 2,

    ORYN_STATUS_INVALID_ARGUMENT = 10,
    ORYN_STATUS_NOT_FOUND = 11,
    ORYN_STATUS_NOT_READY = 12,
    ORYN_STATUS_ALREADY_STARTED = 13,
    ORYN_STATUS_UNSUPPORTED = 14,
    ORYN_STATUS_OUT_OF_MEMORY = 15,
    ORYN_STATUS_IO_ERROR = 16,
    ORYN_STATUS_PERMISSION_DENIED = 17,
    ORYN_STATUS_TIMEOUT = 18,
    ORYN_STATUS_CORRUPT = 19
} OrynStatusCode;

typedef struct OrynStatus
{
    OrynStatusCode Code;
    const char* Message;
} OrynStatus;

#define ORYN_STATUS_SUCCESS(status) ((status).Code == ORYN_STATUS_OK || (status).Code == ORYN_STATUS_WARN)
#define ORYN_STATUS_FAILED(status)  ((status).Code >= ORYN_STATUS_FAIL)

static inline OrynStatus OrynStatusMake(OrynStatusCode code, const char* message)
{
    OrynStatus status;
    status.Code = code;
    status.Message = message;
    return status;
}

static inline OrynStatus OrynStatusOk(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_OK, message);
}

static inline OrynStatus OrynStatusWarn(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_WARN, message);
}

static inline OrynStatus OrynStatusFail(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_FAIL, message);
}

static inline OrynStatus OrynStatusInvalidArgument(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_INVALID_ARGUMENT, message);
}

static inline OrynStatus OrynStatusNotFound(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_NOT_FOUND, message);
}

static inline OrynStatus OrynStatusNotReady(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_NOT_READY, message);
}

static inline OrynStatus OrynStatusAlreadyStarted(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_ALREADY_STARTED, message);
}

static inline OrynStatus OrynStatusUnsupported(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_UNSUPPORTED, message);
}

static inline OrynStatus OrynStatusOutOfMemory(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_OUT_OF_MEMORY, message);
}

static inline OrynStatus OrynStatusIoError(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_IO_ERROR, message);
}

static inline OrynStatus OrynStatusPermissionDenied(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_PERMISSION_DENIED, message);
}

static inline OrynStatus OrynStatusTimeout(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_TIMEOUT, message);
}

static inline OrynStatus OrynStatusCorrupt(const char* message)
{
    return OrynStatusMake(ORYN_STATUS_CORRUPT, message);
}

#ifdef __cplusplus
}
#endif

#endif
