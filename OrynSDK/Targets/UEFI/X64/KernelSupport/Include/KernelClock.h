#ifndef ORYN_KERNEL_CLOCK_H
#define ORYN_KERNEL_CLOCK_H

#include "OrynBootInfo.h"
#include "KernelRtc.h"

typedef enum OrynKernelClockSourceKind
{
    OrynKernelClockSourceNone = 0,
    OrynKernelClockSourceTsc,
    OrynKernelClockSourceHpet,
    OrynKernelClockSourceApicTimer
} OrynKernelClockSourceKind;

typedef enum OrynKernelClockEventKind
{
    OrynKernelClockEventNone = 0,
    OrynKernelClockEventApicTimer,
    OrynKernelClockEventHpetTimer
} OrynKernelClockEventKind;

typedef struct OrynKernelClockSourceRecord
{
    OrynKernelClockSourceKind Kind;
    const char* Name;
    unsigned int Registered;
    unsigned int Selected;
    unsigned int Priority;
    unsigned int Monotonic;
    unsigned int Stable;
    unsigned long long FrequencyHz;
    unsigned long long LastRaw;
} OrynKernelClockSourceRecord;

typedef struct OrynKernelClockEventRecord
{
    OrynKernelClockEventKind Kind;
    const char* Name;
    unsigned int Registered;
    unsigned int Selected;
    unsigned int Priority;
    unsigned int CanOneShot;
    unsigned int CanPeriodicTick;
    unsigned int Vector;
    unsigned long long MinimumDelta;
    unsigned long long MaximumDelta;
} OrynKernelClockEventRecord;

typedef struct OrynKernelClockTimezonePolicy
{
    unsigned int Defined;
    unsigned int KernelWallClockUtcOnly;
    unsigned int LocalTimeConversionInUserland;
    unsigned int UefiBootTimeHandoffOnly;
    unsigned int UefiBootTimeCopied;
    int KernelTimezoneOffsetMinutes;
    int BootHandoffTimezoneMinutes;
} OrynKernelClockTimezonePolicy;

typedef struct OrynKernelClockCalibration
{
    unsigned int Ran;
    unsigned int HpetReferenceAvailable;
    unsigned int TscCalibrated;
    unsigned int ApicCalibrated;
    unsigned long long HpetPeriodFemtoSeconds;
    unsigned long long HpetDelta;
    unsigned long long TscDelta;
    unsigned long long ApicDelta;
    unsigned long long TscFrequencyHz;
    unsigned long long ApicFrequencyHz;
} OrynKernelClockCalibration;

typedef struct OrynKernelClockState
{
    unsigned int Initialized;
    unsigned int SourceCount;
    unsigned int EventCount;
    unsigned int SourceSelected;
    unsigned int EventSelected;
    unsigned int SelectionRan;
    unsigned int TickRegistered;
    unsigned int RtcWallClockAttempted;
    unsigned int RtcWallClockValid;
    OrynKernelWallClockTime WallClockUtc;
    OrynKernelClockTimezonePolicy TimezonePolicy;
    OrynKernelClockSourceRecord Sources[4];
    OrynKernelClockEventRecord Events[4];
    OrynKernelClockCalibration Calibration;
} OrynKernelClockState;

int OrynKernelClockBootSelect(const OrynBootInfo* bootInfo);
const OrynKernelClockState* OrynKernelClockGetState(void);
unsigned long long OrynKernelClockReadMonotonicRaw(void);
int OrynKernelClockReadWallClockUtc(OrynKernelWallClockTime* outTime);
void OrynKernelClockPrintProof(void);

#endif
