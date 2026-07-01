#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

static int AddProjectModule(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    return AddBuildModule(plan, project, "ProjectKernel", project->source_dir, 1, "TargetRuntime,SDKRuntime");
}

static int AddCommonKernelModules(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    char root[ORYN_MAX_PATH];
    snprintf(root, sizeof(root), "%s", project->sdk_kernel_common_source_dir);
    if (!AddBuildModule(plan, project, "SDKKernelCore", root, 0, "OrynLibC")) return 0;
    char path[ORYN_MAX_PATH];
#define ADD(name, dir, req) do { OrynJoinPath(path, sizeof(path), root, dir); if (!AddBuildModule(plan, project, name, path, 1, req)) return 0; } while (0)
    ADD("SDKBootInfo", "BootInfo", "OrynLibC");
    ADD("SDKScreenReport", "Screen", "OrynLibC");
    ADD("SDKLifecycle", "Lifecycle", "OrynLibC");
    ADD("SDKPanic", "Panic", "SDKBootInfo,SDKScreenReport");
    ADD("SDKManifest", "Manifest", "OrynLibC");
    ADD("SDKMemory", "Memory", "SDKBootInfo");
    ADD("SDKConsole", "Console", "SDKBootInfo,SDKKernelCore");
    ADD("SDKFonts", "Fonts", "SDKConsole");
    ADD("SDKSysCalls", "SysCalls", "OrynLibC");
    ADD("SDKRuntime", "Runtime", "SDKBootInfo,SDKLifecycle,SDKPanic,SDKManifest");
#undef ADD
    return 1;
}

static int AddTargetKernelModules(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    char root[ORYN_MAX_PATH];
    snprintf(root, sizeof(root), "%s", project->sdk_kernel_target_source_dir);
    if (!AddBuildModule(plan, project, "TargetKernelCore", root, 0, "SDKKernelCore")) return 0;
    char path[ORYN_MAX_PATH];
#define ADD(name, dir, req) do { OrynJoinPath(path, sizeof(path), root, dir); if (!AddBuildModule(plan, project, name, path, 1, req)) return 0; } while (0)
    ADD("TargetCpu", "CPU", "TargetKernelCore");
    ADD("TargetIo", "IO", "TargetCpu");
    ADD("TargetInterrupts", "Interrupts", "TargetCpu,TargetIo");
    ADD("TargetInput", "Input", "SDKConsole,TargetInterrupts");
    ADD("TargetMemory", "Memory", "SDKMemory,TargetCpu");
    ADD("TargetPci", "PCI", "SDKBootInfo,TargetMemory");
    ADD("TargetSmp", "SMP", "TargetInterrupts,TargetMemory");
    ADD("TargetTimers", "Timers", "SDKBootInfo,TargetInterrupts");
    ADD("TargetDiagnostics", "Diagnostics", "SDKScreenReport,TargetTimers,TargetPci");
    ADD("TargetFat32", "FileSystems/FAT32", "SDKConsole");
    ADD("TargetVfs", "FileSystems/VFS", "TargetFat32");
    ADD("TargetRuntime", "Runtime", "SDKRuntime,TargetInterrupts,TargetMemory,TargetTimers,TargetPci,TargetInput,TargetVfs,TargetDiagnostics");
#undef ADD
    return 1;
}

int BuildKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan)
{
    memset(plan, 0, sizeof(*plan));
    char libc[ORYN_MAX_PATH];
    OrynJoinPath(libc, sizeof(libc), project->sdk_root, "Common/OrynLibC/Source");
    if (!AddBuildModule(plan, project, "OrynLibC", libc, 1, "")) return 0;
    if (!AddCommonKernelModules(project, plan)) return 0;
    if (!AddTargetKernelModules(project, plan)) return 0;
    return AddProjectModule(project, plan) && ResolveKernelArchivePlan(project, plan);
}
