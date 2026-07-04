#include "KernelClock.h"
#include "KernelApic.h"
#include "KernelCpu.h"
#include "KernelHpet.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelModuleManifest.h"
#include "KernelRtc.h"
#include "KernelScreenReport.h"

#define ORYN_CLOCK_SOURCE_LIMIT 4U
#define ORYN_CLOCK_EVENT_LIMIT 4U
#define ORYN_CLOCK_APIC_TICK_VECTOR 0xEFU
#define ORYN_CLOCK_FEMTOSECONDS_PER_SECOND 1000000000000000ULL

static OrynKernelClockState gClockState;

static unsigned long long ReadTsc(void)
{
    unsigned int low;
    unsigned int high;
    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));
    return ((unsigned long long)high << 32) | (unsigned long long)low;
}

static void BusyDelay(void)
{
    for (volatile unsigned int index = 0U; index < 400000U; ++index)
    {
    }
}

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gClockState;
    for (unsigned int index = 0U; index < sizeof(gClockState); ++index)
    {
        bytes[index] = 0U;
    }
}

static unsigned long long ScaleCounterHz(unsigned long long delta, unsigned long long hpetDelta,
    unsigned long long hpetPeriodFemtoSeconds)
{
    unsigned long long elapsedFemto;
    if (delta == 0ULL || hpetDelta == 0ULL || hpetPeriodFemtoSeconds == 0ULL)
    {
        return 0ULL;
    }

    elapsedFemto = hpetDelta * hpetPeriodFemtoSeconds;
    if (elapsedFemto == 0ULL)
    {
        return 0ULL;
    }

    return (delta * ORYN_CLOCK_FEMTOSECONDS_PER_SECOND) / elapsedFemto;
}

static OrynKernelClockSourceRecord* AddSource(OrynKernelClockSourceKind kind, const char* name,
    unsigned int priority, unsigned int monotonic, unsigned int stable, unsigned long long frequencyHz)
{
    OrynKernelClockSourceRecord* record;
    if (gClockState.SourceCount >= ORYN_CLOCK_SOURCE_LIMIT)
    {
        return 0;
    }

    record = &gClockState.Sources[gClockState.SourceCount++];
    record->Kind = kind;
    record->Name = name;
    record->Priority = priority;
    record->Monotonic = monotonic;
    record->Stable = stable;
    record->FrequencyHz = frequencyHz;
    record->Registered = 1U;
    return record;
}

static OrynKernelClockEventRecord* AddEvent(OrynKernelClockEventKind kind, const char* name,
    unsigned int priority, unsigned int vector, unsigned int oneShot, unsigned int periodic)
{
    OrynKernelClockEventRecord* record;
    if (gClockState.EventCount >= ORYN_CLOCK_EVENT_LIMIT)
    {
        return 0;
    }

    record = &gClockState.Events[gClockState.EventCount++];
    record->Kind = kind;
    record->Name = name;
    record->Priority = priority;
    record->Vector = vector;
    record->CanOneShot = oneShot;
    record->CanPeriodicTick = periodic;
    record->MinimumDelta = 1ULL;
    record->MaximumDelta = 0xFFFFFFFFULL;
    record->Registered = 1U;
    return record;
}

static void SelectBestSource(void)
{
    unsigned int best = ORYN_CLOCK_SOURCE_LIMIT;
    unsigned int bestPriority = 0U;
    for (unsigned int index = 0U; index < gClockState.SourceCount; ++index)
    {
        OrynKernelClockSourceRecord* source = &gClockState.Sources[index];
        source->Selected = 0U;
        if (source->Registered && source->Monotonic && source->Priority >= bestPriority)
        {
            best = index;
            bestPriority = source->Priority;
        }
    }

    if (best < gClockState.SourceCount)
    {
        gClockState.Sources[best].Selected = 1U;
        gClockState.SourceSelected = 1U;
    }
}

static void SelectBestEvent(void)
{
    unsigned int best = ORYN_CLOCK_EVENT_LIMIT;
    unsigned int bestPriority = 0U;
    for (unsigned int index = 0U; index < gClockState.EventCount; ++index)
    {
        OrynKernelClockEventRecord* event = &gClockState.Events[index];
        event->Selected = 0U;
        if (event->Registered && event->Priority >= bestPriority)
        {
            best = index;
            bestPriority = event->Priority;
        }
    }

    if (best < gClockState.EventCount)
    {
        gClockState.Events[best].Selected = 1U;
        gClockState.EventSelected = 1U;
        gClockState.TickRegistered = gClockState.Events[best].CanPeriodicTick;
    }
}
static void CalibrateAgainstHpet(void)
{
    unsigned long long hpetBefore;
    unsigned long long hpetAfter;
    unsigned long long tscBefore;
    unsigned long long tscAfter;
    unsigned long long apicBefore;
    unsigned long long apicAfter;
    const OrynKernelHpetState* hpet = OrynKernelHpetGetState();
    const OrynKernelApicState* apic = OrynKernelApicGetState();

    gClockState.Calibration.Ran = 1U;
    if (hpet->Initialized == 0U || hpet->CounterPeriodFemtoSeconds == 0ULL)
    {
        return;
    }

    hpetBefore = OrynKernelHpetReadCounter();
    tscBefore = ReadTsc();
    apicBefore = OrynKernelApicReadTimerCurrent();
    BusyDelay();
    apicAfter = OrynKernelApicReadTimerCurrent();
    tscAfter = ReadTsc();
    hpetAfter = OrynKernelHpetReadCounter();

    if (hpetAfter <= hpetBefore)
    {
        return;
    }

    gClockState.Calibration.HpetReferenceAvailable = 1U;
    gClockState.Calibration.HpetPeriodFemtoSeconds = hpet->CounterPeriodFemtoSeconds;
    gClockState.Calibration.HpetDelta = hpetAfter - hpetBefore;
    gClockState.Calibration.TscDelta = tscAfter - tscBefore;
    gClockState.Calibration.TscFrequencyHz = ScaleCounterHz(
        gClockState.Calibration.TscDelta,
        gClockState.Calibration.HpetDelta,
        hpet->CounterPeriodFemtoSeconds);
    gClockState.Calibration.TscCalibrated =
        (gClockState.Calibration.TscFrequencyHz != 0ULL) ? 1U : 0U;

    if (apic->Initialized && apicBefore > apicAfter)
    {
        gClockState.Calibration.ApicDelta = apicBefore - apicAfter;
        gClockState.Calibration.ApicFrequencyHz = ScaleCounterHz(
            gClockState.Calibration.ApicDelta,
            gClockState.Calibration.HpetDelta,
            hpet->CounterPeriodFemtoSeconds);
        gClockState.Calibration.ApicCalibrated =
            (gClockState.Calibration.ApicFrequencyHz != 0ULL) ? 1U : 0U;
    }
}

static void RegisterAvailableSources(void)
{
    const OrynKernelCpuFeatures* cpu = OrynKernelCpuGetFeatures();
    const OrynKernelHpetState* hpet = OrynKernelHpetGetState();
    const OrynKernelApicState* apic = OrynKernelApicGetState();
    unsigned long long tscHz = gClockState.Calibration.TscFrequencyHz;
    unsigned long long apicHz = gClockState.Calibration.ApicFrequencyHz;

    if (cpu->Detected)
    {
        AddSource(OrynKernelClockSourceTsc, "TSC",
            cpu->HasInvariantTsc ? 300U : 160U, 1U,
            cpu->HasInvariantTsc ? 1U : 0U, tscHz);
    }

    if (hpet->Initialized && hpet->CounterAdvanced)
    {
        unsigned long long hz = 0ULL;
        if (hpet->CounterPeriodFemtoSeconds != 0ULL)
        {
            hz = ORYN_CLOCK_FEMTOSECONDS_PER_SECOND / hpet->CounterPeriodFemtoSeconds;
        }
        AddSource(OrynKernelClockSourceHpet, "HPET", 260U, 1U, 1U, hz);
    }

    if (apic->Initialized && apic->TimerCountMoved)
    {
        AddSource(OrynKernelClockSourceApicTimer, "APIC timer", 120U, 1U, 0U, apicHz);
    }
}

static void AdoptBootHandoffTime(const OrynBootInfo* bootInfo)
{
    gClockState.TimezonePolicy.Defined = 1U;
    gClockState.TimezonePolicy.KernelWallClockUtcOnly = 1U;
    gClockState.TimezonePolicy.LocalTimeConversionInUserland = 1U;
    gClockState.TimezonePolicy.UefiBootTimeHandoffOnly = 1U;
    gClockState.TimezonePolicy.KernelTimezoneOffsetMinutes = 0;
    if (bootInfo != 0 && bootInfo->FirmwareData.BootTimeValid)
    {
        gClockState.TimezonePolicy.UefiBootTimeCopied = 1U;
        gClockState.TimezonePolicy.BootHandoffTimezoneMinutes =
            bootInfo->FirmwareData.BootTimeTimeZone;
    }
}

static void ReadWallClockFromRtc(void)
{
    gClockState.RtcWallClockAttempted = 1U;
    if (OrynKernelRtcReadWallClockUtc(&gClockState.WallClockUtc))
    {
        gClockState.RtcWallClockValid = 1U;
    }
}

static void RegisterAvailableEvents(void)
{
    const OrynKernelHpetState* hpet = OrynKernelHpetGetState();
    const OrynKernelApicState* apic = OrynKernelApicGetState();
    if (apic->Initialized)
    {
        AddEvent(OrynKernelClockEventApicTimer, "APIC local timer", 300U,
            ORYN_CLOCK_APIC_TICK_VECTOR, 1U, 1U);
    }

    if (hpet->Initialized && hpet->TimerCount > 0U)
    {
        AddEvent(OrynKernelClockEventHpetTimer, "HPET comparator", 240U, 0U, 1U, 1U);
    }
}

int OrynKernelClockBootSelect(const OrynBootInfo* bootInfo)
{
    ClearState();
    AdoptBootHandoffTime(bootInfo);
    CalibrateAgainstHpet();
    RegisterAvailableSources();
    RegisterAvailableEvents();
    ReadWallClockFromRtc();
    SelectBestSource();
    SelectBestEvent();
    gClockState.SelectionRan = 1U;
    gClockState.Initialized = gClockState.SourceSelected;
    return gClockState.Initialized ? 1 : 0;
}

const OrynKernelClockState* OrynKernelClockGetState(void)
{
    return &gClockState;
}

int OrynKernelClockReadWallClockUtc(OrynKernelWallClockTime* outTime)
{
    if (outTime == 0 || gClockState.RtcWallClockValid == 0U)
    {
        return 0;
    }
    *outTime = gClockState.WallClockUtc;
    return 1;
}

unsigned long long OrynKernelClockReadMonotonicRaw(void)
{
    for (unsigned int index = 0U; index < gClockState.SourceCount; ++index)
    {
        if (gClockState.Sources[index].Selected)
        {
            if (gClockState.Sources[index].Kind == OrynKernelClockSourceTsc)
            {
                return ReadTsc();
            }

            if (gClockState.Sources[index].Kind == OrynKernelClockSourceHpet)
            {
                return OrynKernelHpetReadCounter();
            }

            if (gClockState.Sources[index].Kind == OrynKernelClockSourceApicTimer)
            {
                return OrynKernelApicReadTimerCurrent();
            }
        }
    }

    return 0ULL;
}

static const char* SelectedSourceName(void)
{
    for (unsigned int index = 0U; index < gClockState.SourceCount; ++index)
    {
        if (gClockState.Sources[index].Selected)
        {
            return gClockState.Sources[index].Name;
        }
    }

    return "none";
}

static const char* SelectedEventName(void)
{
    for (unsigned int index = 0U; index < gClockState.EventCount; ++index)
    {
        if (gClockState.Events[index].Selected)
        {
            return gClockState.Events[index].Name;
        }
    }

    return "none";
}

void OrynKernelClockPrintProof(void)
{
    OrynKernelScreenReportOkOrFail(gClockState.SelectionRan,
        "Kernel clock selection ran during boot.",
        "Kernel clock selection did not run during boot.");
    OrynKernelScreenReportOkOrFail(gClockState.SourceSelected,
        "Monotonic kernel clock source selected.",
        "No monotonic kernel clock source selected.");
    OrynKernelDiagnosticsLogText("[KERNEL] Clocksource selected: ");
    OrynKernelDiagnosticsLogText(SelectedSourceName());
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrWarn(gClockState.EventSelected,
        "Clockevent/tick provider registered.",
        "Clockevent/tick provider not registered yet.");
    OrynKernelDiagnosticsLogText("[KERNEL] Clockevent selected: ");
    OrynKernelDiagnosticsLogText(SelectedEventName());
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrWarn(gClockState.Calibration.HpetReferenceAvailable,
        "HPET reference available for clock calibration.",
        "HPET reference unavailable for clock calibration.");
    OrynKernelScreenReportOkOrWarn(gClockState.Calibration.TscCalibrated,
        "TSC calibrated against HPET.",
        "TSC calibration deferred until reference timer is available.");
    OrynKernelScreenReportOkOrWarn(gClockState.Calibration.ApicCalibrated,
        "APIC timer calibrated against HPET.",
        "APIC timer calibration deferred until APIC/HPET are both available.");
    OrynKernelScreenReportOkOrWarn(gClockState.RtcWallClockAttempted,
        "RTC read path attempted for wall-clock time.",
        "RTC read path did not run.");
    OrynKernelScreenReportOkOrWarn(gClockState.RtcWallClockValid,
        "RTC wall-clock time is available through UTC clock policy.",
        "RTC wall-clock time unavailable; UTC wall clock deferred.");
    OrynKernelScreenReportOkOrFail(gClockState.TimezonePolicy.UefiBootTimeHandoffOnly,
        "UEFI boot time is treated as boot handoff data only.",
        "UEFI boot time policy is not marked handoff-only.");
    OrynKernelScreenReportOkOrFail(gClockState.TimezonePolicy.KernelWallClockUtcOnly &&
        gClockState.TimezonePolicy.LocalTimeConversionInUserland,
        "UTC/local timezone policy defined: kernel stores UTC, userland converts local time.",
        "UTC/local timezone policy is not defined.");
    OrynKernelRtcPrintProof();
}
