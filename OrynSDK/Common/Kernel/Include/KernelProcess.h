
#ifndef ORYN_KERNEL_PROCESS_H
#define ORYN_KERNEL_PROCESS_H

#include "KernelHeap.h"
#include "KernelVirtualMemory.h"
#include "KernelContextSwitch.h"

#define ORYN_KERNEL_PROCESS_NAME_LENGTH 32U
#define ORYN_KERNEL_THREAD_NAME_LENGTH 32U
#define ORYN_KERNEL_THREAD_DEFAULT_STACK_BYTES 16384ULL
#define ORYN_KERNEL_THREAD_STACK_GUARD_BYTES 4096ULL

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
    OrynKernelProcessStateTerminated = 3
} OrynKernelProcessState;

typedef enum OrynKernelThreadState
{
    OrynKernelThreadStateUnused = 0,
    OrynKernelThreadStateCreated = 1,
    OrynKernelThreadStateSchedulerReady = 2,
    OrynKernelThreadStateRunning = 3,
    OrynKernelThreadStateBlocked = 4,
    OrynKernelThreadStateSleeping = 5,
    OrynKernelThreadStateTerminated = 6
} OrynKernelThreadState;

typedef struct OrynKernelProcess
{
    unsigned int ProcessId;
    unsigned int ParentProcessId;
    unsigned int State;
    unsigned int Kind;
    unsigned int UserMode;
    unsigned int ThreadCount;
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
int OrynKernelThreadIsSchedulerReady(const OrynKernelThread* thread);
const OrynKernelProcessStats* OrynKernelProcessGetStats(void);
int OrynKernelProcessRunSelfTest(OrynKernelPhysicalMemory* physicalMemory);
void OrynKernelProcessPrintProof(void);

#endif
