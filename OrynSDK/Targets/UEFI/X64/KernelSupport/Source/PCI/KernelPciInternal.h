#ifndef ORYN_KERNEL_PCI_INTERNAL_H
#define ORYN_KERNEL_PCI_INTERNAL_H

#include "KernelPci.h"
#include "KernelBootInfo.h"
#include "KernelIo.h"
#include "KernelPortIo.h"

#define ORYN_PCI_CONFIG_ADDRESS 0xCF8U
#define ORYN_PCI_CONFIG_DATA 0xCFCU
#define ORYN_PCI_ENABLE 0x80000000U
#define ORYN_PCI_INVALID_VENDOR 0xFFFFU

extern OrynKernelPciState gPciState;

void ClearState(void);
unsigned int MakePciAddress(unsigned int bus, unsigned int device, unsigned int function, unsigned int offset);
unsigned int PciRead16(unsigned int bus, unsigned int device, unsigned int function, unsigned int offset);
unsigned int PciRead8(unsigned int bus, unsigned int device, unsigned int function, unsigned int offset);
void DiscoverMcfg(const OrynBootInfo* bootInfo);
void CountClass(const OrynKernelPciDevice* pciDevice);
void RecordDevice(const OrynKernelPciDevice* pciDevice);
void ReadDevice(unsigned int bus, unsigned int device, unsigned int function, OrynKernelPciDevice* outDevice);
void ScanConfigSpace(void);
void PrintDeviceLine(const OrynKernelPciDevice* pciDevice);

#endif
