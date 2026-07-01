#include "KernelScheduler.h"
#include "KernelContextSwitch.h"
#include "KernelDiagnosticsLogger.h"
#include "string.h"

static OrynKernelRoundRobinStats gRoundRobin;

static unsigned int ClampCpu(unsigned int cpuId)
{
    if (cpuId >= ORYN_KERNEL_SCHEDULER_CPU_LIMIT)
    {
        return 0U;
    }
    return cpuId;
}

static void RoundRobinInit(unsigned int cpuCount)
{
    (void)memset(&gRoundRobin, 0, sizeof(gRoundRobin));
    if (cpuCount == 0U)
    {
        cpuCount = 1U;
    }
    if (cpuCount > ORYN_KERNEL_SCHEDULER_CPU_LIMIT)
    {
        cpuCount = ORYN_KERNEL_SCHEDULER_CPU_LIMIT;
    }
    gRoundRobin.Initialized = 1U;
    gRoundRobin.CpuCount = cpuCount;
    gRoundRobin.RunQueueReady = 1U;
    gRoundRobin.PreemptiveRoundRobinReady = 1U;
    gRoundRobin.PrioritySchedulingReady = 1U;
    gRoundRobin.CpuAffinityReady = 1U;
    gRoundRobin.SchedulerDiagnosticsReady = 1U;
    gRoundRobin.QuantumTicks = ORYN_KERNEL_ROUND_ROBIN_QUANTUM_TICKS;
    for (unsigned int cpu = 0U; cpu < cpuCount; ++cpu)
    {
        gRoundRobin.RunQueues[cpu].Ready = 1U;
        gRoundRobin.RunQueues[cpu].CpuId = cpu;
    }
}

static OrynKernelRunQueue* GetQueue(unsigned int cpuId)
{
    cpuId = ClampCpu(cpuId);
    if (gRoundRobin.Initialized == 0U)
    {
        RoundRobinInit(cpuId + 1U);
    }
    return &gRoundRobin.RunQueues[cpuId];
}

int OrynKernelSchedulerAddRunnableThread(unsigned int cpuId, OrynKernelThread* thread)
{
    OrynKernelRunQueue* queue = GetQueue(cpuId);
    if (thread == 0 || queue->Count >= ORYN_KERNEL_RUN_QUEUE_LIMIT)
    {
        return 0;
    }
    queue->Threads[queue->Tail] = thread;
    queue->Tail = (queue->Tail + 1U) % ORYN_KERNEL_RUN_QUEUE_LIMIT;
    queue->Count += 1U;
    if (thread->AffinitySet != 0U &&
        (thread->CpuAffinityMask & (1U << queue->CpuId)) == 0U)
    {
        unsigned int targetCpu;
        for (targetCpu = 0U; targetCpu < gRoundRobin.CpuCount; ++targetCpu)
        {
            if ((thread->CpuAffinityMask & (1U << targetCpu)) != 0U)
            {
                queue = GetQueue(targetCpu);
                gRoundRobin.CpuAffinitySelections += 1U;
                break;
            }
        }
        if (targetCpu >= gRoundRobin.CpuCount)
        {
            return 0;
        }
    }
    thread->AssignedCpu = queue->CpuId;
    thread->RemainingQuantumTicks = ORYN_KERNEL_ROUND_ROBIN_QUANTUM_TICKS;
    thread->State = OrynKernelThreadStateSchedulerReady;
    thread->SchedulerReady = 1U;
    gRoundRobin.ThreadsEnqueued += 1U;
    return 1;
}

static OrynKernelThread* PopQueueHead(OrynKernelRunQueue* queue)
{
    OrynKernelThread* thread = queue->Threads[queue->Head];
    queue->Threads[queue->Head] = 0;
    queue->Head = (queue->Head + 1U) % ORYN_KERNEL_RUN_QUEUE_LIMIT;
    queue->Count -= 1U;
    return thread;
}

OrynKernelThread* OrynKernelSchedulerPickNext(unsigned int cpuId)
{
    OrynKernelRunQueue* queue = GetQueue(cpuId);
    OrynKernelThread* best = 0;
    unsigned int originalCount = queue->Count;
    if (queue->Count == 0U)
    {
        return gRoundRobin.IdleThreads[queue->CpuId];
    }
    for (unsigned int seen = 0U; seen < originalCount; ++seen)
    {
        OrynKernelThread* candidate = PopQueueHead(queue);
        if (candidate == 0)
        {
            continue;
        }
        if (best == 0 || candidate->Priority > best->Priority)
        {
            if (best != 0)
            {
                (void)OrynKernelSchedulerAddRunnableThread(queue->CpuId, best);
            }
            best = candidate;
        }
        else
        {
            (void)OrynKernelSchedulerAddRunnableThread(queue->CpuId, candidate);
        }
    }
    gRoundRobin.ThreadsDequeued += 1U;
    gRoundRobin.PrioritySelections += 1U;
    gRoundRobin.Current[queue->CpuId] = best;
    if (best != 0)
    {
        best->State = OrynKernelThreadStateRunning;
        best->SchedulerReady = 0U;
    }
    return best;
}

int OrynKernelSchedulerRegisterIdleThread(unsigned int cpuId, OrynKernelThread* thread)
{
    unsigned int cpu = ClampCpu(cpuId);
    if (thread == 0)
    {
        return 0;
    }
    if (gRoundRobin.Initialized == 0U)
    {
        RoundRobinInit(cpu + 1U);
    }
    thread->AssignedCpu = cpu;
    thread->Priority = ORYN_KERNEL_THREAD_PRIORITY_MIN;
    thread->State = OrynKernelThreadStateRunning;
    thread->SchedulerReady = 0U;
    gRoundRobin.IdleThreads[cpu] = thread;
    gRoundRobin.IdleThreadReady = 1U;
    gRoundRobin.IdleThreadCount += 1U;
    return 1;
}

OrynKernelThread* OrynKernelSchedulerPreemptCurrent(unsigned int cpuId)
{
    unsigned int cpu = ClampCpu(cpuId);
    OrynKernelThread* current;
    OrynKernelThread* next;
    if (gRoundRobin.Initialized == 0U)
    {
        RoundRobinInit(cpu + 1U);
    }
    current = gRoundRobin.Current[cpu];
    if (current != 0)
    {
        current->State = OrynKernelThreadStateSchedulerReady;
        current->SchedulerReady = 1U;
        current->PreemptionCount += 1ULL;
        (void)OrynKernelSchedulerAddRunnableThread(cpu, current);
        gRoundRobin.Preemptions += 1U;
    }
    next = OrynKernelSchedulerPickNext(cpu);
    if (next != 0 && current != 0)
    {
        OrynKernelX64ContextSwitch(&current->CpuContext, &next->CpuContext);
        gRoundRobin.ContextSwitchRequests += 1U;
    }
    return next;
}

const OrynKernelRoundRobinStats* OrynKernelSchedulerGetRoundRobinStats(void)
{
    return &gRoundRobin;
}

void OrynKernelSchedulerDumpRunQueue(unsigned int cpuId)
{
    OrynKernelRunQueue* queue = GetQueue(cpuId);
    gRoundRobin.RunQueueDumpCount += 1U;
    OrynKernelDiagnosticsLogText("[KERNEL] Scheduler run queue CPU ");
    OrynKernelDiagnosticsLogDec64(queue->CpuId);
    OrynKernelDiagnosticsLogText(": count ");
    OrynKernelDiagnosticsLogDec64(queue->Count);
    OrynKernelDiagnosticsLogText(" head ");
    OrynKernelDiagnosticsLogDec64(queue->Head);
    OrynKernelDiagnosticsLogText(" tail ");
    OrynKernelDiagnosticsLogDec64(queue->Tail);
    OrynKernelDiagnosticsLogText("\n");
    for (unsigned int index = 0U; index < queue->Count; ++index)
    {
        unsigned int slot = (queue->Head + index) % ORYN_KERNEL_RUN_QUEUE_LIMIT;
        OrynKernelThread* thread = queue->Threads[slot];
        if (thread != 0)
        {
            OrynKernelDiagnosticsLogText("[KERNEL]   tid ");
            OrynKernelDiagnosticsLogDec64(thread->ThreadId);
            OrynKernelDiagnosticsLogText(" state ");
            OrynKernelDiagnosticsLogDec64(thread->State);
            OrynKernelDiagnosticsLogText(" priority ");
            OrynKernelDiagnosticsLogDec64(thread->Priority);
            OrynKernelDiagnosticsLogText(" affinity ");
            OrynKernelDiagnosticsLogDec64(thread->CpuAffinityMask);
            OrynKernelDiagnosticsLogText("\n");
        }
    }
}
