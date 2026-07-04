#include "KernelLifecycle.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"

#define ORYN_LIFECYCLE_MAX_TRANSITIONS 32

static OrynKernelLifecycleStatus gLifecycle;
static OrynKernelLifecycleState gTransitionFrom[ORYN_LIFECYCLE_MAX_TRANSITIONS];
static OrynKernelLifecycleState gTransitionTo[ORYN_LIFECYCLE_MAX_TRANSITIONS];

static void SetSeenFlag(OrynKernelLifecycleState state)
{
    switch (state)
    {
        case OrynKernelLifecycleEntered:
            gLifecycle.HasEntered = 1;
            break;
        case OrynKernelLifecycleBootInfoAdopted:
            gLifecycle.HasBootInfo = 1;
            break;
        case OrynKernelLifecycleDescriptorsReady:
            gLifecycle.HasDescriptors = 1;
            break;
        case OrynKernelLifecycleInterruptsReady:
            gLifecycle.HasInterrupts = 1;
            break;
        case OrynKernelLifecycleTimersReady:
            gLifecycle.HasTimers = 1;
            break;
        case OrynKernelLifecycleConsoleReady:
            gLifecycle.HasConsole = 1;
            break;
        case OrynKernelLifecycleMemoryReady:
            gLifecycle.HasMemory = 1;
            break;
        case OrynKernelLifecycleVirtualMemoryReady:
            gLifecycle.HasVirtualMemory = 1;
            break;
        case OrynKernelLifecycleRunning:
            gLifecycle.HasRunning = 1;
            break;
        case OrynKernelLifecycleDebugExitRequested:
            gLifecycle.HasDebugExitRequest = 1;
            break;
        case OrynKernelLifecycleInteractiveHalt:
            gLifecycle.HasInteractiveHalt = 1;
            break;
        case OrynKernelLifecycleHalting:
            gLifecycle.HasHalting = 1;
            break;
        case OrynKernelLifecycleHalted:
            gLifecycle.HasHalted = 1;
            break;
        case OrynKernelLifecyclePanic:
            gLifecycle.HasPanic = 1;
            break;
        default:
            break;
    }
}

const char* OrynKernelLifecycleStateName(OrynKernelLifecycleState state)
{
    switch (state)
    {
        case OrynKernelLifecycleCold:
            return "Cold";
        case OrynKernelLifecycleEntered:
            return "Entered";
        case OrynKernelLifecycleBootInfoAdopted:
            return "BootInfoAdopted";
        case OrynKernelLifecycleDescriptorsReady:
            return "DescriptorsReady";
        case OrynKernelLifecycleInterruptsReady:
            return "InterruptsReady";
        case OrynKernelLifecycleTimersReady:
            return "TimersReady";
        case OrynKernelLifecycleConsoleReady:
            return "ConsoleReady";
        case OrynKernelLifecycleMemoryReady:
            return "MemoryReady";
        case OrynKernelLifecycleVirtualMemoryReady:
            return "VirtualMemoryReady";
        case OrynKernelLifecycleRunning:
            return "Running";
        case OrynKernelLifecycleDebugExitRequested:
            return "DebugExitRequested";
        case OrynKernelLifecycleInteractiveHalt:
            return "InteractiveHalt";
        case OrynKernelLifecycleHalting:
            return "Halting";
        case OrynKernelLifecycleHalted:
            return "Halted";
        case OrynKernelLifecyclePanic:
            return "Panic";
        default:
            return "Unknown";
    }
}

static int IsForwardLifecycleState(OrynKernelLifecycleState state)
{
    return state >= OrynKernelLifecycleCold && state <= OrynKernelLifecycleHalted;
}

static int IsTransitionAllowed(OrynKernelLifecycleState current, OrynKernelLifecycleState next)
{
    if (next == OrynKernelLifecyclePanic)
    {
        return 1;
    }

    if (current == OrynKernelLifecyclePanic)
    {
        return next == OrynKernelLifecycleHalting;
    }

    if (current == next)
    {
        return 1;
    }

    if (!IsForwardLifecycleState(current) || !IsForwardLifecycleState(next))
    {
        return 0;
    }

    if (next == OrynKernelLifecycleInteractiveHalt)
    {
        return current == OrynKernelLifecycleRunning;
    }

    if (next == OrynKernelLifecycleHalting)
    {
        return current == OrynKernelLifecycleRunning ||
            current == OrynKernelLifecycleDebugExitRequested ||
            current == OrynKernelLifecycleInteractiveHalt;
    }

    return next > current;
}

static void RecordTransition(OrynKernelLifecycleState from, OrynKernelLifecycleState to)
{
    if (gLifecycle.TransitionCount < ORYN_LIFECYCLE_MAX_TRANSITIONS)
    {
        gTransitionFrom[gLifecycle.TransitionCount] = from;
        gTransitionTo[gLifecycle.TransitionCount] = to;
    }
}

static void PrintTransitionLine(OrynKernelLifecycleState from, OrynKernelLifecycleState to)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Lifecycle: ");
    OrynKernelDiagnosticsLogText(OrynKernelLifecycleStateName(from));
    OrynKernelDiagnosticsLogText(" -> ");
    OrynKernelDiagnosticsLogText(OrynKernelLifecycleStateName(to));
    OrynKernelDiagnosticsLogText("\n");
}

static void PrintInvalidTransitionLine(OrynKernelLifecycleState from, OrynKernelLifecycleState to)
{
    OrynKernelScreenReportBeginFail();
    OrynKernelDiagnosticsLogText("Lifecycle rejected transition ");
    OrynKernelDiagnosticsLogText(OrynKernelLifecycleStateName(from));
    OrynKernelDiagnosticsLogText(" -> ");
    OrynKernelDiagnosticsLogText(OrynKernelLifecycleStateName(to));
    OrynKernelDiagnosticsLogText("\n");
}

void OrynKernelLifecycleInit(void)
{
    gLifecycle.State = OrynKernelLifecycleCold;
    gLifecycle.TransitionCount = 0ULL;
    gLifecycle.InvalidTransitionCount = 0ULL;
    gLifecycle.HasEntered = 0;
    gLifecycle.HasBootInfo = 0;
    gLifecycle.HasDescriptors = 0;
    gLifecycle.HasInterrupts = 0;
    gLifecycle.HasTimers = 0;
    gLifecycle.HasConsole = 0;
    gLifecycle.HasMemory = 0;
    gLifecycle.HasVirtualMemory = 0;
    gLifecycle.HasRunning = 0;
    gLifecycle.HasDebugExitRequest = 0;
    gLifecycle.HasInteractiveHalt = 0;
    gLifecycle.HasHalting = 0;
    gLifecycle.HasHalted = 0;
    gLifecycle.HasPanic = 0;
    for (unsigned int index = 0; index < ORYN_LIFECYCLE_MAX_TRANSITIONS; ++index)
    {
        gTransitionFrom[index] = OrynKernelLifecycleCold;
        gTransitionTo[index] = OrynKernelLifecycleCold;
    }
    OrynKernelScreenReportOk(0, "Kernel lifecycle state machine initialized.");
}

OrynKernelLifecycleState OrynKernelLifecycleGetState(void)
{
    return gLifecycle.State;
}

int OrynKernelLifecycleTransition(OrynKernelLifecycleState nextState)
{
    OrynKernelLifecycleState previous = gLifecycle.State;
    if (!IsTransitionAllowed(previous, nextState))
    {
        gLifecycle.InvalidTransitionCount += 1ULL;
        PrintInvalidTransitionLine(previous, nextState);
        return 0;
    }

    if (previous != nextState)
    {
        RecordTransition(previous, nextState);
        gLifecycle.State = nextState;
        gLifecycle.TransitionCount += 1ULL;
        SetSeenFlag(nextState);
        PrintTransitionLine(previous, nextState);
    }

    return 1;
}

void OrynKernelLifecycleMarkPanic(const char* reason)
{
    (void)OrynKernelLifecycleTransition(OrynKernelLifecyclePanic);
    OrynKernelScreenReportBeginFail();
    OrynKernelDiagnosticsLogText("Kernel lifecycle panic: ");
    OrynKernelDiagnosticsLogText(reason == 0 ? "unspecified" : reason);
    OrynKernelDiagnosticsLogText("\n");
}

void OrynKernelLifecycleGetStatus(OrynKernelLifecycleStatus* status)
{
    if (status == 0)
    {
        return;
    }

    status->State = gLifecycle.State;
    status->TransitionCount = gLifecycle.TransitionCount;
    status->InvalidTransitionCount = gLifecycle.InvalidTransitionCount;
    status->HasEntered = gLifecycle.HasEntered;
    status->HasBootInfo = gLifecycle.HasBootInfo;
    status->HasDescriptors = gLifecycle.HasDescriptors;
    status->HasInterrupts = gLifecycle.HasInterrupts;
    status->HasTimers = gLifecycle.HasTimers;
    status->HasConsole = gLifecycle.HasConsole;
    status->HasMemory = gLifecycle.HasMemory;
    status->HasVirtualMemory = gLifecycle.HasVirtualMemory;
    status->HasRunning = gLifecycle.HasRunning;
    status->HasDebugExitRequest = gLifecycle.HasDebugExitRequest;
    status->HasInteractiveHalt = gLifecycle.HasInteractiveHalt;
    status->HasHalting = gLifecycle.HasHalting;
    status->HasHalted = gLifecycle.HasHalted;
    status->HasPanic = gLifecycle.HasPanic;
}

void OrynKernelLifecyclePrintProof(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Kernel lifecycle transition proof history:\n");
    unsigned long long count = gLifecycle.TransitionCount;
    if (count > ORYN_LIFECYCLE_MAX_TRANSITIONS)
    {
        count = ORYN_LIFECYCLE_MAX_TRANSITIONS;
    }

    for (unsigned long long index = 0ULL; index < count; ++index)
    {
        PrintTransitionLine(gTransitionFrom[index], gTransitionTo[index]);
    }

    OrynKernelDiagnosticsLogText("[KERNEL] Kernel lifecycle final state: ");
    OrynKernelDiagnosticsLogText(OrynKernelLifecycleStateName(gLifecycle.State));
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Kernel lifecycle transition count: ");
    OrynKernelDiagnosticsLogDec64(gLifecycle.TransitionCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Kernel lifecycle invalid transitions: ");
    OrynKernelDiagnosticsLogDec64(gLifecycle.InvalidTransitionCount);
    OrynKernelDiagnosticsLogText("\n");

    if (gLifecycle.HasEntered && gLifecycle.HasBootInfo &&
        gLifecycle.HasDescriptors && gLifecycle.HasInterrupts &&
        gLifecycle.HasTimers && gLifecycle.HasRunning &&
        gLifecycle.HasHalting && gLifecycle.HasHalted &&
        gLifecycle.InvalidTransitionCount == 0ULL)
    {
        OrynKernelScreenReportOk(0, "Kernel lifecycle state machine proof complete.");
    }
    else
    {
        OrynKernelScreenReportFail(0, "Kernel lifecycle state machine proof incomplete.");
    }
}
