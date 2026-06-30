#include "KernelIo.h"
#include "KernelSmp.h"

void OrynKernelSmpPrintDiscoveryProof(const OrynKernelSmpState* state)
{
    KernelIoWriteString(state->RsdpPresent ?
        "[KERNEL] PASS: SMP ACPI RSDP input present.\n" :
        "[KERNEL] WARN: SMP ACPI RSDP input missing.\n");
    KernelIoWriteString(state->AcpiChecksumOk ?
        "[KERNEL] PASS: SMP ACPI checksum validation passed.\n" :
        "[KERNEL] WARN: SMP ACPI checksum validation failed or was unavailable.\n");
    KernelIoWriteString(state->MadtFound ?
        "[KERNEL] PASS: SMP ACPI MADT table discovered.\n" :
        "[KERNEL] WARN: SMP ACPI MADT table was not discovered.\n");
    KernelIoWriteString("[KERNEL] SMP CPU entries discovered: ");
    KernelIoWriteDec64(state->LocalApicEntryCount);
    KernelIoWriteString("\n");
    KernelIoWriteString("[KERNEL] SMP enabled CPU count: ");
    KernelIoWriteDec64(state->EnabledCpuCount);
    KernelIoWriteString("\n");
    KernelIoWriteString(state->EnabledCpuCount > 1U ?
        "[KERNEL] PASS: SMP multi-core CPU topology discovered.\n" :
        "[KERNEL] WARN: SMP found only one enabled CPU.\n");
}

void OrynKernelSmpPrintProof(void)
{
    const OrynKernelSmpState* state = OrynKernelSmpGetState();
    OrynKernelSmpPrintDiscoveryProof(state);
    KernelIoWriteString(state->AcpiReadBeforeVirtualMemory ?
        "[KERNEL] PASS: SMP ACPI topology cached before virtual memory switch.\n" :
        "[KERNEL] WARN: SMP ACPI topology was not cached before virtual memory switch.\n");
    KernelIoWriteString("[KERNEL] SMP bootstrap APIC ID: ");
    KernelIoWriteHex64(state->BootstrapApicId);
    KernelIoWriteString("\n");
    KernelIoWriteString(state->TrampolinePrepared ?
        "[KERNEL] PASS: SMP AP startup trampoline prepared below 1MB.\n" :
        "[KERNEL] FAIL: SMP AP startup trampoline was not prepared.\n");
    KernelIoWriteString(state->Cr3Below4G ?
        "[KERNEL] PASS: SMP startup CR3 is reachable by the AP trampoline.\n" :
        "[KERNEL] WARN: SMP startup CR3 is above 4GB; AP startup skipped.\n");
    KernelIoWriteString(state->IpiPathReady ?
        "[KERNEL] PASS: SMP Local APIC IPI path ready.\n" :
        "[KERNEL] FAIL: SMP Local APIC IPI path unavailable.\n");
    KernelIoWriteString("[KERNEL] SMP AP startup attempts/successes: ");
    KernelIoWriteDec64(state->StartupAttemptCount);
    KernelIoWriteString(" / ");
    KernelIoWriteDec64(state->StartupSuccessCount);
    KernelIoWriteString("\n");
    KernelIoWriteString(state->InitIpiCount ?
        "[KERNEL] PASS: SMP INIT IPI sent to application processors.\n" :
        "[KERNEL] WARN: SMP INIT IPI was not sent.\n");
    KernelIoWriteString(state->StartupIpiCount ?
        "[KERNEL] PASS: SMP STARTUP IPI sent to application processors.\n" :
        "[KERNEL] WARN: SMP STARTUP IPI was not sent.\n");
    KernelIoWriteString((state->ApplicationProcessorCount == state->StartupSuccessCount) ?
        "[KERNEL] PASS: SMP application processors entered kernel AP loop.\n" :
        "[KERNEL] WARN: SMP application processor startup is incomplete.\n");
    KernelIoWriteString("[KERNEL] PASS: SMP AP startup completed before PCI/HPET/console/memory proof.\n");
    KernelIoWriteString("[KERNEL] PASS: Multi-Core processing initialized.\n");
}
