#ifndef ORYN_KERNEL_SCHEDULER_H
#define ORYN_KERNEL_SCHEDULER_H

#include "KernelProcess.h"

#define ORYN_KERNEL_SCHEDULER_CPU_LIMIT 32U
#define ORYN_KERNEL_TIMER_WHEEL_SLOTS 64U
#define ORYN_KERNEL_TIMER_LIMIT 128U
#define ORYN_KERNEL_WORK_QUEUE_LIMIT 64U
#define ORYN_KERNEL_WAIT_QUEUE_LIMIT 64U

typedef enum OrynKernelTimerState
{
    OrynKernelTimerStateFree = 0,
    OrynKernelTimerStateQueued = 1,
    OrynKernelTimerStateExpired = 2,
    OrynKernelTimerStateCancelled = 3
} OrynKernelTimerState;

typedef enum OrynKernelTimerKind
{
    OrynKernelTimerKindNone = 0,
    OrynKernelTimerKindSleep = 1,
    OrynKernelTimerKindCallback = 2
} OrynKernelTimerKind;

typedef struct OrynKernelTimerNode
{
    unsigned int Id;
    unsigned int State;
    unsigned int Kind;
    unsigned int Slot;
    unsigned long long DeadlineTick;
    OrynKernelThread* Thread;
    void (*Callback)(void* context);
    void* Context;
    struct OrynKernelTimerNode* Next;
} OrynKernelTimerNode;

typedef void (*OrynKernelWorkRoutine)(void* context);

typedef struct OrynKernelWorkItem
{
    unsigned int Used;
    unsigned int DeviceId;
    OrynKernelWorkRoutine Routine;
    void* Context;
    const char* Name;
} OrynKernelWorkItem;

typedef struct OrynKernelWaitQueueNode
{
    unsigned int Used;
    unsigned int WaitId;
    OrynKernelThread* Thread;
    const char* Reason;
} OrynKernelWaitQueueNode;

typedef struct OrynKernelCpuSchedulerTick
{
    unsigned int Registered;
    unsigned int CpuId;
    unsigned int TickVector;
    unsigned long long TickCount;
    unsigned long long LastTick;
} OrynKernelCpuSchedulerTick;

typedef struct OrynKernelSchedulerState
{
    unsigned int Initialized;
    unsigned int TimerWheelReady;
    unsigned int SleepQueueReady;
    unsigned int PerCpuTickReady;
    unsigned int CpuCount;
    unsigned int TimerCount;
    unsigned int QueuedTimers;
    unsigned int ExpiredTimers;
    unsigned int SleepQueueCount;
    unsigned int WakeCount;
    unsigned int BusyWaitSleepRejected;
    unsigned int WorkQueueReady;
    unsigned int WorkQueued;
    unsigned int WorkExecuted;
    unsigned int WorkRejectedInInterrupt;
    unsigned int WaitQueueReady;
    unsigned int WaitQueueCount;
    unsigned int WaitQueueWakeCount;
    unsigned int FailedTimerAllocations;
    unsigned int JiffiesInternalOnly;
    unsigned long long CurrentTick;
    unsigned long long InternalJiffies;
    OrynKernelTimerNode Timers[ORYN_KERNEL_TIMER_LIMIT];
    OrynKernelTimerNode* Wheel[ORYN_KERNEL_TIMER_WHEEL_SLOTS];
    OrynKernelCpuSchedulerTick CpuTicks[ORYN_KERNEL_SCHEDULER_CPU_LIMIT];
    OrynKernelWorkItem WorkQueue[ORYN_KERNEL_WORK_QUEUE_LIMIT];
    OrynKernelWaitQueueNode WaitQueue[ORYN_KERNEL_WAIT_QUEUE_LIMIT];
} OrynKernelSchedulerState;

void OrynKernelSchedulerInit(unsigned int cpuCount, unsigned int tickVector);
int OrynKernelSchedulerRegisterPerCpuTick(unsigned int cpuId, unsigned int tickVector);
int OrynKernelSchedulerSleepUntil(OrynKernelThread* thread, unsigned long long deadlineTick);
int OrynKernelSchedulerQueueTimer(unsigned long long deadlineTick,
    void (*callback)(void* context), void* context);
int OrynKernelSchedulerQueueDeviceWork(unsigned int deviceId,
    OrynKernelWorkRoutine routine, void* context, const char* name);
unsigned int OrynKernelSchedulerRunDeviceWork(unsigned int budget, unsigned int inInterruptContext);
int OrynKernelSchedulerWaitQueueSleep(OrynKernelThread* thread, const char* reason);
unsigned int OrynKernelSchedulerWaitQueueWakeOne(const char* reason);
void OrynKernelSchedulerTick(unsigned int cpuId, unsigned long long nowTick);
const OrynKernelSchedulerState* OrynKernelSchedulerGetState(void);
int OrynKernelSchedulerRunSelfTest(OrynKernelThread* thread);
void OrynKernelSchedulerPrintProof(void);

#endif
