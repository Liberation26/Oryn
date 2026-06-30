#ifndef ORYN_KERNEL_LIFECYCLE_H
#define ORYN_KERNEL_LIFECYCLE_H

typedef enum OrynKernelLifecycleState
{
    OrynKernelLifecycleCold = 0,
    OrynKernelLifecycleEntered = 1,
    OrynKernelLifecycleBootInfoAdopted = 2,
    OrynKernelLifecycleDescriptorsReady = 3,
    OrynKernelLifecycleInterruptsReady = 4,
    OrynKernelLifecycleTimersReady = 5,
    OrynKernelLifecycleConsoleReady = 6,
    OrynKernelLifecycleMemoryReady = 7,
    OrynKernelLifecycleVirtualMemoryReady = 8,
    OrynKernelLifecycleRunning = 9,
    OrynKernelLifecycleDebugExitRequested = 10,
    OrynKernelLifecycleInteractiveHalt = 11,
    OrynKernelLifecycleHalting = 12,
    OrynKernelLifecycleHalted = 13,
    OrynKernelLifecyclePanic = 14
} OrynKernelLifecycleState;

typedef struct OrynKernelLifecycleStatus
{
    OrynKernelLifecycleState State;
    unsigned long long TransitionCount;
    unsigned long long InvalidTransitionCount;
    int HasEntered;
    int HasBootInfo;
    int HasDescriptors;
    int HasInterrupts;
    int HasTimers;
    int HasConsole;
    int HasMemory;
    int HasVirtualMemory;
    int HasRunning;
    int HasDebugExitRequest;
    int HasInteractiveHalt;
    int HasHalting;
    int HasHalted;
    int HasPanic;
} OrynKernelLifecycleStatus;

void OrynKernelLifecycleInit(void);
OrynKernelLifecycleState OrynKernelLifecycleGetState(void);
const char* OrynKernelLifecycleStateName(OrynKernelLifecycleState state);
int OrynKernelLifecycleTransition(OrynKernelLifecycleState nextState);
void OrynKernelLifecycleMarkPanic(const char* reason);
void OrynKernelLifecycleGetStatus(OrynKernelLifecycleStatus* status);
void OrynKernelLifecyclePrintProof(void);

#endif
