#include "KernelProcess.h"

extern OrynKernelProcessStats gProcessStats;

int OrynKernelProcessExit(OrynKernelProcess* process, int exitStatus)
{
    if (process == 0)
    {
        return 0;
    }
    process->ExitStatus = exitStatus;
    process->Exited = 1U;
    process->State = OrynKernelProcessStateZombie;
    gProcessStats.ProcessExitCount += 1U;
    return 1;
}

int OrynKernelProcessWait(OrynKernelProcess* parent, unsigned int childProcessId, int* exitStatus)
{
    OrynKernelProcess* child;
    if (parent == 0)
    {
        gProcessStats.FailedWaits += 1U;
        return 0;
    }
    child = parent->FirstChild;
    while (child != 0)
    {
        if ((childProcessId == 0U || child->ProcessId == childProcessId) && child->Exited)
        {
            if (exitStatus != 0)
            {
                *exitStatus = child->ExitStatus;
            }
            child->Waited = 1U;
            gProcessStats.ProcessWaitCount += 1U;
            return 1;
        }
        child = child->NextSibling;
    }
    gProcessStats.FailedWaits += 1U;
    return 0;
}

int OrynKernelThreadExit(OrynKernelThread* thread, int exitStatus)
{
    if (thread == 0)
    {
        return 0;
    }
    thread->ExitStatus = exitStatus;
    thread->Exited = 1U;
    thread->State = OrynKernelThreadStateZombie;
    thread->SchedulerReady = 0U;
    return 1;
}

int OrynKernelThreadStop(OrynKernelThread* thread)
{
    if (thread == 0)
    {
        return 0;
    }
    thread->State = OrynKernelThreadStateStopped;
    thread->SchedulerReady = 0U;
    return 1;
}

int OrynKernelThreadSetPriority(OrynKernelThread* thread, unsigned int priority)
{
    if (thread == 0 || priority > ORYN_KERNEL_THREAD_PRIORITY_MAX)
    {
        return 0;
    }
    thread->Priority = priority;
    return 1;
}

