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
int OrynKernelModuleManifestBegin(OrynKernelModuleId id);
void OrynKernelModuleManifestReady(OrynKernelModuleId id);
void OrynKernelModuleManifestFailed(OrynKernelModuleId id);
void OrynKernelModuleManifestPrintProof(void);

#endif
