#ifndef ORYN_KERNEL_MODULE_MANIFEST_H
#define ORYN_KERNEL_MODULE_MANIFEST_H

/* Generated from Common/Kernel/ModuleManifests/*.module. Do not hand-edit module tables here. */

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
    OrynKernelModuleId Requires[6];
    unsigned int RequireCount;
    OrynKernelModuleState State;
} OrynKernelModuleManifestItem;

void OrynKernelModuleManifestInit(void);
const OrynKernelModuleManifestItem* OrynKernelModuleManifestGet(OrynKernelModuleId id);
int OrynKernelModuleManifestCanStart(OrynKernelModuleId id);
unsigned int OrynKernelModuleManifestRequireCount(OrynKernelModuleId id);
OrynKernelModuleId OrynKernelModuleManifestRequireAt(OrynKernelModuleId id, unsigned int index);
int OrynKernelModuleManifestIsReady(OrynKernelModuleId id);
int OrynKernelModuleManifestBegin(OrynKernelModuleId id);
void OrynKernelModuleManifestReady(OrynKernelModuleId id);
void OrynKernelModuleManifestSkipped(OrynKernelModuleId id);
void OrynKernelModuleManifestFailed(OrynKernelModuleId id);
const char* OrynKernelModuleManifestStateName(OrynKernelModuleState state);
void OrynKernelModuleManifestPrintProof(void);

#endif
