#ifndef ORYN_KERNEL_INIT_GRAPH_H
#define ORYN_KERNEL_INIT_GRAPH_H

typedef enum OrynKernelInitStageId
{
    OrynKernelInitStageEntry = 0,
    OrynKernelInitStageBootInfo = 1,
    OrynKernelInitStageModuleManifest = 2,
    OrynKernelInitStageLibC = 3,
    OrynKernelInitStageDescriptors = 4,
    OrynKernelInitStageInterrupts = 5,
    OrynKernelInitStageTimers = 6,
    OrynKernelInitStagePci = 7,
    OrynKernelInitStageConsole = 8,
    OrynKernelInitStageFat32Vfs = 9,
    OrynKernelInitStageMemory = 10,
    OrynKernelInitStageHeap = 11,
    OrynKernelInitStageVirtualMemory = 12,
    OrynKernelInitStageRunning = 13,
    OrynKernelInitStageCount = 14
} OrynKernelInitStageId;

typedef struct OrynKernelInitStage
{
    OrynKernelInitStageId Id;
    const char* Name;
    const char* Description;
    unsigned int DependencyCount;
    OrynKernelInitStageId Dependencies[4];
} OrynKernelInitStage;

unsigned int OrynKernelInitGraphCount(void);
const OrynKernelInitStage* OrynKernelInitGraphGet(unsigned int index);
const char* OrynKernelInitStageName(OrynKernelInitStageId id);
int OrynKernelInitGraphValidate(void);

#endif
