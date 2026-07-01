#ifndef ORYN_KERNEL_RTC_H
#define ORYN_KERNEL_RTC_H

typedef struct OrynKernelWallClockTime
{
    unsigned int Year;
    unsigned int Month;
    unsigned int Day;
    unsigned int Hour;
    unsigned int Minute;
    unsigned int Second;
    unsigned int Valid;
    unsigned int IsUtc;
    int TimeZoneMinutes;
} OrynKernelWallClockTime;

typedef struct OrynKernelRtcState
{
    unsigned int ReadAttempted;
    unsigned int ReadValid;
    unsigned int CmosAccessible;
    unsigned int BcdMode;
    unsigned int TwentyFourHourMode;
    unsigned int UpdateClear;
    unsigned int StableRead;
    unsigned int TreatedAsUtc;
    OrynKernelWallClockTime WallClockUtc;
} OrynKernelRtcState;

int OrynKernelRtcReadWallClockUtc(OrynKernelWallClockTime* outTime);
const OrynKernelRtcState* OrynKernelRtcGetState(void);
void OrynKernelRtcPrintProof(void);

#endif
