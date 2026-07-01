
#ifndef ORYN_KERNEL_PROCESS_H
#define ORYN_KERNEL_PROCESS_H

#include "KernelHeap.h"
#include "KernelVirtualMemory.h"
#include "KernelContextSwitch.h"

#define ORYN_KERNEL_PROCESS_NAME_LENGTH 32U
#define ORYN_KERNEL_THREAD_NAME_LENGTH 32U
#define ORYN_KERNEL_THREAD_DEFAULT_STACK_BYTES 16384ULL
#define ORYN_KERNEL_THREAD_STACK_GUARD_BYTES 4096ULL
#define ORYN_KERNEL_THREAD_PRIORITY_MIN 0U
#define ORYN_KERNEL_THREAD_PRIORITY_DEFAULT 8U
#define ORYN_KERNEL_THREAD_PRIORITY_MAX 15U
#define ORYN_KERNEL_PROCESS_EVENT_QUEUE_LIMIT 8U

typedef enum OrynKernelProcessKind
{
    OrynKernelProcessKindKernel = 1,
    OrynKernelProcessKindUser = 2
} OrynKernelProcessKind;

typedef enum OrynKernelProcessState
{
    OrynKernelProcessStateUnused = 0,
    OrynKernelProcessStateCreated = 1,
    OrynKernelProcessStateReady = 2,
    OrynKernelProcessStateRunning = 3,
    OrynKernelProcessStateZombie = 4,
    OrynKernelProcessStateStopped = 5,
    OrynKernelProcessStateTerminated = 4
} OrynKernelProcessState;

typedef enum OrynKernelThreadState
{
    OrynKernelThreadStateUnused = 0,
    OrynKernelThreadStateCreated = 1,
    OrynKernelThreadStateReady = 2,
    OrynKernelThreadStateSchedulerReady = 2,
    OrynKernelThreadStateRunning = 3,
    OrynKernelThreadStateSleeping = 4,
    OrynKernelThreadStateBlocked = 5,
    OrynKernelThreadStateZombie = 6,
    OrynKernelThreadStateStopped = 7,
    OrynKernelThreadStateTerminated = 6
} OrynKernelThreadState;

typedef struct OrynKernelProcessEvent
{
    unsigned int Used;
    unsigned int EventCode;
    unsigned long long EventValue;
    unsigned int SourceProcessId;
    unsigned int SourceThreadId;
} OrynKernelProcessEvent;

typedef struct OrynKernelProcess
{
    unsigned int ProcessId;
    unsigned int ParentProcessId;
    unsigned int State;
    unsigned int Kind;
    unsigned int UserMode;
    unsigned int ThreadCount;
    int ExitStatus;
    unsigned int Exited;
    unsigned int Waited;
    struct OrynKernelProcess* Parent;
    struct OrynKernelProcess* FirstChild;
    struct OrynKernelProcess* NextSibling;
    unsigned int ChildCount;
    unsigned int PendingEventCount;
    OrynKernelProcessEvent Events[ORYN_KERNEL_PROCESS_EVENT_QUEUE_LIMIT];
    char Name[ORYN_KERNEL_PROCESS_NAME_LENGTH];
    OrynKernelAddressSpace* AddressSpace;
} OrynKernelProcess;

typedef struct OrynKernelThread
{
    unsigned int ThreadId;
    unsigned int State;
    OrynKernelProcess* OwnerProcess;
    OrynKernelCpuContext CpuContext;
    void (*EntryPoint)(void* context);
    void* Context;
    void* StackBase;
    void* StackTop;
    unsigned long long StackBytes;
    unsigned long long GuardBytes;
    unsigned int SchedulerReady;
    unsigned int IsUserThread;
    unsigned int Priority;
    int ExitStatus;
    unsigned int Exited;
    const void* WaitChannel;
    unsigned int PendingEventCount;
    OrynKernelProcessEvent Events[ORYN_KERNEL_PROCESS_EVENT_QUEUE_LIMIT];
    unsigned int CpuAffinityMask;
    unsigned int AffinitySet;
    unsigned int AssignedCpu;
    unsigned int QuantumTicks;
    unsigned int RemainingQuantumTicks;
    unsigned long long PreemptionCount;
    char Name[ORYN_KERNEL_THREAD_NAME_LENGTH];
} OrynKernelThread;


typedef struct OrynKernelUserProcess
{
    OrynKernelProcess* Process;
    unsigned int UserProcessId;
    OrynKernelAddressSpace* AddressSpace;
} OrynKernelUserProcess;

typedef struct OrynKernelUserThread
{
    OrynKernelThread* Thread;
    OrynKernelUserProcess* UserProcess;
    unsigned long long UserStackTop;
    unsigned long long UserEntry;
} OrynKernelUserThread;

typedef struct OrynKernelProcessStats
{
    unsigned int Initialized;
    unsigned int ProcessCreatedCount;
    unsigned int ThreadCreatedCount;
    unsigned int KernelThreadStructureReady;
    unsigned int UserProcessStructureReady;
    unsigned int UserThreadStructureReady;
    unsigned int SchedulerReadyThreadCount;
    unsigned int KernelThreadStackCount;
    unsigned long long KernelThreadStackBytes;
    unsigned long long KernelThreadGuardBytes;
    unsigned int AddressSpaceBoundProcessCount;
    unsigned int CopyOnWriteChildProcessCount;
    unsigned int ProcessIdReady;
    unsigned int ThreadIdReady;
    unsigned int ParentChildReady;
    unsigned int ExitWaitReady;
    unsigned int OrynEventDeliveryReady;
    unsigned int DeliveredEventCount;
    unsigned int ReceivedEventCount;
    unsigned int CpuAffinityReady;
    unsigned int CpuAffinitySetCount;
    unsigned int ProcessExitCount;
    unsigned int ProcessWaitCount;
    unsigned int ThreadStateReady;
    unsigned int FailedWaits;
    unsigned long long CopyOnWriteSharedPages;
    unsigned long long CopyOnWriteResolvedPages;
    unsigned int FailedAllocations;
} OrynKernelProcessStats;

void OrynKernelProcessSystemInit(void);
OrynKernelProcess* OrynKernelProcessCreate(
    OrynKernelPhysicalMemory* physicalMemory,
    const char* name,
    unsigned int parentProcessId);
void OrynKernelProcessDestroy(OrynKernelProcess* process);
OrynKernelProcess* OrynKernelProcessCreateCopyOnWriteChild(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelProcess* parentProcess,
    const char* name);
OrynKernelUserProcess OrynKernelUserProcessFromProcess(OrynKernelProcess* process);
OrynKernelThread* OrynKernelThreadCreateKernel(
    OrynKernelProcess* process,
    const char* name,
    void (*entryPoint)(void* context),
    void* context,
    unsigned long long stackBytes);
OrynKernelThread* OrynKernelThreadCreateUser(
    OrynKernelUserProcess* userProcess,
    const char* name,
    unsigned long long userEntry,
    unsigned long long userStackTop);
void OrynKernelThreadDestroy(OrynKernelThread* thread);
int OrynKernelProcessExit(OrynKernelProcess* process, int exitStatus);
int OrynKernelProcessWait(OrynKernelProcess* parent, unsigned int childProcessId, int* exitStatus);
int OrynKernelThreadExit(OrynKernelThread* thread, int exitStatus);
int OrynKernelThreadStop(OrynKernelThread* thread);
int OrynKernelThreadSetPriority(OrynKernelThread* thread, unsigned int priority);
int OrynKernelThreadSetCpuAffinity(OrynKernelThread* thread, unsigned int affinityMask);
int OrynKernelProcessSendEvent(OrynKernelProcess* target, unsigned int eventCode, unsigned long long eventValue);
int OrynKernelThreadSendEvent(OrynKernelThread* target, unsigned int eventCode, unsigned long long eventValue);
int OrynKernelThreadReceiveEvent(OrynKernelThread* thread, OrynKernelProcessEvent* eventOut);
int OrynKernelThreadIsSchedulerReady(const OrynKernelThread* thread);
const OrynKernelProcessStats* OrynKernelProcessGetStats(void);
int OrynKernelProcessRunSelfTest(OrynKernelPhysicalMemory* physicalMemory);
void OrynKernelProcessPrintProof(void);

#endif
