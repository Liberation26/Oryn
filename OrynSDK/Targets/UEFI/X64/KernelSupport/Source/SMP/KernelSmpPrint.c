#include "KernelDiagnosticsLogger.h"
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
    OrynKernelDiagnosticsLogText("[KERNEL] SMP CPU entries discovered: ");
    OrynKernelDiagnosticsLogDec64(state->LocalApicEntryCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] SMP enabled CPU count: ");
    OrynKernelDiagnosticsLogDec64(state->EnabledCpuCount);
    OrynKernelDiagnosticsLogText("\n");
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
    OrynKernelDiagnosticsLogText("[KERNEL] SMP bootstrap APIC ID: ");
    OrynKernelDiagnosticsLogHex64(state->BootstrapApicId);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelScreenReportOkOrFail(state->TrampolinePrepared,
        "SMP AP startup trampoline prepared below 1MB.",
        "SMP AP startup trampoline was not prepared.");
    OrynKernelScreenReportOkOrWarn(state->Cr3Below4G,
        "SMP startup CR3 is reachable by the AP trampoline.",
        "SMP startup CR3 is above 4GB; AP startup skipped.");
    OrynKernelScreenReportOkOrFail(state->IpiPathReady,
        "SMP Local APIC IPI path ready.",
        "SMP Local APIC IPI path unavailable.");
    OrynKernelDiagnosticsLogText("[KERNEL] SMP AP startup attempts/successes: ");
    OrynKernelDiagnosticsLogDec64(state->StartupAttemptCount);
    OrynKernelDiagnosticsLogText(" / ");
    OrynKernelDiagnosticsLogDec64(state->StartupSuccessCount);
    OrynKernelDiagnosticsLogText("\n");
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
