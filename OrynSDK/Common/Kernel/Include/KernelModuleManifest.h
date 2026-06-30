#ifndef ORYN_KERNEL_MODULE_MANIFEST_H
#define ORYN_KERNEL_MODULE_MANIFEST_H

typedef enum OrynKernelModuleId
{
    OrynKernelModuleBootInfo = 0,
    OrynKernelModuleScreenReport,
    OrynKernelModuleLifecycle,
    OrynKernelModulePanic,
    OrynKernelModuleGdt,
    OrynKernelModuleIdt,
    OrynKernelModuleInterrupts,
    OrynKernelModuleSysCalls,
    OrynKernelModuleCpu,
    OrynKernelModulePic,
    OrynKernelModuleApic,
    OrynKernelModuleSmp,
    OrynKernelModuleHpet,
    OrynKernelModulePci,
    OrynKernelModuleConsole,
    OrynKernelModuleKeyboard,
    OrynKernelModuleFat32,
    OrynKernelModuleVfs,
    OrynKernelModulePhysicalMemory,
    OrynKernelModuleVirtualMemory,
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
