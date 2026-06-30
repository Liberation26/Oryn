#ifndef ORYN_KERNEL_HPET_H
#define ORYN_KERNEL_HPET_H

#include "OrynBootInfo.h"

typedef struct OrynKernelHpetState
{
    unsigned int Initialized;
    unsigned int RsdpPresent;
    unsigned int AcpiChecksumOk;
    unsigned int HpetTableFound;
    unsigned int Enabled;
    unsigned int Counter64Bit;
    unsigned int LegacyReplacementRoute;
    unsigned int TimerCount;
    unsigned int HpetNumber;
    unsigned int MinimumTick;
    unsigned int CounterAdvanced;
    unsigned long long HpetTable;
    unsigned long long BaseAddress;
    unsigned long long Capabilities;
    unsigned long long Configuration;
    unsigned long long CounterBefore;
    unsigned long long CounterAfter;
    unsigned long long CounterPeriodFemtoSeconds;
} OrynKernelHpetState;

int OrynKernelHpetInit(const OrynBootInfo* bootInfo);
const OrynKernelHpetState* OrynKernelHpetGetState(void);
void OrynKernelHpetPrintProof(void);

#endif
