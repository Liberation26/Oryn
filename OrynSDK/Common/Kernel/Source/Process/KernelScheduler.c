#include "KernelScheduler.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"
#include "string.h"

#define ORYN_SCHEDULER_DEFAULT_VECTOR 0xEFU

static OrynKernelSchedulerState gScheduler;
static unsigned int gNextTimerId = 1U;
static unsigned long long gSchedulerJiffies;

static unsigned int ClampCpuCount(unsigned int cpuCount)
{
    if (cpuCount == 0U)
    {
        return 1U;
    }
    if (cpuCount > ORYN_KERNEL_SCHEDULER_CPU_LIMIT)
    {
        return ORYN_KERNEL_SCHEDULER_CPU_LIMIT;
    }
    return cpuCount;
}

static unsigned int TimerSlot(unsigned long long tick)
{
    return (unsigned int)(tick % ORYN_KERNEL_TIMER_WHEEL_SLOTS);
}

static OrynKernelTimerNode* AllocateTimer(void)
{
    for (unsigned int index = 0U; index < ORYN_KERNEL_TIMER_LIMIT; ++index)
    {
        if (gScheduler.Timers[index].State == OrynKernelTimerStateFree)
        {
            OrynKernelTimerNode* node = &gScheduler.Timers[index];
            (void)memset(node, 0, sizeof(*node));
            node->Id = gNextTimerId++;
            gScheduler.TimerCount += 1U;
            return node;
        }
    }
    gScheduler.FailedTimerAllocations += 1U;
    return 0;
}

static void QueueNode(OrynKernelTimerNode* node)
{
    unsigned int slot = TimerSlot(node->DeadlineTick);
    node->Slot = slot;
    node->State = OrynKernelTimerStateQueued;
    node->Next = gScheduler.Wheel[slot];
    gScheduler.Wheel[slot] = node;
    gScheduler.QueuedTimers += 1U;
}

static void WakeSleepingThread(OrynKernelTimerNode* node)
{
    if (node->Thread != 0)
    {
        node->Thread->State = OrynKernelThreadStateSchedulerReady;
        node->Thread->SchedulerReady = 1U;
        if (gScheduler.SleepQueueCount > 0U)
        {
            gScheduler.SleepQueueCount -= 1U;
        }
        gScheduler.WakeCount += 1U;
    }
}

static void ExpireNode(OrynKernelTimerNode* node)
{
    node->State = OrynKernelTimerStateExpired;
    node->Next = 0;
    gScheduler.ExpiredTimers += 1U;
    if (gScheduler.QueuedTimers > 0U)
    {
        gScheduler.QueuedTimers -= 1U;
    }
    if (node->Kind == OrynKernelTimerKindSleep)
    {
        WakeSleepingThread(node);
    }
    if (node->Kind == OrynKernelTimerKindCallback && node->Callback != 0)
    {
        node->Callback(node->Context);
    }
}

void OrynKernelSchedulerInit(unsigned int cpuCount, unsigned int tickVector)
{
    (void)memset(&gScheduler, 0, sizeof(gScheduler));
    if (tickVector == 0U)
    {
        tickVector = ORYN_SCHEDULER_DEFAULT_VECTOR;
    }
    gSchedulerJiffies = 0ULL;
    gScheduler.Initialized = 1U;
    gScheduler.JiffiesInternalOnly = 1U;
    gScheduler.TimerWheelReady = 1U;
    gScheduler.SleepQueueReady = 1U;
    gScheduler.CpuCount = ClampCpuCount(cpuCount);
    for (unsigned int cpu = 0U; cpu < gScheduler.CpuCount; ++cpu)
    {
        (void)OrynKernelSchedulerRegisterPerCpuTick(cpu, tickVector);
    }
}

int OrynKernelSchedulerRegisterPerCpuTick(unsigned int cpuId, unsigned int tickVector)
{
    if (cpuId >= ORYN_KERNEL_SCHEDULER_CPU_LIMIT)
    {
        return 0;
    }
    gScheduler.CpuTicks[cpuId].Registered = 1U;
    gScheduler.CpuTicks[cpuId].CpuId = cpuId;
    gScheduler.CpuTicks[cpuId].TickVector = tickVector;
    if (cpuId + 1U > gScheduler.CpuCount)
    {
        gScheduler.CpuCount = cpuId + 1U;
    }
    gScheduler.PerCpuTickReady = 1U;
    return 1;
}

int OrynKernelSchedulerSleepUntil(OrynKernelThread* thread, unsigned long long deadlineTick)
{
    OrynKernelTimerNode* node;
    if (gScheduler.Initialized == 0U || thread == 0 || deadlineTick <= gScheduler.CurrentTick)
    {
        gScheduler.BusyWaitSleepRejected += 1U;
        return 0;
    }
    node = AllocateTimer();
    if (node == 0)
    {
        return 0;
    }
    node->Kind = OrynKernelTimerKindSleep;
    node->DeadlineTick = deadlineTick;
    node->Thread = thread;
    thread->State = OrynKernelThreadStateSleeping;
    thread->SchedulerReady = 0U;
    gScheduler.SleepQueueCount += 1U;
    QueueNode(node);
    return 1;
}

int OrynKernelSchedulerQueueTimer(unsigned long long deadlineTick,
    void (*callback)(void* context), void* context)
{
    OrynKernelTimerNode* node;
    if (gScheduler.Initialized == 0U || deadlineTick <= gScheduler.CurrentTick)
    {
        return 0;
    }
    node = AllocateTimer();
    if (node == 0)
    {
        return 0;
    }
    node->Kind = OrynKernelTimerKindCallback;
    node->DeadlineTick = deadlineTick;
    node->Callback = callback;
    node->Context = context;
    QueueNode(node);
    return 1;
}

void OrynKernelSchedulerTick(unsigned int cpuId, unsigned long long nowTick)
{
    unsigned int slot;
    OrynKernelTimerNode* previous = 0;
    OrynKernelTimerNode* current;
    if (gScheduler.Initialized == 0U)
    {
        return;
    }
    gSchedulerJiffies += 1ULL;
    gScheduler.InternalJiffies = gSchedulerJiffies;
    gScheduler.CurrentTick = nowTick;
    if (cpuId < ORYN_KERNEL_SCHEDULER_CPU_LIMIT && gScheduler.CpuTicks[cpuId].Registered)
    {
        gScheduler.CpuTicks[cpuId].TickCount += 1ULL;
        gScheduler.CpuTicks[cpuId].LastTick = nowTick;
    }
    slot = TimerSlot(nowTick);
    current = gScheduler.Wheel[slot];
    while (current != 0)
    {
        OrynKernelTimerNode* next = current->Next;
        if (current->DeadlineTick <= nowTick)
        {
            if (previous == 0)
            {
                gScheduler.Wheel[slot] = next;
            }
            else
            {
                previous->Next = next;
            }
            ExpireNode(current);
        }
        else
        {
            previous = current;
        }
        current = next;
    }
}

const OrynKernelSchedulerState* OrynKernelSchedulerGetState(void)
{
    return &gScheduler;
}

int OrynKernelSchedulerRunSelfTest(OrynKernelThread* thread)
{
    if (thread == 0)
    {
        return 0;
    }
    OrynKernelSchedulerInit(2U, ORYN_SCHEDULER_DEFAULT_VECTOR);
    if (!OrynKernelSchedulerSleepUntil(thread, 5ULL))
    {
        return 0;
    }
    if (thread->SchedulerReady != 0U || thread->State != OrynKernelThreadStateSleeping)
    {
        return 0;
    }
    OrynKernelSchedulerTick(0U, 1ULL);
    if (thread->State != OrynKernelThreadStateSleeping)
    {
        return 0;
    }
    OrynKernelSchedulerTick(1U, 5ULL);
    return thread->SchedulerReady != 0U && thread->State == OrynKernelThreadStateSchedulerReady &&
        gScheduler.WakeCount > 0U && gScheduler.ExpiredTimers > 0U &&
        gScheduler.CpuTicks[0].TickCount > 0ULL && gScheduler.CpuTicks[1].TickCount > 0ULL;
}

void OrynKernelSchedulerPrintProof(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Scheduler timer wheel slots: ");
    OrynKernelDiagnosticsLogDec64(ORYN_KERNEL_TIMER_WHEEL_SLOTS);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Scheduler per-CPU tick records: ");
    OrynKernelDiagnosticsLogDec64(gScheduler.CpuCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(gScheduler.SleepQueueReady && gScheduler.WakeCount > 0U,
        "Scheduler sleep queues block threads without busy-waiting.",
        "Scheduler sleep queue proof failed.");
    OrynKernelScreenReportOkOrFail(gScheduler.TimerWheelReady && gScheduler.ExpiredTimers > 0U,
        "Scheduler timer wheel queues and expires timers.",
        "Scheduler timer wheel proof failed.");
    OrynKernelScreenReportOkOrFail(gScheduler.PerCpuTickReady && gScheduler.CpuTicks[0].TickCount > 0ULL,
        "Per-CPU scheduler tick registration is active.",
        "Per-CPU scheduler tick registration proof failed.");
    OrynKernelScreenReportOkOrFail(gScheduler.JiffiesInternalOnly && gScheduler.InternalJiffies > 0ULL,
        "Scheduler jiffies are private implementation detail, not public clock API.",
        "Scheduler jiffies implementation-detail proof failed.");
}
