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

#define ORYN_STATUS_SUCCESS(statusValue) ((statusValue).Code == ORYN_STATUS_OK)
#define ORYN_STATUS_WARNING(statusValue) ((statusValue).Code == ORYN_STATUS_WARN)
#define ORYN_STATUS_FAILED(statusValue)  ((statusValue).Code >= ORYN_STATUS_FAIL)

OrynStatus OrynStatusOk(const char* message);
OrynStatus OrynStatusWarn(const char* message);
OrynStatus OrynStatusFail(const char* message);
OrynStatus OrynStatusInvalidArgument(const char* message);
OrynStatus OrynStatusNotFound(const char* message);
OrynStatus OrynStatusNotReady(const char* message);
OrynStatus OrynStatusAlreadyStarted(const char* message);
OrynStatus OrynStatusUnsupported(const char* message);
OrynStatus OrynStatusOutOfMemory(const char* message);
OrynStatus OrynStatusIoError(const char* message);
OrynStatus OrynStatusPermissionDenied(const char* message);
OrynStatus OrynStatusTimeout(const char* message);
OrynStatus OrynStatusCorrupt(const char* message);
const char* OrynStatusCodeName(OrynStatusCode code);

#ifdef __cplusplus
}
#endif

#endif
