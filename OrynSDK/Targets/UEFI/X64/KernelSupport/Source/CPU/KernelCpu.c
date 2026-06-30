#include "KernelCpu.h"
#include "KernelIo.h"

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

void OrynKernelCpuPrintFeatures(void)
{
    const OrynKernelCpuFeatures* features = OrynKernelCpuGetFeatures();
    KernelIoWriteString("[KERNEL] CPU vendor: ");
    KernelIoWriteString(features->Vendor);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] CPU max basic CPUID leaf: ");
    KernelIoWriteHex64(features->MaxBasicLeaf);
    KernelIoWriteString("\n");
    KernelIoWriteString(features->HasLocalApic ?
        "[KERNEL] PASS: CPU local APIC feature present.\n" :
        "[KERNEL] WARN: CPU local APIC feature not reported.\n");
    KernelIoWriteString(features->HasX2Apic ?
        "[KERNEL] PASS: CPU APIC2/x2APIC feature present.\n" :
        "[KERNEL] WARN: CPU APIC2/x2APIC feature not reported.\n");
    KernelIoWriteString(features->HasTscDeadline ?
        "[KERNEL] PASS: CPU TSC deadline timer feature present.\n" :
        "[KERNEL] WARN: CPU TSC deadline timer feature not reported.\n");
}
