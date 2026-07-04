#ifndef ORYN_KERNEL_MODULE_MANIFEST_H
#define ORYN_KERNEL_MODULE_MANIFEST_H

/* Generated from kernel module manifest files. Do not hand-edit module tables here. */

typedef enum OrynKernelModuleId
{
    OrynKernelModuleBootInfo = 0,
    OrynKernelModuleScreenReport = 1,
    OrynKernelModuleLibC = 2,
    OrynKernelModuleLifecycle = 3,
    OrynKernelModulePanic = 4,
    OrynKernelModuleGdt = 5,
    OrynKernelModuleIdt = 6,
    OrynKernelModuleInterrupts = 7,
    OrynKernelModuleSysCalls = 8,
    OrynKernelModuleCpu = 9,
    OrynKernelModulePic = 10,
    OrynKernelModuleApic = 11,
    OrynKernelModuleSmp = 12,
    OrynKernelModuleHpet = 13,
    OrynKernelModulePci = 14,
    OrynKernelModuleConsole = 15,
    OrynKernelModuleKeyboard = 16,
    OrynKernelModuleFat32 = 17,
    OrynKernelModuleVfs = 18,
    OrynKernelModulePhysicalMemory = 19,
    OrynKernelModuleHeap = 20,
    OrynKernelModuleVirtualMemory = 21,
    OrynKernelModuleProcess = 22,
    OrynKernelModuleScheduler = 23,
    OrynKernelModuleCount
} OrynKernelModuleId;

typedef enum OrynKernelModuleState
{
    OrynKernelModuleStateAbsent = 0,
    OrynKernelModuleStateRegistered,
    OrynKernelModuleStateSelected,
    OrynKernelModuleStateStarting,
    OrynKernelModuleStateReady,
    OrynKernelModuleStateStopping,
    OrynKernelModuleStateStopped,
    OrynKernelModuleStatePanic,
    OrynKernelModuleStateShuttingDown,
    OrynKernelModuleStateShutdown,
    OrynKernelModuleStateSkipped,
    OrynKernelModuleStateFailed
} OrynKernelModuleState;

typedef int (*OrynKernelModuleLifecycleCallback)(OrynKernelModuleId id);
typedef void (*OrynKernelModuleLinkRoot)(void);

typedef struct OrynKernelModuleManifestItem
{
    OrynKernelModuleId Id;
    const char* Name;
    const char* Items;
    const char* Selects;
    OrynKernelModuleId Requires[6];
    unsigned int RequireCount;
    int CompiledIn;
    int Required;
    int FatalOnMissingPrerequisite;
    const char* StopCallbackName;
    const char* PanicCallbackName;
    const char* ShutdownCallbackName;
    OrynKernelModuleLifecycleCallback StopCallback;
    OrynKernelModuleLifecycleCallback PanicCallback;
    OrynKernelModuleLifecycleCallback ShutdownCallback;
    OrynKernelModuleState State;
} OrynKernelModuleManifestItem;

typedef struct OrynKernelCompiledModuleRecord
{
    OrynKernelModuleId Id;
    const char* Name;
    int CompiledIn;
    int SelectedInBuild;
    const char* LinkRootName;
    OrynKernelModuleLinkRoot LinkRoot;
} OrynKernelCompiledModuleRecord;

void OrynKernelModuleLinkRoot_OrynKernelModuleBootInfo(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleScreenReport(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleLibC(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleLifecycle(void);
void OrynKernelModuleLinkRoot_OrynKernelModulePanic(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleGdt(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleIdt(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleInterrupts(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleSysCalls(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleCpu(void);
void OrynKernelModuleLinkRoot_OrynKernelModulePic(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleApic(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleSmp(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleHpet(void);
void OrynKernelModuleLinkRoot_OrynKernelModulePci(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleConsole(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleKeyboard(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleFat32(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleVfs(void);
void OrynKernelModuleLinkRoot_OrynKernelModulePhysicalMemory(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleHeap(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleVirtualMemory(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleProcess(void);
void OrynKernelModuleLinkRoot_OrynKernelModuleScheduler(void);

void OrynKernelModuleManifestInit(void);
const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id);
int OrynKernelModuleManifestCanStart(OrynKernelModuleId id);
unsigned int OrynKernelModuleManifestRequireCount(OrynKernelModuleId id);
OrynKernelModuleId OrynKernelModuleManifestRequireAt(OrynKernelModuleId id, unsigned int index);
int OrynKernelModuleManifestIsReady(OrynKernelModuleId id);
int OrynKernelModuleManifestIsCompiledIn(OrynKernelModuleId id);
int OrynKernelModuleManifestIsRequired(OrynKernelModuleId id);
int OrynKernelModuleManifestFatalOnMissingPrerequisite(OrynKernelModuleId id);
const char* OrynKernelModuleManifestSelects(OrynKernelModuleId id);
void OrynKernelModuleManifestSelected(OrynKernelModuleId id);
int OrynKernelModuleManifestBegin(OrynKernelModuleId id);
int OrynKernelModuleManifestBeginProof(OrynKernelModuleId id);
void OrynKernelModuleManifestReady(OrynKernelModuleId id);
void OrynKernelModuleManifestSkipped(OrynKernelModuleId id);
void OrynKernelModuleManifestFailed(OrynKernelModuleId id);
int OrynKernelModuleDefaultStop(OrynKernelModuleId id);
int OrynKernelModuleDefaultPanic(OrynKernelModuleId id);
int OrynKernelModuleDefaultShutdown(OrynKernelModuleId id);
int OrynKernelModuleManifestHasLifecycleCallbacks(OrynKernelModuleId id);
int OrynKernelModuleManifestStop(OrynKernelModuleId id);
int OrynKernelModuleManifestPanic(OrynKernelModuleId id);
int OrynKernelModuleManifestShutdown(OrynKernelModuleId id);
unsigned int OrynKernelModuleManifestInvokeStopCallbacks(void);
unsigned int OrynKernelModuleManifestInvokePanicCallbacks(void);
unsigned int OrynKernelModuleManifestInvokeShutdownCallbacks(void);
void OrynKernelModuleManifestCallbackProof(void);
void OrynKernelModuleManifestTransitionProof(void);
const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state);
unsigned int OrynKernelCompiledModuleCount(void);
const OrynKernelCompiledModuleRecord* OrynKernelCompiledModuleGet(unsigned int index);
unsigned int OrynKernelSelectedModuleLinkRootCount(void);
unsigned int OrynKernelSelectedModuleMissingLinkRootCount(void);
void OrynKernelCompiledModuleRegistryPrintProof(void);
void OrynKernelModuleManifestPrintProof(void);

#endif
