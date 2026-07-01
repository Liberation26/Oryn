#include "KernelRtc.h"
#include "KernelPortIo.h"
#include "KernelScreenReport.h"

#define ORYN_RTC_INDEX_PORT 0x70U
#define ORYN_RTC_DATA_PORT 0x71U
#define ORYN_RTC_NMI_DISABLE 0x80U
#define ORYN_RTC_REGISTER_SECONDS 0x00U
#define ORYN_RTC_REGISTER_MINUTES 0x02U
#define ORYN_RTC_REGISTER_HOURS 0x04U
#define ORYN_RTC_REGISTER_DAY 0x07U
#define ORYN_RTC_REGISTER_MONTH 0x08U
#define ORYN_RTC_REGISTER_YEAR 0x09U
#define ORYN_RTC_REGISTER_STATUS_A 0x0AU
#define ORYN_RTC_REGISTER_STATUS_B 0x0BU
#define ORYN_RTC_STATUS_A_UPDATE 0x80U
#define ORYN_RTC_STATUS_B_BINARY 0x04U
#define ORYN_RTC_STATUS_B_24HOUR 0x02U

static OrynKernelRtcState gRtcState;

static unsigned char ReadCmos(unsigned char reg)
{
    OrynPortOut8((unsigned short)ORYN_RTC_INDEX_PORT,
        (unsigned char)(ORYN_RTC_NMI_DISABLE | reg));
    OrynPortIoWait();
    return OrynPortIn8((unsigned short)ORYN_RTC_DATA_PORT);
}

static unsigned int BcdToBinary(unsigned int value)
{
    return ((value >> 4U) * 10U) + (value & 0x0FU);
}

static unsigned int ConvertField(unsigned int value, unsigned int bcdMode)
{
    if (bcdMode)
    {
        return BcdToBinary(value);
    }
    return value;
}

static int RtcUpdateInProgress(void)
{
    return (ReadCmos((unsigned char)ORYN_RTC_REGISTER_STATUS_A) & ORYN_RTC_STATUS_A_UPDATE) ? 1 : 0;
}

static int ReadRawStable(unsigned int* second, unsigned int* minute, unsigned int* hour,
    unsigned int* day, unsigned int* month, unsigned int* year, unsigned int* statusB)
{
    unsigned int firstSecond;
    unsigned int guard;
    for (guard = 0U; guard < 100000U && RtcUpdateInProgress(); ++guard)
    {
    }
    if (RtcUpdateInProgress())
    {
        return 0;
    }
    firstSecond = ReadCmos((unsigned char)ORYN_RTC_REGISTER_SECONDS);
    *minute = ReadCmos((unsigned char)ORYN_RTC_REGISTER_MINUTES);
    *hour = ReadCmos((unsigned char)ORYN_RTC_REGISTER_HOURS);
    *day = ReadCmos((unsigned char)ORYN_RTC_REGISTER_DAY);
    *month = ReadCmos((unsigned char)ORYN_RTC_REGISTER_MONTH);
    *year = ReadCmos((unsigned char)ORYN_RTC_REGISTER_YEAR);
    *statusB = ReadCmos((unsigned char)ORYN_RTC_REGISTER_STATUS_B);
    *second = ReadCmos((unsigned char)ORYN_RTC_REGISTER_SECONDS);
    return (*second == firstSecond) ? 1 : 0;
}

static unsigned int ConvertHour(unsigned int rawHour, unsigned int bcdMode, unsigned int twentyFour)
{
    unsigned int pm = rawHour & 0x80U;
    unsigned int hour = ConvertField(rawHour & 0x7FU, bcdMode);
    if (!twentyFour && pm && hour < 12U)
    {
        hour += 12U;
    }
    if (!twentyFour && !pm && hour == 12U)
    {
        hour = 0U;
    }
    return hour;
}

static int ValidateTime(const OrynKernelWallClockTime* time)
{
    return time->Year >= 2000U && time->Month >= 1U && time->Month <= 12U &&
        time->Day >= 1U && time->Day <= 31U && time->Hour <= 23U &&
        time->Minute <= 59U && time->Second <= 59U;
}

int OrynKernelRtcReadWallClockUtc(OrynKernelWallClockTime* outTime)
{
    unsigned int second;
    unsigned int minute;
    unsigned int hour;
    unsigned int day;
    unsigned int month;
    unsigned int year;
    unsigned int statusB;
    unsigned int bcdMode;
    unsigned int twentyFour;
    gRtcState.ReadAttempted = 1U;
    if (outTime == 0)
    {
        return 0;
    }
    if (!ReadRawStable(&second, &minute, &hour, &day, &month, &year, &statusB))
    {
        return 0;
    }
    gRtcState.CmosAccessible = 1U;
    gRtcState.UpdateClear = 1U;
    gRtcState.StableRead = 1U;
    bcdMode = ((statusB & ORYN_RTC_STATUS_B_BINARY) == 0U) ? 1U : 0U;
    twentyFour = (statusB & ORYN_RTC_STATUS_B_24HOUR) ? 1U : 0U;
    outTime->Second = ConvertField(second, bcdMode);
    outTime->Minute = ConvertField(minute, bcdMode);
    outTime->Hour = ConvertHour(hour, bcdMode, twentyFour);
    outTime->Day = ConvertField(day, bcdMode);
    outTime->Month = ConvertField(month, bcdMode);
    outTime->Year = 2000U + ConvertField(year, bcdMode);
    outTime->IsUtc = 1U;
    outTime->TimeZoneMinutes = 0;
    outTime->Valid = ValidateTime(outTime) ? 1U : 0U;
    gRtcState.BcdMode = bcdMode;
    gRtcState.TwentyFourHourMode = twentyFour;
    gRtcState.TreatedAsUtc = 1U;
    gRtcState.ReadValid = outTime->Valid;
    gRtcState.WallClockUtc = *outTime;
    return outTime->Valid ? 1 : 0;
}

const OrynKernelRtcState* OrynKernelRtcGetState(void)
{
    return &gRtcState;
}

void OrynKernelRtcPrintProof(void)
{
    OrynKernelScreenReportOkOrWarn(gRtcState.ReadAttempted,
        "RTC wall-clock read path attempted.",
        "RTC wall-clock read path has not run yet.");
    OrynKernelScreenReportOkOrWarn(gRtcState.ReadValid,
        "RTC wall-clock time read and normalized as UTC.",
        "RTC wall-clock time unavailable or invalid.");
}
