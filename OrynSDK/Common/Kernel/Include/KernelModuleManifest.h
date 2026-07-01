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
    OrynKernelModuleVirtualMemory = 20,
    OrynKernelModuleCount
} OrynKernelModuleId;

typedef enum OrynKernelModuleState
{
    OrynKernelModuleStateAbsent = 0,
    OrynKernelModuleStateRegistered,
    OrynKernelModuleStateSelected,
    OrynKernelModuleStateStarting,
    OrynKernelModuleStateReady,
    OrynKernelModuleStateSkipped,
    OrynKernelModuleStateFailed
} OrynKernelModuleState;

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
    OrynKernelModuleState State;
} OrynKernelModuleManifestItem;

typedef struct OrynKernelCompiledModuleRecord
{
    OrynKernelModuleId Id;
    const char* Name;
    int CompiledIn;
} OrynKernelCompiledModuleRecord;

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
void OrynKernelModuleManifestReady(OrynKernelModuleId id);
void OrynKernelModuleManifestSkipped(OrynKernelModuleId id);
void OrynKernelModuleManifestFailed(OrynKernelModuleId id);
const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state);
unsigned int OrynKernelCompiledModuleCount(void);
const OrynKernelCompiledModuleRecord* OrynKernelCompiledModuleGet(unsigned int index);
void OrynKernelCompiledModuleRegistryPrintProof(void);
void OrynKernelModuleManifestPrintProof(void);

#endif
