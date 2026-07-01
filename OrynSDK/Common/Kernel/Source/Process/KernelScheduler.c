#include "KernelScheduler.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"
#include "string.h"

#define ORYN_SCHEDULER_DEFAULT_VECTOR 0xEFU

OrynKernelSchedulerState gScheduler;
static unsigned int gNextTimerId = 1U;
static unsigned long long gSchedulerJiffies;
static unsigned int gSchedulerProofWorkCount;

static void SchedulerProofWork(void* context)
{
    unsigned int* value = (unsigned int*)context;
    if (value != 0)
    {
        *value += 1U;
    }
    gSchedulerProofWorkCount += 1U;
}

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
    gScheduler.WorkQueueReady = 1U;
    gScheduler.WaitQueueReady = 1U;
    gScheduler.CpuCount = ClampCpuCount(cpuCount);
    OrynKernelSchedulerRoundRobinInit(gScheduler.CpuCount);
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

int OrynKernelSchedulerQueueDeviceWork(unsigned int deviceId,
    OrynKernelWorkRoutine routine, void* context, const char* name)
{
    if (gScheduler.Initialized == 0U || routine == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < ORYN_KERNEL_WORK_QUEUE_LIMIT; ++index)
    {
        OrynKernelWorkItem* item = &gScheduler.WorkQueue[index];
        if (item->Used == 0U)
        {
            item->Used = 1U;
            item->DeviceId = deviceId;
            item->Routine = routine;
            item->Context = context;
            item->Name = name;
            gScheduler.WorkQueued += 1U;
            return 1;
        }
    }
    return 0;
}

unsigned int OrynKernelSchedulerRunDeviceWork(unsigned int budget, unsigned int inInterruptContext)
{
    unsigned int ran = 0U;
    if (inInterruptContext)
    {
        gScheduler.WorkRejectedInInterrupt += 1U;
        return 0U;
    }
    if (budget == 0U)
    {
        budget = ORYN_KERNEL_WORK_QUEUE_LIMIT;
    }
    for (unsigned int index = 0U; index < ORYN_KERNEL_WORK_QUEUE_LIMIT && ran < budget; ++index)
    {
        OrynKernelWorkItem item = gScheduler.WorkQueue[index];
        if (item.Used && item.Routine != 0)
        {
            (void)memset(&gScheduler.WorkQueue[index], 0, sizeof(gScheduler.WorkQueue[index]));
            item.Routine(item.Context);
            gScheduler.WorkExecuted += 1U;
            ran += 1U;
        }
    }
    return ran;
}

int OrynKernelSchedulerWait(OrynKernelThread* thread, const void* channel, const char* reason)
{
    if (gScheduler.Initialized == 0U || thread == 0 || channel == 0)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < ORYN_KERNEL_WAIT_QUEUE_LIMIT; ++index)
    {
        OrynKernelWaitQueueNode* node = &gScheduler.WaitQueue[index];
        if (node->Used == 0U)
        {
            node->Used = 1U;
            node->WaitId = index + 1U;
            node->Thread = thread;
            node->Reason = reason;
            node->Channel = channel;
            thread->State = OrynKernelThreadStateBlocked;
            thread->SchedulerReady = 0U;
            thread->WaitChannel = channel;
            gScheduler.WaitQueueCount += 1U;
            return 1;
        }
    }
    return 0;
}

static unsigned int WakeMatchingWaiter(const void* channel, unsigned int all)
{
    unsigned int woken = 0U;
    for (unsigned int index = 0U; index < ORYN_KERNEL_WAIT_QUEUE_LIMIT; ++index)
    {
        OrynKernelWaitQueueNode* node = &gScheduler.WaitQueue[index];
        if (node->Used != 0U && (channel == 0 || node->Channel == channel))
        {
            OrynKernelThread* thread = node->Thread;
            (void)memset(node, 0, sizeof(*node));
            if (thread != 0)
            {
                thread->State = OrynKernelThreadStateSchedulerReady;
                thread->SchedulerReady = 1U;
                thread->WaitChannel = 0;
            }
            if (gScheduler.WaitQueueCount > 0U)
            {
                gScheduler.WaitQueueCount -= 1U;
            }
            gScheduler.WaitQueueWakeCount += 1U;
            woken += 1U;
            if (all == 0U)
            {
                return woken;
            }
        }
    }
    return woken;
}

int OrynKernelSchedulerWaitQueueSleep(OrynKernelThread* thread, const char* reason)
{
    return OrynKernelSchedulerWait(thread, (const void*)reason, reason);
}

unsigned int OrynKernelSchedulerWaitQueueWakeOne(const char* reason)
{
    return WakeMatchingWaiter((const void*)reason, 0U);
}

unsigned int OrynKernelSchedulerWakeOne(const void* channel)
{
    return WakeMatchingWaiter(channel, 0U);
}

unsigned int OrynKernelSchedulerWakeAll(const void* channel)
{
    return WakeMatchingWaiter(channel, 1U);
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
    if (!OrynKernelSchedulerRegisterIdleThread(1U, thread))
    {
        return 0;
    }
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
    unsigned int workValue = 0U;
    if (!OrynKernelSchedulerQueueDeviceWork(1U, SchedulerProofWork, &workValue, "proof-device-work"))
    {
        return 0;
    }
    if (OrynKernelSchedulerRunDeviceWork(4U, 1U) != 0U)
    {
        return 0;
    }
    if (OrynKernelSchedulerRunDeviceWork(4U, 0U) != 1U || workValue != 1U)
    {
        return 0;
    }
    static const unsigned int waitChannel = 1U;
    if (!OrynKernelSchedulerWait(thread, &waitChannel, "proof-wait"))
    {
        return 0;
    }
    if (OrynKernelSchedulerWakeOne(&waitChannel) != 1U)
    {
        return 0;
    }
    if (!OrynKernelSchedulerAddRunnableThread(1U, thread))
    {
        return 0;
    }
    OrynKernelSchedulerDumpRunQueue(0U);
    if (OrynKernelSchedulerPickNext(0U) != thread)
    {
        return 0;
    }
    if (OrynKernelSchedulerPreemptCurrent(0U) == 0)
    {
        return 0;
    }
    return thread->SchedulerReady == 0U && thread->State == OrynKernelThreadStateRunning &&
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
    OrynKernelScreenReportOkOrFail(gScheduler.WorkQueueReady && gScheduler.WorkExecuted > 0U &&
        gSchedulerProofWorkCount > 0U && gScheduler.WorkRejectedInInterrupt > 0U,
        "Scheduler work queues run device work outside interrupt context.",
        "Scheduler work queue proof failed.");
    OrynKernelScreenReportOkOrFail(gScheduler.WaitQueueReady && gScheduler.WaitQueueWakeCount > 0U,
        "Scheduler wait/wake primitives block and wake sleeping threads by channel.",
        "Scheduler wait/wake primitive proof failed.");
    const OrynKernelRoundRobinStats* rr = OrynKernelSchedulerGetRoundRobinStats();
    OrynKernelScreenReportOkOrFail(rr->RunQueueReady && rr->ThreadsEnqueued > 0U,
        "Per-CPU scheduler run queues are implemented.",
        "Per-CPU scheduler run queue proof failed.");
    OrynKernelScreenReportOkOrFail(rr->PreemptiveRoundRobinReady && rr->Preemptions > 0U,
        "Pre-emptive round-robin scheduler foundation is implemented.",
        "Pre-emptive round-robin scheduler proof failed.");
    OrynKernelScreenReportOkOrFail(rr->PrioritySchedulingReady && rr->PrioritySelections > 0U,
        "Scheduler priorities are layered after stable round-robin selection.",
        "Scheduler priority proof failed.");
    OrynKernelScreenReportOkOrFail(rr->IdleThreadReady && rr->IdleThreadCount > 0U,
        "Kernel idle thread registration exists for each CPU run queue.",
        "Kernel idle thread proof failed.");
    OrynKernelScreenReportOkOrFail(rr->CpuAffinityReady && rr->CpuAffinitySelections > 0U,
        "CPU affinity is enforced by the scheduler run-queue path.",
        "Scheduler CPU affinity proof failed.");
    OrynKernelScreenReportOkOrFail(rr->SchedulerDiagnosticsReady && rr->RunQueueDumpCount > 0U,
        "Scheduler diagnostics can dump per-CPU run queues.",
        "Scheduler run-queue diagnostic proof failed.");
}
