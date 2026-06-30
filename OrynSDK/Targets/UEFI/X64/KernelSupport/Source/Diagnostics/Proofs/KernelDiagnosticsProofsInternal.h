#ifndef ORYN_KERNEL_DIAGNOSTICS_PROOFS_INTERNAL_H
#define ORYN_KERNEL_DIAGNOSTICS_PROOFS_INTERNAL_H

#include "KernelApic.h"
#include "KernelBootInfo.h"
#include "KernelConsole.h"
#include "KernelCpu.h"
#include "KernelGdt.h"
#include "KernelHpet.h"
#include "KernelFat32.h"
#include "KernelVfs.h"
#include "KernelIdt.h"
#include "KernelInterrupts.h"
#include "KernelIo.h"
#include "KernelKeyboard.h"
#include "KernelLifecycle.h"
#include "KernelMemoryMap.h"
#include "KernelPanic.h"
#include "KernelPci.h"
#include "KernelPhysicalMemory.h"
#include "KernelPic.h"
#include "KernelScreenReport.h"
#include "KernelSmp.h"
#include "KernelSysCallInterrupts.h"
#include "KernelVirtualMemory.h"
#include "SysCall.h"

#ifndef ORYN_VM_PIC
#define ORYN_VM_PIC 1
#endif

#ifndef ORYN_VM_APIC
#define ORYN_VM_APIC 1
#endif

#ifndef ORYN_VM_APIC2
#define ORYN_VM_APIC2 1
#endif

#ifndef ORYN_VM_HPET
#define ORYN_VM_HPET 1
#endif

#ifndef ORYN_VM_SMP_CPUS
#define ORYN_VM_SMP_CPUS 1
#endif

#ifndef ORYN_VM_INTERACTIVE_DISPLAY
#define ORYN_VM_INTERACTIVE_DISPLAY 0
#endif

void OrynKernelDiagnosticsPrintEntryProofs(const OrynBootInfo* kernelBootInfo);
void OrynKernelDiagnosticsRunDescriptorProofs(void);
void OrynKernelDiagnosticsRunInterruptTimerProofs(const OrynBootInfo* kernelBootInfo);
void OrynKernelDiagnosticsRunPciProof(const OrynBootInfo* kernelBootInfo);
void OrynKernelDiagnosticsRunConsoleProofs(const OrynBootInfo* kernelBootInfo);
int OrynKernelDiagnosticsConsoleRunScrollProbe(void);
int OrynKernelDiagnosticsConsoleRunDoubleBufferProbe(void);
int OrynKernelDiagnosticsConsoleRunLineBufferedProbe(void);
int OrynKernelDiagnosticsConsoleRunFastRefreshProbe(void);
int OrynKernelDiagnosticsRunKeyboardScrollProof(void);
void OrynKernelDiagnosticsRunMemoryProofs(const OrynBootInfo* kernelBootInfo);
void OrynKernelDiagnosticsRunFat32VfsProof(void);
void OrynKernelDiagnosticsRunBootInfoFailureProof(void);

#endif
