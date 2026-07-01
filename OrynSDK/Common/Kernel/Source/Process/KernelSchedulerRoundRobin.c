#include "KernelScheduler.h"
#include "KernelContextSwitch.h"
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
    thread->AssignedCpu = queue->CpuId;
    thread->RemainingQuantumTicks = ORYN_KERNEL_ROUND_ROBIN_QUANTUM_TICKS;
    gRoundRobin.ThreadsEnqueued += 1U;
    return 1;
}

OrynKernelThread* OrynKernelSchedulerPickNext(unsigned int cpuId)
{
    OrynKernelRunQueue* queue = GetQueue(cpuId);
    OrynKernelThread* thread;
    if (queue->Count == 0U)
    {
        return 0;
    }
    thread = queue->Threads[queue->Head];
    queue->Threads[queue->Head] = 0;
    queue->Head = (queue->Head + 1U) % ORYN_KERNEL_RUN_QUEUE_LIMIT;
    queue->Count -= 1U;
    gRoundRobin.ThreadsDequeued += 1U;
    gRoundRobin.Current[queue->CpuId] = thread;
    if (thread != 0)
    {
        thread->State = OrynKernelThreadStateRunning;
        thread->SchedulerReady = 0U;
    }
    return thread;
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
