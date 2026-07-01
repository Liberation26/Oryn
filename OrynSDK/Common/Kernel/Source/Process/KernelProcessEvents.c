#include "KernelProcess.h"
#include "string.h"

extern OrynKernelProcessStats gProcessStats;

static int QueueProcessEvent(OrynKernelProcessEvent* queue,
    unsigned int* count,
    unsigned int eventCode,
    unsigned long long eventValue,
    unsigned int sourceProcessId,
    unsigned int sourceThreadId)
{
    if (queue == 0 || count == 0 || *count >= ORYN_KERNEL_PROCESS_EVENT_QUEUE_LIMIT)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < ORYN_KERNEL_PROCESS_EVENT_QUEUE_LIMIT; ++index)
    {
        if (queue[index].Used == 0U)
        {
            queue[index].Used = 1U;
            queue[index].EventCode = eventCode;
            queue[index].EventValue = eventValue;
            queue[index].SourceProcessId = sourceProcessId;
            queue[index].SourceThreadId = sourceThreadId;
            *count += 1U;
            gProcessStats.DeliveredEventCount += 1U;
            return 1;
        }
    }
    return 0;
}

int OrynKernelProcessSendEvent(OrynKernelProcess* target,
    unsigned int eventCode,
    unsigned long long eventValue)
{
    if (target == 0 || eventCode == 0U)
    {
        return 0;
    }
    gProcessStats.OrynEventDeliveryReady = 1U;
    return QueueProcessEvent(target->Events, &target->PendingEventCount,
        eventCode, eventValue, 0U, 0U);
}

int OrynKernelThreadSendEvent(OrynKernelThread* target,
    unsigned int eventCode,
    unsigned long long eventValue)
{
    unsigned int sourceProcessId = 0U;
    if (target == 0 || eventCode == 0U)
    {
        return 0;
    }
    if (target->OwnerProcess != 0)
    {
        sourceProcessId = target->OwnerProcess->ProcessId;
    }
    gProcessStats.OrynEventDeliveryReady = 1U;
    return QueueProcessEvent(target->Events, &target->PendingEventCount,
        eventCode, eventValue, sourceProcessId, target->ThreadId);
}

int OrynKernelThreadReceiveEvent(OrynKernelThread* thread,
    OrynKernelProcessEvent* eventOut)
{
    if (thread == 0 || eventOut == 0 || thread->PendingEventCount == 0U)
    {
        return 0;
    }
    for (unsigned int index = 0U; index < ORYN_KERNEL_PROCESS_EVENT_QUEUE_LIMIT; ++index)
    {
        if (thread->Events[index].Used != 0U)
        {
            *eventOut = thread->Events[index];
            (void)memset(&thread->Events[index], 0, sizeof(thread->Events[index]));
            thread->PendingEventCount -= 1U;
            gProcessStats.ReceivedEventCount += 1U;
            return 1;
        }
    }
    return 0;
}

int OrynKernelThreadSetCpuAffinity(OrynKernelThread* thread, unsigned int affinityMask)
{
    if (thread == 0 || affinityMask == 0U)
    {
        return 0;
    }
    thread->CpuAffinityMask = affinityMask;
    thread->AffinitySet = 1U;
    gProcessStats.CpuAffinityReady = 1U;
    gProcessStats.CpuAffinitySetCount += 1U;
    return 1;
}
