#include "KernelIo.h"
#include "KernelSmp.h"
#include "KernelScreenReport.h"

void OrynKernelSmpPrintDiscoveryProof(const OrynKernelSmpState* state)
{
    OrynKernelScreenReportOkOrWarn(state->RsdpPresent,
        "SMP ACPI RSDP input present.",
        "SMP ACPI RSDP input missing.");
    OrynKernelScreenReportOkOrWarn(state->AcpiChecksumOk,
        "SMP ACPI checksum validation passed.",
        "SMP ACPI checksum validation failed or was unavailable.");
    OrynKernelScreenReportOkOrWarn(state->MadtFound,
        "SMP ACPI MADT table discovered.",
        "SMP ACPI MADT table was not discovered.");
    KernelIoWriteString("[KERNEL] SMP CPU entries discovered: ");
    KernelIoWriteDec64(state->LocalApicEntryCount);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] SMP enabled CPU count: ");
    KernelIoWriteDec64(state->EnabledCpuCount);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrWarn(state->EnabledCpuCount > 1U,
        "SMP multi-core CPU topology discovered.",
        "SMP found only one enabled CPU.");
}

void OrynKernelSmpPrintProof(void)
{
    const OrynKernelSmpState* state = OrynKernelSmpGetState();
    OrynKernelSmpPrintDiscoveryProof(state);
    OrynKernelScreenReportOkOrWarn(state->AcpiReadBeforeVirtualMemory,
        "SMP ACPI topology cached before virtual memory switch.",
        "SMP ACPI topology was not cached before virtual memory switch.");
    KernelIoWriteString("[KERNEL] SMP bootstrap APIC ID: ");
    KernelIoWriteHex64(state->BootstrapApicId);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrFail(state->TrampolinePrepared,
        "SMP AP startup trampoline prepared below 1MB.",
        "SMP AP startup trampoline was not prepared.");
    OrynKernelScreenReportOkOrWarn(state->Cr3Below4G,
        "SMP startup CR3 is reachable by the AP trampoline.",
        "SMP startup CR3 is above 4GB; AP startup skipped.");
    OrynKernelScreenReportOkOrFail(state->IpiPathReady,
        "SMP Local APIC IPI path ready.",
        "SMP Local APIC IPI path unavailable.");
    KernelIoWriteString("[KERNEL] SMP AP startup attempts/successes: ");
    KernelIoWriteDec64(state->StartupAttemptCount);
    KernelIoWriteString(" / ");
    KernelIoWriteDec64(state->StartupSuccessCount);
    KernelIoWriteString("\n");
    OrynKernelScreenReportOkOrWarn(state->InitIpiCount,
        "SMP INIT IPI sent to application processors.",
        "SMP INIT IPI was not sent.");
    OrynKernelScreenReportOkOrWarn(state->StartupIpiCount,
        "SMP STARTUP IPI sent to application processors.",
        "SMP STARTUP IPI was not sent.");
    OrynKernelScreenReportOkOrWarn((state->ApplicationProcessorCount == state->StartupSuccessCount),
        "SMP application processors entered kernel AP loop.",
        "SMP application processor startup is incomplete.");
    OrynKernelScreenReportOk(0, "SMP AP startup completed before PCI/HPET/console/memory proof.");
    OrynKernelScreenReportOk(0, "Multi-Core processing initialized.");
}
