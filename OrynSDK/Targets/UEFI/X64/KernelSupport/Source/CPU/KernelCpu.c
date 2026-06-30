#include "KernelCpu.h"
#include "KernelIo.h"

#ifndef ORYN_VM_APIC
#define ORYN_VM_APIC 1
#endif

#ifndef ORYN_VM_APIC2
#define ORYN_VM_APIC2 1
#endif

#ifndef ORYN_VM_SMP_CPUS
#define ORYN_VM_SMP_CPUS 1
#endif

static OrynKernelCpuFeatures gCpuFeatures;

static void Cpuid(
    unsigned int leaf,
    unsigned int subleaf,
    unsigned int* eax,
    unsigned int* ebx,
    unsigned int* ecx,
    unsigned int* edx)
{
    __asm__ volatile (
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf));
}

static void ClearFeatures(void)
{
    unsigned char* bytes = (unsigned char*)&gCpuFeatures;
    for (unsigned int index = 0U; index < sizeof(gCpuFeatures); ++index)
    {
        bytes[index] = 0U;
    }
}

void OrynKernelCpuDetect(void)
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    ClearFeatures();
    Cpuid(0U, 0U, &eax, &ebx, &ecx, &edx);
    gCpuFeatures.MaxBasicLeaf = eax;
    ((unsigned int*)gCpuFeatures.Vendor)[0] = ebx;
    ((unsigned int*)gCpuFeatures.Vendor)[1] = edx;
    ((unsigned int*)gCpuFeatures.Vendor)[2] = ecx;
    gCpuFeatures.Vendor[12] = 0;

    if (gCpuFeatures.MaxBasicLeaf >= 1U)
    {
        Cpuid(1U, 0U, &eax, &ebx, &ecx, &edx);
        gCpuFeatures.HasLocalApic = ((edx & (1U << 9)) != 0U) ? 1U : 0U;
        gCpuFeatures.HasX2Apic = ((ecx & (1U << 21)) != 0U) ? 1U : 0U;
        gCpuFeatures.HasTscDeadline = ((ecx & (1U << 24)) != 0U) ? 1U : 0U;
    }

    Cpuid(0x80000000U, 0U, &eax, &ebx, &ecx, &edx);
    gCpuFeatures.MaxExtendedLeaf = eax;
    if (gCpuFeatures.MaxExtendedLeaf >= 0x80000007U)
    {
        Cpuid(0x80000007U, 0U, &eax, &ebx, &ecx, &edx);
        gCpuFeatures.HasInvariantTsc = ((edx & (1U << 8)) != 0U) ? 1U : 0U;
    }

    gCpuFeatures.Detected = 1U;
}

const OrynKernelCpuFeatures* OrynKernelCpuGetFeatures(void)
{
    if (gCpuFeatures.Detected == 0U)
    {
        OrynKernelCpuDetect();
    }

    return &gCpuFeatures;
}

static void PrintCpuLocalApicStatus(const OrynKernelCpuFeatures* features)
{
    int required = (ORYN_VM_APIC || ORYN_VM_APIC2 || ORYN_VM_SMP_CPUS > 1) ? 1 : 0;
    if (features->HasLocalApic)
    {
        KernelIoWriteString("[KERNEL] PASS: CPU local APIC feature present.\n");
    }
    else if (required)
    {
        KernelIoWriteString("[KERNEL] FAIL: CPU local APIC feature required but not reported.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] PASS: CPU local APIC feature not required by this profile.\n");
    }
}

static void PrintCpuApic2Status(const OrynKernelCpuFeatures* features)
{
    if (features->HasX2Apic)
    {
        KernelIoWriteString("[KERNEL] PASS: CPU APIC2/x2APIC feature present.\n");
    }
    else if (ORYN_VM_APIC2)
    {
        KernelIoWriteString("[KERNEL] FAIL: CPU APIC2/x2APIC feature required but not reported.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] PASS: CPU APIC2/x2APIC feature not required by this profile.\n");
    }
}

static void PrintCpuTimerStatus(const OrynKernelCpuFeatures* features)
{
    if (features->HasTscDeadline)
    {
        KernelIoWriteString("[KERNEL] PASS: CPU TSC deadline timer feature present.\n");
    }
    else
    {
        KernelIoWriteString("[KERNEL] PASS: CPU TSC deadline timer feature optional for this profile.\n");
    }
}

void OrynKernelCpuPrintFeatures(void)
{
    const OrynKernelCpuFeatures* features = OrynKernelCpuGetFeatures();
    KernelIoWriteString("[KERNEL] CPU vendor: ");
    KernelIoWriteString(features->Vendor);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] CPU max basic CPUID leaf: ");
    KernelIoWriteHex64(features->MaxBasicLeaf);
    KernelIoWriteString("\n");
    PrintCpuLocalApicStatus(features);
    PrintCpuApic2Status(features);
    PrintCpuTimerStatus(features);
}
