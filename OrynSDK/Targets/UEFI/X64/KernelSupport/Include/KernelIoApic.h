#ifndef ORYN_KERNEL_IOAPIC_H
#define ORYN_KERNEL_IOAPIC_H

#include "OrynBootInfo.h"

#define ORYN_KERNEL_IOAPIC_MAX_CONTROLLERS 8U
#define ORYN_KERNEL_IOAPIC_MAX_OVERRIDES 32U
#define ORYN_KERNEL_IOAPIC_MAX_ROUTES 64U

typedef struct OrynKernelIoApicController
{
    unsigned int Id;
    unsigned int Address;
    unsigned int GlobalSystemInterruptBase;
    unsigned int RedirectionEntries;
} OrynKernelIoApicController;

typedef struct OrynKernelIoApicOverride
{
    unsigned int Bus;
    unsigned int SourceIrq;
    unsigned int GlobalSystemInterrupt;
    unsigned int Flags;
} OrynKernelIoApicOverride;

typedef struct OrynKernelIoApicRoute
{
    unsigned int LegacyIrq;
    unsigned int GlobalSystemInterrupt;
    unsigned int Vector;
    unsigned int Flags;
    unsigned int ControllerIndex;
    unsigned int RedirectionIndex;
    unsigned int Programmed;
} OrynKernelIoApicRoute;

typedef struct OrynKernelIoApicState
{
    unsigned int Initialized;
    unsigned int AcpiRsdpPresent;
    unsigned int AcpiChecksumOk;
    unsigned int MadtFound;
    unsigned int LocalApicAddress;
    unsigned int ControllerCount;
    unsigned int ControllerListTruncated;
    unsigned int OverrideCount;
    unsigned int OverrideListTruncated;
    unsigned int RoutesRecorded;
    unsigned int RoutesProgrammed;
    OrynKernelIoApicController Controllers[ORYN_KERNEL_IOAPIC_MAX_CONTROLLERS];
    OrynKernelIoApicOverride Overrides[ORYN_KERNEL_IOAPIC_MAX_OVERRIDES];
    OrynKernelIoApicRoute Routes[ORYN_KERNEL_IOAPIC_MAX_ROUTES];
} OrynKernelIoApicState;

int OrynKernelIoApicInit(const OrynBootInfo* bootInfo);
const OrynKernelIoApicState* OrynKernelIoApicGetState(void);
int OrynKernelIoApicRouteLegacyIrq(unsigned int legacyIrq, unsigned int vector);
void OrynKernelIoApicRouteLegacySet(unsigned int firstVector);
void OrynKernelIoApicPrintProof(void);

#endif
