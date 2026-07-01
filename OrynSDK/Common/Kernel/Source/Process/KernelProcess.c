#include "KernelProcess.h"
#include "KernelScheduler.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelScreenReport.h"
#include "string.h"

static unsigned int gNextProcessId = 1U;
static unsigned int gNextThreadId = 1U;
static OrynKernelProcessStats gProcessStats;
static const char* gProcessProofFailure;

static void ProcessClear(void* pointer, unsigned long long bytes)
{
    if (pointer != 0 && bytes != 0ULL)
    {
        (void)memset(pointer, 0, (size_t)bytes);
    }
}

static void ProcessCopyName(char* target, unsigned int capacity, const char* source)
{
    unsigned int index = 0U;
    if (target == 0 || capacity == 0U)
    {
        return;
    }
    if (source == 0)
    {
        source = "unnamed";
    }
    while (source[index] != 0 && index + 1U < capacity)
    {
        target[index] = source[index];
        index += 1U;
    }
    target[index] = 0;
}

static unsigned long long AlignStackBytes(unsigned long long bytes)
{
    if (bytes < ORYN_KERNEL_THREAD_DEFAULT_STACK_BYTES)
    {
        bytes = ORYN_KERNEL_THREAD_DEFAULT_STACK_BYTES;
    }
    return (bytes + ORYN_KERNEL_THREAD_STACK_GUARD_BYTES - 1ULL) &
        ~(ORYN_KERNEL_THREAD_STACK_GUARD_BYTES - 1ULL);
}

void OrynKernelProcessSystemInit(void)
{
    ProcessClear(&gProcessStats, sizeof(gProcessStats));
    gProcessStats.Initialized = 1U;
    gProcessStats.KernelThreadStructureReady = 1U;
    gProcessStats.UserProcessStructureReady = 1U;
    gProcessStats.UserThreadStructureReady = 1U;
    gNextProcessId = 1U;
    gNextThreadId = 1U;
    gProcessProofFailure = 0;
    OrynKernelContextSwitchInit();
}

OrynKernelProcess* OrynKernelProcessCreate(
    OrynKernelPhysicalMemory* physicalMemory,
    const char* name,
    unsigned int parentProcessId)
{
    OrynKernelProcess* process;
    OrynKernelAddressSpace* addressSpace;
    if (gProcessStats.Initialized == 0U)
    {
        OrynKernelProcessSystemInit();
    }
    process = (OrynKernelProcess*)kmalloc(sizeof(OrynKernelProcess));
    addressSpace = (OrynKernelAddressSpace*)kmalloc(sizeof(OrynKernelAddressSpace));
    if (process == 0 || addressSpace == 0)
    {
        if (process != 0) kfree(process);
        if (addressSpace != 0) kfree(addressSpace);
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    ProcessClear(process, sizeof(*process));
    ProcessClear(addressSpace, sizeof(*addressSpace));
    if (!OrynVirtualMemoryCreateProcessAddressSpace(physicalMemory, addressSpace))
    {
        kfree(addressSpace);
        kfree(process);
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    process->ProcessId = gNextProcessId++;
    process->ParentProcessId = parentProcessId;
    process->State = OrynKernelProcessStateReady;
    process->Kind = OrynKernelProcessKindKernel;
    process->AddressSpace = addressSpace;
    ProcessCopyName(process->Name, ORYN_KERNEL_PROCESS_NAME_LENGTH, name);
    gProcessStats.ProcessCreatedCount += 1U;
    gProcessStats.AddressSpaceBoundProcessCount += 1U;
    return process;
}

OrynKernelProcess* OrynKernelProcessCreateCopyOnWriteChild(
    OrynKernelPhysicalMemory* physicalMemory,
    OrynKernelProcess* parentProcess,
    const char* name)
{
    OrynKernelProcess* child;
    OrynKernelAddressSpace* childAddressSpace;
    if (parentProcess == 0 || parentProcess->AddressSpace == 0 ||
        parentProcess->AddressSpace->ProcessOwned == 0U)
    {
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    child = (OrynKernelProcess*)kmalloc(sizeof(OrynKernelProcess));
    childAddressSpace = (OrynKernelAddressSpace*)kmalloc(sizeof(OrynKernelAddressSpace));
    if (child == 0 || childAddressSpace == 0)
    {
        if (child != 0) kfree(child);
        if (childAddressSpace != 0) kfree(childAddressSpace);
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    ProcessClear(child, sizeof(*child));
    ProcessClear(childAddressSpace, sizeof(*childAddressSpace));
    if (!OrynVirtualMemoryCreateCopyOnWriteClone(
        physicalMemory,
        parentProcess->AddressSpace,
        childAddressSpace))
    {
        kfree(childAddressSpace);
        kfree(child);
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    child->ProcessId = gNextProcessId++;
    child->ParentProcessId = parentProcess->ProcessId;
    child->State = OrynKernelProcessStateCreated;
    child->Kind = parentProcess->Kind;
    child->UserMode = parentProcess->UserMode;
    child->AddressSpace = childAddressSpace;
    ProcessCopyName(child->Name, ORYN_KERNEL_PROCESS_NAME_LENGTH, name);
    gProcessStats.ProcessCreatedCount += 1U;
    gProcessStats.AddressSpaceBoundProcessCount += 1U;
    gProcessStats.CopyOnWriteChildProcessCount += 1U;
    gProcessStats.CopyOnWriteSharedPages += childAddressSpace->CopyOnWriteSharedPages;
    return child;
}

OrynKernelUserProcess OrynKernelUserProcessFromProcess(OrynKernelProcess* process)
{
    OrynKernelUserProcess userProcess;
    ProcessClear(&userProcess, sizeof(userProcess));
    if (process != 0)
    {
        process->Kind = OrynKernelProcessKindUser;
        process->UserMode = 1U;
        userProcess.Process = process;
        userProcess.UserProcessId = process->ProcessId;
        userProcess.AddressSpace = process->AddressSpace;
    }
    return userProcess;
}

void OrynKernelProcessDestroy(OrynKernelProcess* process)
{
    if (process == 0)
    {
        return;
    }
    if (process->AddressSpace != 0)
    {
        kfree(process->AddressSpace);
        process->AddressSpace = 0;
    }
    process->State = OrynKernelProcessStateTerminated;
    kfree(process);
}

static OrynKernelThread* AllocateThread(OrynKernelProcess* process,
    const char* name, unsigned long long stackBytes)
{
    OrynKernelThread* thread;
    unsigned long long alignedStackBytes;
    void* stack;
    alignedStackBytes = AlignStackBytes(stackBytes);
    stack = kcalloc(1ULL, alignedStackBytes);
    thread = (OrynKernelThread*)kmalloc(sizeof(OrynKernelThread));
    if (stack == 0 || thread == 0)
    {
        if (stack != 0) kfree(stack);
        if (thread != 0) kfree(thread);
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    ProcessClear(thread, sizeof(*thread));
    thread->ThreadId = gNextThreadId++;
    thread->State = OrynKernelThreadStateSchedulerReady;
    thread->OwnerProcess = process;
    thread->StackBase = stack;
    thread->StackBytes = alignedStackBytes;
    thread->GuardBytes = ORYN_KERNEL_THREAD_STACK_GUARD_BYTES;
    thread->StackTop = (void*)((unsigned long long)stack + alignedStackBytes);
    thread->SchedulerReady = 1U;
    thread->QuantumTicks = 4U;
    thread->RemainingQuantumTicks = thread->QuantumTicks;
    ProcessCopyName(thread->Name, ORYN_KERNEL_THREAD_NAME_LENGTH, name);
    OrynKernelHeapInstallStackGuard((unsigned long long)thread->StackBase, thread->StackBytes);
    process->ThreadCount += 1U;
    gProcessStats.ThreadCreatedCount += 1U;
    gProcessStats.SchedulerReadyThreadCount += 1U;
    gProcessStats.KernelThreadStackCount += 1U;
    gProcessStats.KernelThreadStackBytes += alignedStackBytes;
    gProcessStats.KernelThreadGuardBytes += ORYN_KERNEL_THREAD_STACK_GUARD_BYTES;
    return thread;
}

OrynKernelThread* OrynKernelThreadCreateKernel(
    OrynKernelProcess* process,
    const char* name,
    void (*entryPoint)(void* context),
    void* context,
    unsigned long long stackBytes)
{
    OrynKernelThread* thread;
    if (process == 0 || process->AddressSpace == 0)
    {
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    thread = AllocateThread(process, name, stackBytes);
    if (thread == 0)
    {
        return 0;
    }
    thread->EntryPoint = entryPoint;
    thread->Context = context;
    OrynKernelContextSwitchPrepareInitial(&thread->CpuContext,
        thread->StackTop, entryPoint, context, process->AddressSpace->Pml4Physical);
    return thread;
}

OrynKernelThread* OrynKernelThreadCreateUser(
    OrynKernelUserProcess* userProcess,
    const char* name,
    unsigned long long userEntry,
    unsigned long long userStackTop)
{
    OrynKernelThread* thread;
    if (userProcess == 0 || userProcess->Process == 0 || userProcess->AddressSpace == 0)
    {
        gProcessStats.FailedAllocations += 1U;
        return 0;
    }
    thread = AllocateThread(userProcess->Process, name, ORYN_KERNEL_THREAD_DEFAULT_STACK_BYTES);
    if (thread == 0)
    {
        return 0;
    }
    thread->IsUserThread = 1U;
    thread->CpuContext.Rip = userEntry;
    thread->CpuContext.Rsp = userStackTop;
    thread->CpuContext.Rflags = 0x202ULL;
    thread->CpuContext.Cr3 = userProcess->AddressSpace->Pml4Physical;
    return thread;
}

void OrynKernelThreadDestroy(OrynKernelThread* thread)
{
    if (thread == 0)
    {
        return;
    }
    if (thread->OwnerProcess != 0 && thread->OwnerProcess->ThreadCount > 0U)
    {
        thread->OwnerProcess->ThreadCount -= 1U;
    }
    if (thread->StackBase != 0)
    {
        kfree(thread->StackBase);
        thread->StackBase = 0;
    }
    thread->State = OrynKernelThreadStateTerminated;
    kfree(thread);
}

int OrynKernelThreadIsSchedulerReady(const OrynKernelThread* thread)
{
    return thread != 0 && thread->SchedulerReady != 0U &&
        thread->State == OrynKernelThreadStateSchedulerReady &&
        thread->OwnerProcess != 0 && thread->OwnerProcess->AddressSpace != 0 &&
        thread->StackBase != 0 && thread->StackTop != 0;
}

const OrynKernelProcessStats* OrynKernelProcessGetStats(void)
{
    return &gProcessStats;
}

static void DummyKernelThreadEntry(void* context)
{
    (void)context;
}

int OrynKernelProcessRunSelfTest(OrynKernelPhysicalMemory* physicalMemory)
{
    OrynKernelProcess* process;
    OrynKernelThread* thread;
    OrynKernelThread* userThread;
    OrynKernelProcess* child;
    OrynKernelUserProcess userProcess;
    const OrynKernelHeapStats* heapBefore;
    const OrynKernelHeapStats* heapAfter;
    unsigned long long stackGuardPagesBefore;
    gProcessProofFailure = 0;
    if (physicalMemory == 0 || physicalMemory->Initialized == 0U)
    {
        gProcessProofFailure = "physical memory allocator is not initialized";
        return 0;
    }
    OrynKernelProcessSystemInit();
    heapBefore = OrynKernelHeapGetStats();
    if (heapBefore == 0 || heapBefore->Initialized == 0U)
    {
        gProcessProofFailure = "kernel heap is not initialized";
        return 0;
    }
    stackGuardPagesBefore = heapBefore->StackGuardPages;
    process = OrynKernelProcessCreate(physicalMemory, "init", 0U);
    if (process == 0)
    {
        gProcessProofFailure = "process allocation failed";
        return 0;
    }
    child = OrynKernelProcessCreateCopyOnWriteChild(physicalMemory, process, "init-child");
    userProcess = OrynKernelUserProcessFromProcess(process);
    userThread = OrynKernelThreadCreateUser(&userProcess, "user-main", 0x400000ULL, 0x700000ULL);
    thread = OrynKernelThreadCreateKernel(
        process, "init-main", DummyKernelThreadEntry, process,
        ORYN_KERNEL_THREAD_DEFAULT_STACK_BYTES);
    if (child == 0 || !OrynKernelThreadIsSchedulerReady(thread) ||
        !OrynKernelThreadIsSchedulerReady(userThread) || userThread->IsUserThread == 0U)
    {
        gProcessProofFailure = "process/thread structure proof failed";
        OrynKernelThreadDestroy(userThread);
        OrynKernelThreadDestroy(thread);
        OrynKernelProcessDestroy(child);
        OrynKernelProcessDestroy(process);
        return 0;
    }
    heapAfter = OrynKernelHeapGetStats();
    if (heapAfter == 0 || heapAfter->StackGuardPages <= stackGuardPagesBefore)
    {
        gProcessProofFailure = "stack guard page accounting did not advance";
        OrynKernelThreadDestroy(userThread);
        OrynKernelThreadDestroy(thread);
        OrynKernelProcessDestroy(child);
        OrynKernelProcessDestroy(process);
        return 0;
    }
    if (!OrynKernelContextSwitchRunSelfTest() || !OrynKernelSchedulerRunSelfTest(thread))
    {
        gProcessProofFailure = "context switch or scheduler proof failed";
        OrynKernelThreadDestroy(userThread);
        OrynKernelThreadDestroy(thread);
        OrynKernelProcessDestroy(child);
        OrynKernelProcessDestroy(process);
        return 0;
    }
    OrynKernelThreadDestroy(userThread);
    OrynKernelThreadDestroy(thread);
    OrynKernelProcessDestroy(child);
    OrynKernelProcessDestroy(process);
    return 1;
}

void OrynKernelProcessPrintProof(void)
{
    OrynKernelDiagnosticsLogText("[KERNEL] Process/thread structures: active\n");
    if (gProcessProofFailure != 0)
    {
        OrynKernelDiagnosticsLogText("[KERNEL] Process/thread proof detail: ");
        OrynKernelDiagnosticsLogText(gProcessProofFailure);
        OrynKernelDiagnosticsLogText("\n");
    }
    OrynKernelDiagnosticsLogText("[KERNEL] Processes created: ");
    OrynKernelDiagnosticsLogDec64(gProcessStats.ProcessCreatedCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] Threads created: ");
    OrynKernelDiagnosticsLogDec64(gProcessStats.ThreadCreatedCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(gProcessStats.KernelThreadStructureReady,
        "Kernel thread structure is defined.",
        "Kernel thread structure proof failed.");
    OrynKernelScreenReportOkOrFail(gProcessStats.UserProcessStructureReady,
        "User process structure is defined.",
        "User process structure proof failed.");
    OrynKernelScreenReportOkOrFail(gProcessStats.UserThreadStructureReady,
        "User thread structure is defined.",
        "User thread structure proof failed.");
    OrynKernelScreenReportOkOrFail(gProcessStats.AddressSpaceBoundProcessCount > 0U,
        "Process structures bind to per-process address spaces.",
        "Process structures did not bind to per-process address spaces.");
    OrynKernelScreenReportOkOrFail(gProcessStats.SchedulerReadyThreadCount > 0U,
        "Scheduler-ready kernel thread stacks use guarded heap/VM helpers.",
        "Scheduler-ready kernel thread stack allocation proof failed.");
    OrynKernelScreenReportOkOrFail(gProcessStats.CopyOnWriteChildProcessCount > 0U,
        "Process creation has a copy-on-write child foundation for fork-like creation.",
        "Process copy-on-write child foundation proof failed.");
    OrynKernelContextSwitchPrintProof();
    OrynKernelSchedulerPrintProof();
}
