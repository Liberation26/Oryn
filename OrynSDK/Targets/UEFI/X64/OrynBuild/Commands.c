#include "OrynBuild.h"
#include "CommandsSupport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int TextEqualsIgnoreCaseCommand(const char* left, const char* right)
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (*left != 0 && *right != 0)
    {
        char a = *left;
        char b = *right;
        if (a >= 'A' && a <= 'Z')
        {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z')
        {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b)
        {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == 0 && *right == 0;
}

static int ProjectBoolEnabled(const char* value, int default_value)
{
    if (value == 0 || value[0] == 0)
    {
        return default_value;
    }

    if (TextEqualsIgnoreCaseCommand(value, "on") ||
        TextEqualsIgnoreCaseCommand(value, "yes") ||
        TextEqualsIgnoreCaseCommand(value, "true") ||
        TextEqualsIgnoreCaseCommand(value, "1") ||
        TextEqualsIgnoreCaseCommand(value, "enabled"))
    {
        return 1;
    }

    if (TextEqualsIgnoreCaseCommand(value, "off") ||
        TextEqualsIgnoreCaseCommand(value, "no") ||
        TextEqualsIgnoreCaseCommand(value, "false") ||
        TextEqualsIgnoreCaseCommand(value, "0") ||
        TextEqualsIgnoreCaseCommand(value, "disabled"))
    {
        return 0;
    }

    return default_value;
}

static unsigned int ProjectCpuCount(const OrynProject* project)
{
    char* end = 0;
    unsigned long value = strtoul(project->run_smp, &end, 10);
    if (end == project->run_smp || *end != 0 || value == 0UL)
    {
        return 1U;
    }
    if (value > 64UL)
    {
        value = 64UL;
    }
    return (unsigned int)value;
}

static const char* OnOffText(int value)
{
    return value ? "on" : "off";
}


static const char* ResolveQemuCpuModel(const char* configured_cpu)
{
    if (configured_cpu == 0 || configured_cpu[0] == 0)
    {
        return "qemu64";
    }

    if (TextEqualsIgnoreCaseCommand(configured_cpu, "host") ||
        TextEqualsIgnoreCaseCommand(configured_cpu, "native"))
    {
        return "max";
    }

    return configured_cpu;
}

static int QemuCpuWasTranslated(const char* configured_cpu, const char* resolved_cpu)
{
    return configured_cpu != 0 && resolved_cpu != 0 &&
        !TextEqualsIgnoreCaseCommand(configured_cpu, resolved_cpu);
}

static int DebugTextIsEmpty(const char* text)
{
    return text == 0 || text[0] == 0;
}

static int IsSafeQemuArgumentValue(const char* value)
{
    if (value == 0 || value[0] == 0)
    {
        return 0;
    }

    for (int index = 0; value[index] != 0; ++index)
    {
        char ch = value[index];
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_' || ch == '-' || ch == '+' || ch == ',' ||
              ch == '=' || ch == ':' || ch == '.'))
        {
            return 0;
        }
    }

    return 1;
}

static void BuildProjectImagePath(const OrynProject* project, char* output, size_t output_size)
{
    char image_name[256];
    snprintf(image_name, sizeof(image_name), "%s.img", project->name);
    OrynJoinPath(output, output_size, project->output_dir, image_name);
}

static void WriteBootReport(
    const OrynProject* project,
    const char* report_path,
    const char* qemu_path,
    const char* ovmf_windows,
    const char* ovmf_qemu,
    const char* disk_windows,
    const char* debug_log,
    const char* resolved_cpu,
    const char* command,
    const char* debug_text,
    int exit_code,
    int command_ok)
{
    int loader_started = TextContains(debug_text, "[BOOT] Stage 01");
    int kernel_loaded = TextContains(debug_text, "[BOOT] Kernel loaded physical base");
    int entry_printed = TextContains(debug_text, "[BOOT] Kernel virtual entry address");
    int virtual_map_prepared = TextContains(debug_text, "[BOOT] PASS: Temporary higher-half/chosen virtual map prepared");
    int virtual_map_active = TextContains(debug_text, "[BOOT] PASS: Temporary higher-half/chosen virtual map activated");
    int boot_services_exited = TextContains(debug_text, "[BOOT] Stage 07: ExitBootServices succeeded");
    int bootinfo_created = TextContains(debug_text, "[BOOT] BootInfo allocated at");
    int boot_config_prepared = TextContains(debug_text, "[BOOT] PASS: Boot configuration block prepared");
    int memory_map_requested = !TextContains(debug_text, "[BOOT] BootInfo selection memory map: disabled");
    int bootinfo_memory_map = !memory_map_requested || TextContains(debug_text, "[BOOT] BootInfo memory map entries");
    int kernel_jump = TextContains(debug_text, "[BOOT] Stage 08: Jumping to kernel entry");
    int kernel_entered = TextContains(debug_text, "[KERNEL] PASS: Kernel entered successfully") ||
        TextContains(debug_text, "[KERNEL] Oryn Kernel-5 entered.");
    int serial_ok = TextContains(debug_text, "[KERNEL] PASS: Serial/debug output path is working");
    int bootinfo_received = TextContains(debug_text, "[KERNEL] PASS: BootInfo received");
    int kernel_boot_config = TextContains(debug_text, "[KERNEL] PASS: Boot configuration block received");
    int kernel_command_line = TextContains(debug_text, "[KERNEL] PASS: Kernel command line received");
    int gdt_installing = TextContains(debug_text, "[KERNEL] GDT: installing");
    int gdt_installed = TextContains(debug_text, "[KERNEL] PASS: GDT installed.");
    int gdt_entries = TextContains(debug_text, "[KERNEL] GDT entries: 7");
    int tss_loaded = TextContains(debug_text, "[KERNEL] PASS: TSS loaded.");
    int idt_installing = TextContains(debug_text, "[KERNEL] IDT: installing");
    int idt_installed = TextContains(debug_text, "[KERNEL] PASS: IDT installed.");
    int idt_entries = TextContains(debug_text, "[KERNEL] IDT entries: 256");
    int interrupt_dispatcher = TextContains(debug_text, "[KERNEL] PASS: Interrupt dispatcher initialized.");
    int interrupt_handlers = TextContains(debug_text, "[KERNEL] PASS: Interrupt handler table ready for 256 vectors.");
    int interrupt_controlled = TextContains(debug_text, "[KERNEL] PASS: CPU interrupts are currently disabled for controlled boot.");
    int cpu_exception = TextContains(debug_text, "[KERNEL] EXCEPTION:");
    int cpu_local_apic = TextContains(debug_text, "[KERNEL] PASS: CPU local APIC feature present.");
    int cpu_apic2 = TextContains(debug_text, "[KERNEL] PASS: CPU APIC2/x2APIC feature present.");
    int pic_initialized = TextContains(debug_text, "[KERNEL] PASS: PIC initialized.");
    int pic_remapped = TextContains(debug_text, "[KERNEL] PASS: PIC remapped to vectors 0x20-0x2F.");
    int pic_masked = TextContains(debug_text, "[KERNEL] PASS: PIC masked/disabled for APIC handoff.");
    int apic_available = TextContains(debug_text, "[KERNEL] PASS: APIC CPU feature available.");
    int apic2_enabled = TextContains(debug_text, "[KERNEL] PASS: APIC2/x2APIC mode enabled.");
    int local_apic_enabled = TextContains(debug_text, "[KERNEL] PASS: Local APIC software enable bit set.");
    int apic_timer_probe = TextContains(debug_text, "[KERNEL] PASS: APIC timer counter moved in masked probe.");
    int hpet_rsdp = TextContains(debug_text, "[KERNEL] PASS: HPET ACPI RSDP input present.");
    int hpet_checksum = TextContains(debug_text, "[KERNEL] PASS: HPET ACPI checksum validation passed.");
    int hpet_table = TextContains(debug_text, "[KERNEL] PASS: HPET ACPI table discovered.");
    int hpet_enabled = TextContains(debug_text, "[KERNEL] PASS: HPET main counter enabled.");
    int hpet_counter = TextContains(debug_text, "[KERNEL] PASS: HPET counter advanced in probe.");
    int pic_irq0 = TextContains(debug_text, "[KERNEL] PASS: PIC IRQ0 interrupt fired through IDT dispatch.");
    int pic_irq0_count = TextContains(debug_text, "[KERNEL] PASS: PIC IRQ0 handler counter updated.");
    int pic_eoi = TextContains(debug_text, "[KERNEL] PASS: PIC EOI path executed.");
    int apic_timer_interrupt = TextContains(debug_text, "[KERNEL] PASS: APIC timer interrupt fired through IDT dispatch.");
    int apic_irq_counters = TextContains(debug_text, "[KERNEL] PASS: APIC timer IRQ counter updated by interrupt dispatch.");
    int apic_eoi = TextContains(debug_text, "[KERNEL] PASS: APIC EOI path executed.");
    int interrupt_chain = TextContains(debug_text, "[KERNEL] PASS: Interrupts work from PIC upward through APIC/APIC2.");
    int syscall_core = TextContains(debug_text, "[KERNEL] PASS: SysCall core initialized.");
    int syscall_packet = TextContains(debug_text, "[KERNEL] PASS: SysCall message packet ABI ready.");
    int syscall_get = TextContains(debug_text, "[KERNEL] PASS: SysCallGet packet handled.");
    int syscall_set = TextContains(debug_text, "[KERNEL] PASS: SysCallSet packet handled.");
    int syscall_event = TextContains(debug_text, "[KERNEL] PASS: SysCallEvent packet handled.");
    int syscall_unknown = TextContains(debug_text, "[KERNEL] PASS: Unknown syscall debug logging path executed.");
    int linux_translator = TextContains(debug_text, "[KERNEL] PASS: LinuxSysCall translator registered.");
    int ms_translator = TextContains(debug_text, "[KERNEL] PASS: MSSysCall translator registered.");
    int linux_vector = TextContains(debug_text, "[KERNEL] PASS: Linux syscall vector 0x80 received and translated.");
    int ms_vector = TextContains(debug_text, "[KERNEL] PASS: MS syscall vector 0x81 received and translated.");
    int unknown_linux = TextContains(debug_text, "[KERNEL] PASS: Unknown Linux syscall was reported as debug.");
    int unknown_ms = TextContains(debug_text, "[KERNEL] PASS: Unknown MS syscall was reported as debug.");
    int syscall_three = TextContains(debug_text, "[KERNEL] PASS: SysCalls use Get/Set/Event message packets.");
    int platform_syscalls = TextContains(debug_text, "[KERNEL] PASS: Platform syscalls translate into Get/Set/Event packets.");
    int syscall_counts = TextContains(debug_text, "[KERNEL] PASS: SysCall header counts listed.");
    int pci_started = TextContains(debug_text, "[KERNEL] PCI: discovery starting.");
    int pci_rsdp = TextContains(debug_text, "[KERNEL] PASS: PCI ACPI RSDP input present.");
    int pci_checksum = TextContains(debug_text, "[KERNEL] PASS: PCI ACPI checksum validation passed.");
    int pci_mcfg = TextContains(debug_text, "[KERNEL] PASS: PCI ACPI MCFG table discovered.");
    int pci_ecam = TextContains(debug_text, "[KERNEL] PASS: PCIe ECAM descriptor captured.");
    int pci_config = TextContains(debug_text, "[KERNEL] PASS: PCI config mechanism #1 responded.");
    int pci_scan = TextContains(debug_text, "[KERNEL] PASS: PCI bus/device/function scan completed.");
    int pci_devices = TextContains(debug_text, "[KERNEL] PASS: PCI devices discovered.");
    int pci_class = TextContains(debug_text, "[KERNEL] PASS: PCI class-code decoding ready.");
    int pci_complete = TextContains(debug_text, "[KERNEL] PASS: PCI Discovery complete.");
    int pci_english = TextContains(debug_text, "[KERNEL] PASS: PCI device output uses English labels.");
    int smp_started = TextContains(debug_text, "[KERNEL] SMP: multi-core processing discovery starting.");
    int smp_rsdp = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI RSDP input present.");
    int smp_checksum = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI checksum validation passed.");
    int smp_madt = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI MADT table discovered.");
    int smp_cached = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI topology cached before virtual memory switch.");
    int smp_topology = TextContains(debug_text, "[KERNEL] PASS: SMP multi-core CPU topology discovered.");
    int smp_trampoline = TextContains(debug_text, "[KERNEL] PASS: SMP AP startup trampoline prepared below 1MB.");
    int smp_cr3 = TextContains(debug_text, "[KERNEL] PASS: SMP startup CR3 is reachable by the AP trampoline.");
    int smp_ipi = TextContains(debug_text, "[KERNEL] PASS: SMP Local APIC IPI path ready.");
    int smp_init_ipi = TextContains(debug_text, "[KERNEL] PASS: SMP INIT IPI sent to application processors.");
    int smp_startup_ipi = TextContains(debug_text, "[KERNEL] PASS: SMP STARTUP IPI sent to application processors.");
    int smp_aps_started = TextContains(debug_text, "[KERNEL] PASS: SMP application processors entered kernel AP loop.");
    int smp_complete = TextContains(debug_text, "[KERNEL] PASS: Multi-Core processing initialized.");
    int qemu_debug_colour = TextContains(debug_text, "\033[32m[KERNEL] PASS") &&
        TextContains(debug_text, "\033[0m");
    int kernel_console = TextContains(debug_text, "[KERNEL] PASS: Kernel console initialized.");
    int screen_scrollback = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scrollback buffer initialized.");
    int screen_coloured_cells = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scrollback stores coloured cells.");
    int screen_scroll_lines = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scroll up/down works.");
    int screen_page_scroll = TextContains(debug_text, "[KERNEL] PASS: Kernel screen page up/down works.");
    int screen_bottom = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scroll-to-bottom works.");
    int screen_scrolling = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scrolling implemented.");
    int screen_back_buffer = TextContains(debug_text, "[KERNEL] PASS: Kernel screen back buffer allocated.");
    int screen_renders_back = TextContains(debug_text, "[KERNEL] PASS: Kernel screen renders into back buffer first.");
    int screen_present = TextContains(debug_text, "[KERNEL] PASS: Kernel screen presents completed frame to visible output.");
    int screen_double_buffer = TextContains(debug_text, "[KERNEL] PASS: Kernel screen double buffering implemented.");
    int qemu_preboot_failure = (!command_ok) && !loader_started && DebugTextIsEmpty(debug_text);

    int want_pic = ProjectBoolEnabled(project->run_pic, 1);
    int want_apic = ProjectBoolEnabled(project->run_apic, 1);
    int want_apic2 = ProjectBoolEnabled(project->run_apic2, 1) && want_apic;
    int want_hpet = ProjectBoolEnabled(project->run_hpet, 1);
    unsigned int wanted_cpu_count = ProjectCpuCount(project);
    int want_smp = (wanted_cpu_count > 1U) && want_apic;
    int pic_skipped = TextContains(debug_text, "[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.");
    int apic_skipped = TextContains(debug_text, "[KERNEL] INFO: APIC proofs skipped by VMSettings.");
    int apic2_skipped = TextContains(debug_text, "[KERNEL] INFO: APIC2/x2APIC disabled by VMSettings.");
    int hpet_skipped = TextContains(debug_text, "[KERNEL] INFO: HPET proof skipped by VMSettings.");
    int smp_skipped = TextContains(debug_text, "[KERNEL] INFO: SMP AP startup skipped by VMSettings.");

    int pic_ok = want_pic ? (pic_initialized && pic_remapped && pic_irq0 && pic_irq0_count && pic_eoi && pic_masked) : pic_skipped;
    int apic_ok = want_apic ? (apic_available && local_apic_enabled && apic_timer_probe && apic_timer_interrupt && apic_irq_counters && apic_eoi) : apic_skipped;
    int apic2_ok = want_apic2 ? (cpu_apic2 && apic2_enabled) : (apic2_skipped || !want_apic);
    int hpet_ok = want_hpet ? (hpet_rsdp && hpet_checksum && hpet_table && hpet_enabled && hpet_counter) : hpet_skipped;
    int interrupt_chain_ok = TextContains(debug_text, "[KERNEL] PASS: Interrupts work from PIC upward through APIC/APIC2.") ||
        TextContains(debug_text, "[KERNEL] PASS: VMSettings interrupt/timer profile applied.");
    int smp_ok = want_smp ? (smp_started && smp_rsdp && smp_checksum && smp_madt && smp_cached &&
        smp_topology && smp_trampoline && smp_cr3 && smp_ipi && smp_init_ipi &&
        smp_startup_ipi && smp_aps_started && smp_complete) : smp_skipped;

    int virtual_memory_started = TextContains(debug_text, "[KERNEL] Virtual memory: starting");
    int virtual_memory_required_mapped = TextContains(debug_text, "[KERNEL] Virtual memory: required ranges mapped");
    int virtual_memory_switching_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: switching CR3 to kernel-owned PML4");
    int virtual_memory_switched_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: CR3 switched to kernel-owned PML4");
    int virtual_memory_active = TextContains(debug_text, "[KERNEL] Virtual memory: active");
    int system_halted = TextContains(debug_text, "[KERNEL] System halted by Kernel-5");
    int debug_exit = TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success");
    int boot_pass = command_ok && (exit_code == 0 || exit_code == 33) && loader_started &&
        kernel_loaded && entry_printed && virtual_map_prepared && virtual_map_active &&
        bootinfo_created && boot_config_prepared && bootinfo_memory_map && boot_services_exited && kernel_jump &&
        kernel_entered && serial_ok && bootinfo_received && kernel_boot_config && kernel_command_line &&
        gdt_installing && gdt_installed && gdt_entries && tss_loaded &&
        idt_installing && idt_installed && idt_entries &&
        interrupt_dispatcher && interrupt_handlers && interrupt_controlled && !cpu_exception &&
        cpu_local_apic && pic_ok && apic_ok && apic2_ok && hpet_ok &&
        interrupt_chain_ok && syscall_core &&
        syscall_packet && syscall_get && syscall_set && syscall_event && syscall_unknown &&
        linux_translator && ms_translator && linux_vector && ms_vector &&
        unknown_linux && unknown_ms && syscall_three && platform_syscalls && syscall_counts &&
        pci_started && pci_rsdp && pci_checksum && pci_mcfg && pci_ecam && pci_config &&
        pci_scan && pci_devices && pci_class && pci_complete && pci_english &&
        smp_ok && qemu_debug_colour && kernel_console && screen_scrollback &&
        screen_coloured_cells && screen_scroll_lines && screen_page_scroll &&
        screen_bottom && screen_scrolling && screen_back_buffer && screen_renders_back &&
        screen_present && screen_double_buffer && virtual_memory_active && debug_exit;

    FILE* file = fopen(report_path, "wb");
    if (file == 0)
    {
        OrynLogWarn("Could not write Output/BootReport.txt.");
        return;
    }

    fprintf(file, "Oryn WSL SDK Boot Report\n");
    fprintf(file, "Version: %s\n", ORYN_VERSION);
    fprintf(file, "Project: %s\n", project->name);
    fprintf(file, "Target: %s\n", project->target);
    fprintf(file, "Toolchain: %s\n", project->toolchain);
    fprintf(file, "VM: %s\n", project->run_vm);
    fprintf(file, "VM Memory: %s\n", project->run_memory);
    fprintf(file, "VM CPU: %s\n", project->run_cpu);
    if (resolved_cpu != 0 && QemuCpuWasTranslated(project->run_cpu, resolved_cpu))
    {
        fprintf(file, "VM CPU resolved for QEMU: %s\n", resolved_cpu);
    }
    fprintf(file, "VM SMP CPUs: %s\n", project->run_smp);
    fprintf(file, "VM PIC/APIC/APIC2/HPET: %s/%s/%s/%s\n",
        project->run_pic, project->run_apic, project->run_apic2, project->run_hpet);
    fprintf(file, "VM disk format/interface: %s/%s\n",
        project->run_disk_format, project->run_storage_interface);
    fprintf(file, "Result: %s\n", PassFail(boot_pass));
    fprintf(file, "QEMU exit code: %d\n\n", exit_code);

    fprintf(file, "Checks:\n");
    fprintf(file, "  QEMU command accepted: %s\n", PassFail(command_ok));
    if (qemu_preboot_failure)
    {
        fprintf(file, "  Firmware, loader, and kernel checks: SKIPPED - QEMU exited before UEFI firmware started.\n");
        fprintf(file, "\nFailure hint:\n");
        if (TextEqualsIgnoreCaseCommand(project->run_cpu, "host") ||
            TextEqualsIgnoreCaseCommand(project->run_cpu, "native"))
        {
            fprintf(file, "  CPU=%s requires hardware acceleration and is not valid for the current Windows QEMU from WSL runner. Use CPU=max or CPU=qemu64.\n\n", project->run_cpu);
        }
        else
        {
            fprintf(file, "  QEMU exited before UEFI firmware started. Check the VM CPU, memory, acceleration, and QEMU command line above.\n\n");
        }
        fprintf(file, "Paths:\n");
        fprintf(file, "  QEMU: %s\n", qemu_path);
        fprintf(file, "  OVMF Windows: %s\n", ovmf_windows);
        fprintf(file, "  OVMF QEMU: %s\n", ovmf_qemu);
        fprintf(file, "  FAT32 disk image: %s\n", disk_windows);
        fprintf(file, "  Debug log: %s\n", debug_log);
        fprintf(file, "  Boot report: %s\n\n", report_path);
        fprintf(file, "Command:\n%s\n\n", command);
        fprintf(file, "Captured debug output:\n%s\n", DebugTextIsEmpty(debug_text) ? "[empty]" : debug_text);
        fclose(file);
        return;
    }
    fprintf(file, "  Loader started: %s\n", PassFail(loader_started));
    fprintf(file, "  Kernel loaded physical address printed: %s\n", PassFail(kernel_loaded));
    fprintf(file, "  Kernel virtual entry address printed: %s\n", PassFail(entry_printed));
    fprintf(file, "  Loader prepared temporary higher-half/chosen map: %s\n", PassFail(virtual_map_prepared));
    fprintf(file, "  Loader activated temporary higher-half/chosen map: %s\n", PassFail(virtual_map_active));
    fprintf(file, "  BootInfo allocated: %s\n", PassFail(bootinfo_created));
    fprintf(file, "  Loader prepared boot configuration block: %s\n", PassFail(boot_config_prepared));
    fprintf(file, "  BootInfo memory map requested: %s\n", PassFail(memory_map_requested));
    fprintf(file, "  BootInfo memory map captured or intentionally omitted: %s\n", PassFail(bootinfo_memory_map));
    fprintf(file, "  ExitBootServices succeeded: %s\n", PassFail(boot_services_exited));
    fprintf(file, "  Loader jumped to kernel: %s\n", PassFail(kernel_jump));
    fprintf(file, "  Kernel entered: %s\n", PassFail(kernel_entered));
    fprintf(file, "  Serial/debug output reached kernel: %s\n", PassFail(serial_ok));
    fprintf(file, "  Kernel received BootInfo: %s\n", PassFail(bootinfo_received));
    fprintf(file, "  Kernel received boot configuration block: %s\n", PassFail(kernel_boot_config));
    fprintf(file, "  Kernel received command line: %s\n", PassFail(kernel_command_line));
    fprintf(file, "  Kernel started GDT install: %s\n", PassFail(gdt_installing));
    fprintf(file, "  Kernel installed and verified GDT: %s\n", PassFail(gdt_installed));
    fprintf(file, "  Kernel installed all 7 GDT entries: %s\n", PassFail(gdt_entries));
    fprintf(file, "  Kernel loaded TSS: %s\n", PassFail(tss_loaded));
    fprintf(file, "  Kernel started IDT install: %s\n", PassFail(idt_installing));
    fprintf(file, "  Kernel installed and verified IDT: %s\n", PassFail(idt_installed));
    fprintf(file, "  Kernel installed all 256 IDT entries: %s\n", PassFail(idt_entries));
    fprintf(file, "  Interrupt dispatcher initialized: %s\n", PassFail(interrupt_dispatcher));
    fprintf(file, "  Interrupt handler table ready: %s\n", PassFail(interrupt_handlers));
    fprintf(file, "  CPU interrupts controlled during boot: %s\n", PassFail(interrupt_controlled));
    fprintf(file, "  Kernel had no trapped CPU exception: %s\n", PassFail(!cpu_exception));
    fprintf(file, "  CPU local APIC feature present: %s\n", PassFail(cpu_local_apic));
    fprintf(file, "  CPU APIC2/x2APIC feature present: %s\n", PassFail(want_apic2 ? cpu_apic2 : 1));
    fprintf(file, "  PIC initialized: %s\n", PassFail(want_pic ? pic_initialized : pic_skipped));
    fprintf(file, "  PIC remapped to 0x20-0x2F: %s\n", PassFail(want_pic ? pic_remapped : pic_skipped));
    fprintf(file, "  PIC IRQ0 interrupt fired: %s\n", PassFail(want_pic ? pic_irq0 : pic_skipped));
    fprintf(file, "  PIC IRQ0 counter updated: %s\n", PassFail(want_pic ? pic_irq0_count : pic_skipped));
    fprintf(file, "  PIC EOI path executed: %s\n", PassFail(want_pic ? pic_eoi : pic_skipped));
    fprintf(file, "  PIC masked for APIC handoff: %s\n", PassFail(want_pic ? pic_masked : pic_skipped));
    fprintf(file, "  APIC CPU feature available: %s\n", PassFail(want_apic ? apic_available : apic_skipped));
    fprintf(file, "  APIC2/x2APIC mode enabled: %s\n", PassFail(want_apic2 ? apic2_enabled : 1));
    fprintf(file, "  Local APIC software enabled: %s\n", PassFail(want_apic ? local_apic_enabled : apic_skipped));
    fprintf(file, "  APIC timer masked probe counted down: %s\n", PassFail(want_apic ? apic_timer_probe : apic_skipped));
    fprintf(file, "  HPET RSDP input present: %s\n", PassFail(want_hpet ? hpet_rsdp : hpet_skipped));
    fprintf(file, "  HPET ACPI checksum validated: %s\n", PassFail(want_hpet ? hpet_checksum : hpet_skipped));
    fprintf(file, "  HPET table discovered: %s\n", PassFail(want_hpet ? hpet_table : hpet_skipped));
    fprintf(file, "  HPET main counter enabled: %s\n", PassFail(want_hpet ? hpet_enabled : hpet_skipped));
    fprintf(file, "  HPET counter advanced: %s\n", PassFail(want_hpet ? hpet_counter : hpet_skipped));
    fprintf(file, "  APIC timer interrupt fired: %s\n", PassFail(want_apic ? apic_timer_interrupt : apic_skipped));
    fprintf(file, "  APIC timer IRQ counter updated: %s\n", PassFail(want_apic ? apic_irq_counters : apic_skipped));
    fprintf(file, "  APIC EOI path executed: %s\n", PassFail(want_apic ? apic_eoi : apic_skipped));
    fprintf(file, "  Interrupt chain PIC upward complete: %s\n", PassFail(interrupt_chain_ok));
    fprintf(file, "  SysCall core initialized: %s\n", PassFail(syscall_core));
    fprintf(file, "  SysCall message packet ABI ready: %s\n", PassFail(syscall_packet));
    fprintf(file, "  SysCallGet packet handled: %s\n", PassFail(syscall_get));
    fprintf(file, "  SysCallSet packet handled: %s\n", PassFail(syscall_set));
    fprintf(file, "  SysCallEvent packet handled: %s\n", PassFail(syscall_event));
    fprintf(file, "  Unknown syscall debug log path: %s\n", PassFail(syscall_unknown));
    fprintf(file, "  LinuxSysCall translator registered: %s\n", PassFail(linux_translator));
    fprintf(file, "  MSSysCall translator registered: %s\n", PassFail(ms_translator));
    fprintf(file, "  Linux syscall vector 0x80 translated: %s\n", PassFail(linux_vector));
    fprintf(file, "  MS syscall vector 0x81 translated: %s\n", PassFail(ms_vector));
    fprintf(file, "  Unknown Linux syscall debug report: %s\n", PassFail(unknown_linux));
    fprintf(file, "  Unknown MS syscall debug report: %s\n", PassFail(unknown_ms));
    fprintf(file, "  Internal SysCalls use Get/Set/Event packets: %s\n", PassFail(syscall_three));
    fprintf(file, "  Platform syscalls translate to Get/Set/Event: %s\n", PassFail(platform_syscalls));
    fprintf(file, "  SysCall header counts listed: %s\n", PassFail(syscall_counts));
    fprintf(file, "  PCI discovery started: %s\n", PassFail(pci_started));
    fprintf(file, "  PCI ACPI RSDP present: %s\n", PassFail(pci_rsdp));
    fprintf(file, "  PCI ACPI checksum validated: %s\n", PassFail(pci_checksum));
    fprintf(file, "  PCI ACPI MCFG table discovered: %s\n", PassFail(pci_mcfg));
    fprintf(file, "  PCIe ECAM descriptor captured: %s\n", PassFail(pci_ecam));
    fprintf(file, "  PCI config mechanism #1 responded: %s\n", PassFail(pci_config));
    fprintf(file, "  PCI bus/device/function scan completed: %s\n", PassFail(pci_scan));
    fprintf(file, "  PCI devices discovered: %s\n", PassFail(pci_devices));
    fprintf(file, "  PCI class-code decoding ready: %s\n", PassFail(pci_class));
    fprintf(file, "  PCI discovery complete: %s\n", PassFail(pci_complete));
    fprintf(file, "  PCI device output uses English labels: %s\n", PassFail(pci_english));
    fprintf(file, "  SMP discovery started: %s\n", PassFail(want_smp ? smp_started : smp_skipped));
    fprintf(file, "  SMP ACPI RSDP present: %s\n", PassFail(want_smp ? smp_rsdp : smp_skipped));
    fprintf(file, "  SMP ACPI checksum validated: %s\n", PassFail(want_smp ? smp_checksum : smp_skipped));
    fprintf(file, "  SMP ACPI MADT table discovered: %s\n", PassFail(want_smp ? smp_madt : smp_skipped));
    fprintf(file, "  SMP ACPI topology cached before VM: %s\n", PassFail(want_smp ? smp_cached : smp_skipped));
    fprintf(file, "  SMP multi-core topology discovered: %s\n", PassFail(want_smp ? smp_topology : smp_skipped));
    fprintf(file, "  SMP AP trampoline prepared: %s\n", PassFail(want_smp ? smp_trampoline : smp_skipped));
    fprintf(file, "  SMP startup CR3 reachable: %s\n", PassFail(want_smp ? smp_cr3 : smp_skipped));
    fprintf(file, "  SMP Local APIC IPI path ready: %s\n", PassFail(want_smp ? smp_ipi : smp_skipped));
    fprintf(file, "  SMP INIT IPI sent: %s\n", PassFail(want_smp ? smp_init_ipi : smp_skipped));
    fprintf(file, "  SMP STARTUP IPI sent: %s\n", PassFail(want_smp ? smp_startup_ipi : smp_skipped));
    fprintf(file, "  SMP APs entered kernel loop: %s\n", PassFail(want_smp ? smp_aps_started : smp_skipped));
    fprintf(file, "  Multi-Core processing initialized: %s\n", PassFail(want_smp ? smp_complete : smp_skipped));
    fprintf(file, "  QEMU debug output includes ANSI colour: %s\n", PassFail(qemu_debug_colour));
    fprintf(file, "  Kernel console initialized: %s\n", PassFail(kernel_console));
    fprintf(file, "  Kernel screen scrollback buffer initialized: %s\n", PassFail(screen_scrollback));
    fprintf(file, "  Kernel screen coloured scrollback cells: %s\n", PassFail(screen_coloured_cells));
    fprintf(file, "  Kernel screen line scroll up/down: %s\n", PassFail(screen_scroll_lines));
    fprintf(file, "  Kernel screen page up/down: %s\n", PassFail(screen_page_scroll));
    fprintf(file, "  Kernel screen scroll-to-bottom: %s\n", PassFail(screen_bottom));
    fprintf(file, "  Kernel screen scrolling implemented: %s\n", PassFail(screen_scrolling));
    fprintf(file, "  Kernel screen back buffer allocated: %s\n", PassFail(screen_back_buffer));
    fprintf(file, "  Kernel screen renders into back buffer first: %s\n", PassFail(screen_renders_back));
    fprintf(file, "  Kernel screen presents completed frame: %s\n", PassFail(screen_present));
    fprintf(file, "  Kernel screen double buffering implemented: %s\n", PassFail(screen_double_buffer));
    fprintf(file, "  Kernel virtual memory started: %s\n", PassFail(virtual_memory_started));
    fprintf(file, "  Kernel virtual memory required ranges mapped: %s\n", PassFail(virtual_memory_required_mapped));
    fprintf(file, "  Kernel virtual memory CR3 switch requested: %s\n", PassFail(virtual_memory_switching_cr3));
    fprintf(file, "  Kernel virtual memory CR3 switch completed: %s\n", PassFail(virtual_memory_switched_cr3));
    fprintf(file, "  Kernel virtual memory active proof printed: %s\n", PassFail(virtual_memory_active));
    fprintf(file, "  Kernel reached halt path: %s\n", PassFail(system_halted));
    fprintf(file, "  Kernel requested QEMU debug-exit: %s\n", PassFail(debug_exit));

    fprintf(file, "\nFailure hint:\n  %s\n\n",
        boot_pass ?
            "No kernel-side failure detected by the boot-proof markers." :
        !command_ok ?
            "QEMU did not accept the VM command line. Check CPU, memory, and acceleration settings." :
        !loader_started ?
            "The loader did not start." :
        !kernel_jump ?
            "The loader did not reach the kernel jump." :
        !kernel_entered ?
            "The kernel did not print the entry success marker." :
        !kernel_boot_config ?
            "The kernel did not validate the boot configuration block." :
        !kernel_command_line ?
            "The kernel did not validate the command line." :
        !gdt_installing ?
            "The kernel did not start GDT installation." :
        !gdt_installed ?
            "The kernel did not verify the GDT install." :
        !gdt_entries ?
            "The kernel did not install the expected GDT entries." :
        !tss_loaded ?
            "The kernel did not load the TSS selector." :
        !idt_installing ?
            "The kernel did not start IDT installation." :
        !idt_installed ?
            "The kernel did not verify the IDT install." :
        !idt_entries ?
            "The kernel did not install all 256 IDT entries." :
        !interrupt_dispatcher ?
            "The interrupt dispatcher did not initialize." :
        !interrupt_handlers ?
            "The interrupt handler table was not prepared." :
        !interrupt_controlled ?
            "CPU interrupts were not under controlled CLI/STI boot state." :
        cpu_exception ?
            "The IDT trapped a CPU exception. Check the exception vector and register dump." :
        !cpu_local_apic ?
            "The CPU did not expose the local APIC feature." :
        !cpu_apic2 ?
            "QEMU did not expose APIC2/x2APIC. Check the -cpu qemu64,+x2apic option." :
        !pic_initialized ?
            "The kernel did not initialize the 8259 PIC path." :
        !pic_remapped ?
            "The kernel did not remap the PIC vectors to 0x20-0x2F." :
        !pic_masked ?
            "The kernel did not mask the PIC for APIC handoff." :
        !pic_irq0 ?
            "The remapped PIC did not deliver IRQ0 through IDT vector 0x20." :
        !pic_irq0_count ?
            "The PIC IRQ0 handler did not update its dispatch counter." :
        !pic_eoi ?
            "The PIC interrupt EOI path did not execute." :
        !apic_available ?
            "The kernel did not reach APIC feature validation." :
        !apic2_enabled ?
            "The kernel did not enable APIC2/x2APIC mode." :
        !local_apic_enabled ?
            "The kernel did not set the local APIC software enable bit." :
        !apic_timer_probe ?
            "The APIC timer masked probe did not count down." :
        !hpet_rsdp ?
            "The kernel did not receive the ACPI RSDP needed for HPET discovery." :
        !hpet_checksum ?
            "The HPET ACPI table walk failed checksum validation." :
        !hpet_table ?
            "The HPET ACPI table was not discovered." :
        !hpet_enabled ?
            "The HPET main counter was not enabled." :
        !hpet_counter ?
            "The HPET counter did not advance during the probe." :
        !apic_timer_interrupt ?
            "The APIC timer interrupt did not fire through the IDT dispatcher." :
        !apic_irq_counters ?
            "The APIC timer IRQ counter did not update." :
        !apic_eoi ?
            "The APIC EOI path did not execute." :
        !interrupt_chain ?
            "The interrupt chain did not pass from PIC upward through APIC/APIC2." :
        !syscall_core ?
            "The internal SysCall core did not initialize." :
        !syscall_packet ?
            "The SysCall message-packet ABI was not prepared." :
        !syscall_get ?
            "SysCallGet did not handle its proof packet." :
        !syscall_set ?
            "SysCallSet did not handle its proof packet." :
        !syscall_event ?
            "SysCallEvent did not handle its proof packet." :
        !syscall_unknown ?
            "Unknown syscall debug logging did not run." :
        !linux_translator ?
            "LinuxSysCall.h translator was not registered." :
        !ms_translator ?
            "MSSysCall.h translator was not registered." :
        !linux_vector ?
            "The Linux syscall vector 0x80 did not translate into the internal syscall system." :
        !ms_vector ?
            "The MS syscall vector 0x81 did not translate into the internal syscall system." :
        !unknown_linux ?
            "Unknown Linux syscall reporting did not produce a debug path." :
        !unknown_ms ?
            "Unknown MS syscall reporting did not produce a debug path." :
        !syscall_three ?
            "The internal syscall proof did not use Get/Set/Event packets." :
        !platform_syscalls ?
            "Platform syscall compatibility did not translate into Get/Set/Event packets." :
        !syscall_counts ?
            "The LinuxSysCall.h and MSSysCall.h count markers were not printed." :
        !pci_started ?
            "The PCI discovery module did not start." :
        !pci_rsdp ?
            "The PCI discovery module did not receive the ACPI RSDP." :
        !pci_checksum ?
            "The PCI ACPI table walk failed checksum validation." :
        !pci_mcfg ?
            "The ACPI MCFG table for PCIe ECAM was not discovered." :
        !pci_ecam ?
            "The PCIe ECAM descriptor was not captured from MCFG." :
        !pci_config ?
            "PCI config mechanism #1 did not respond with any devices." :
        !pci_scan ?
            "The PCI bus/device/function scan did not complete." :
        !pci_devices ?
            "The PCI scan completed but found no devices." :
        !pci_class ?
            "The PCI class-code decoder did not initialize." :
        !pci_complete ?
            "The PCI discovery module did not emit its completion marker." :
        !pci_english ?
            "The PCI device output did not emit the English-label proof marker." :
        !smp_started ?
            "The SMP discovery module did not start." :
        !smp_rsdp ?
            "The SMP module did not receive the ACPI RSDP." :
        !smp_checksum ?
            "The SMP ACPI table walk failed checksum validation." :
        !smp_madt ?
            "The ACPI MADT table was not discovered." :
        !smp_cached ?
            "SMP did not cache ACPI topology before the virtual-memory CR3 switch." :
        !smp_topology ?
            "SMP did not discover more than one enabled CPU. Check the QEMU -smp option." :
        !smp_trampoline ?
            "The SMP AP startup trampoline was not prepared." :
        !smp_cr3 ?
            "The SMP startup CR3 was not reachable from the AP trampoline." :
        !smp_ipi ?
            "The Local APIC IPI path was unavailable for SMP startup." :
        !smp_init_ipi ?
            "The SMP INIT IPI was not sent to application processors." :
        !smp_startup_ipi ?
            "The SMP STARTUP IPI was not sent to application processors." :
        !smp_aps_started ?
            "Application processors did not enter the kernel AP loop." :
        !smp_complete ?
            "The SMP module did not emit its completion marker." :
        !qemu_debug_colour ?
            "The QEMU debug log did not include ANSI colour sequences for status lines." :
        !kernel_console ?
            "The kernel console did not initialize." :
        !screen_scrollback ?
            "The kernel screen scrollback buffer was not initialized." :
        !screen_coloured_cells ?
            "The kernel screen did not preserve coloured scrollback cells." :
        !screen_scroll_lines ?
            "The kernel screen line scrolling proof did not pass." :
        !screen_page_scroll ?
            "The kernel screen page scrolling proof did not pass." :
        !screen_bottom ?
            "The kernel screen did not return to the live bottom view." :
        !screen_scrolling ?
            "Kernel screen scrolling did not complete." :
        !screen_back_buffer ?
            "The kernel screen back buffer was not allocated." :
        !screen_renders_back ?
            "The kernel screen did not render into the back buffer first." :
        !screen_present ?
            "The kernel screen did not present completed frames to visible output." :
        !screen_double_buffer ?
            "Kernel screen double buffering did not complete." :
        !virtual_memory_started ?
            "The kernel stopped before virtual-memory initialization." :
        !virtual_memory_required_mapped ?
            "The kernel stopped while building required virtual-memory mappings." :
        !virtual_memory_switching_cr3 ?
            "The kernel stopped after mapping required ranges and before attempting the CR3 switch." :
        !virtual_memory_switched_cr3 ?
            "The kernel stopped during the CR3 switch to the kernel-owned PML4." :
        !system_halted ?
            "The kernel passed the CR3 switch but did not reach its halt path." :
            "The kernel halted but did not request QEMU debug-exit success.");

    fprintf(file, "Paths:\n");
    fprintf(file, "  QEMU: %s\n", qemu_path);
    fprintf(file, "  OVMF Windows: %s\n", ovmf_windows);
    fprintf(file, "  OVMF QEMU: %s\n", ovmf_qemu);
    fprintf(file, "  FAT32 disk image: %s\n", disk_windows);
    fprintf(file, "  Debug log: %s\n", debug_log);
    fprintf(file, "  Boot report: %s\n\n", report_path);

    fprintf(file, "Command:\n%s\n\n", command);
    fprintf(file, "Captured debug output:\n%s", debug_text[0] == 0 ? "[empty]\n" : debug_text);
    fclose(file);
}

int OrynCommandDoctor(const char* executable_path)
{
    (void)executable_path;
    OrynLogStep("Checking Oryn WSL development tools.");

    int ok = 1;
    PrintToolStatus("clang", "clang", &ok);
    PrintToolStatus("ld.lld", "ld.lld", &ok);
    PrintToolStatus("lld-link", "lld-link", &ok);
    PrintToolStatus("llvm-objcopy", "llvm-objcopy", &ok);
    PrintToolStatus("wslpath", "wslpath", &ok);
    PrintWindowsQemuStatus(&ok);

    char ovmf[ORYN_MAX_PATH];
    if (OrynFindOvmf(ovmf, sizeof(ovmf)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "OVMF found: %s", ovmf);
        OrynLogOk(message);
    }
    else
    {
        OrynLogFail("OVMF was not found. Expected Windows QEMU OVMF under C:/Program Files/qemu/share, WSL ovmf, or ORYN_OVMF_PATH.");
        ok = 0;
    }

    if (ok)
    {
        OrynLogOk("Doctor checks passed.");
        return 0;
    }

    OrynLogFail("Doctor checks failed.");
    return 1;
}

int OrynCommandBuild(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    OrynLogKeyValue("Project", project.name);
    OrynLogKeyValue("Target", project.target);
    OrynLogKeyValue("Toolchain", project.toolchain);

    return OrynBuildKernel(&project) ? 0 : 1;
}

int OrynCommandImage(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (!OrynBuildKernel(&project))
    {
        return 1;
    }

    return OrynBuildImage(&project) ? 0 : 1;
}

int OrynCommandRun(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (!OrynBuildKernel(&project))
    {
        return 1;
    }

    char run_image[ORYN_MAX_PATH];
    BuildProjectImagePath(&project, run_image, sizeof(run_image));
    if (ProjectBoolEnabled(project.run_format_vm, 1) || !OrynFileExists(run_image))
    {
        if (!OrynBuildImage(&project))
        {
            return 1;
        }
    }
    else
    {
        OrynLogInfo("VMSettings FormatVM=no; reusing existing VM disk image.");
    }

    return OrynRunQemu(&project) ? 0 : 1;
}

int OrynCommandClean(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    return OrynCleanProject(&project) ? 0 : 1;
}

int OrynRunQemu(const OrynProject* project)
{
    char qemu_path[ORYN_MAX_PATH];
    if (!OrynFindWindowsQemu(qemu_path, sizeof(qemu_path)))
    {
        OrynLogFail("Windows QEMU could not be resolved from WSL.");
        OrynLogWarn("Expected default Windows QEMU path: /mnt/c/Program Files/qemu/qemu-system-x86_64.exe");
        return 0;
    }

    if (project->ovmf_path[0] == 0)
    {
        OrynLogFail("OVMF path could not be resolved.");
        return 0;
    }

    char ovmf_windows[ORYN_MAX_PATH];
    char ovmf_qemu[ORYN_MAX_PATH];
    char disk_image[ORYN_MAX_PATH];
    char stage_root[ORYN_MAX_PATH];
    char stage_disk_image[ORYN_MAX_PATH];
    char stage_debug_log[ORYN_MAX_PATH];
    char disk_windows[ORYN_MAX_PATH];
    char disk_qemu[ORYN_MAX_PATH];
    char stage_debug_windows[ORYN_MAX_PATH];
    char stage_debug_qemu[ORYN_MAX_PATH];

    BuildProjectImagePath(project, disk_image, sizeof(disk_image));
    if (!OrynFileExists(disk_image))
    {
        OrynLogFail("FAT32 disk image was not found. Run image first.");
        return 0;
    }

    if (!OrynResolveWindowsStageRoot(stage_root, sizeof(stage_root)))
    {
        OrynLogFail("Could not create a Windows-local QEMU staging folder.");
        OrynLogWarn("Set ORYN_QEMU_STAGE_DIR to a Windows-local folder if needed.");
        return 0;
    }

    OrynMakeStageFilePath(stage_disk_image, sizeof(stage_disk_image), stage_root, project->name, ".img");
    OrynMakeStageFilePath(stage_debug_log, sizeof(stage_debug_log), stage_root, project->name, "-Debug.log");
    remove(stage_disk_image);
    remove(stage_debug_log);

    if (!OrynCopyFile(disk_image, stage_disk_image))
    {
        OrynLogFail("Could not copy FAT32 disk image to Windows-local QEMU staging folder.");
        return 0;
    }

    if (!OrynConvertWslPathToWindows(project->ovmf_path, ovmf_windows, sizeof(ovmf_windows)) ||
        !OrynConvertWslPathToWindowsQemu(project->ovmf_path, ovmf_qemu, sizeof(ovmf_qemu)) ||
        !OrynConvertWslPathToWindows(stage_disk_image, disk_windows, sizeof(disk_windows)) ||
        !OrynConvertWslPathToWindowsQemu(stage_disk_image, disk_qemu, sizeof(disk_qemu)) ||
        !OrynConvertWslPathToWindows(stage_debug_log, stage_debug_windows, sizeof(stage_debug_windows)) ||
        !OrynConvertWslPathToWindowsQemu(stage_debug_log, stage_debug_qemu, sizeof(stage_debug_qemu)))
    {
        OrynLogFail("Could not convert one or more Windows-local staging paths for QEMU.");
        return 0;
    }

    OrynLogStep("Launching Windows QEMU from WSL.");
    OrynLogKeyValue("QEMU", qemu_path);
    OrynLogKeyValue("OVMF", ovmf_windows);
    OrynLogKeyValue("OVMF QEMU path", ovmf_qemu);
    OrynLogKeyValue("Firmware mode", "pflash");
    OrynLogKeyValue("FAT32 image source", disk_image);
    OrynLogKeyValue("FAT32 image staged", disk_windows);

    const char* display_mode = ResolveQemuDisplayMode(project);
    if (!IsSafeQemuDisplayMode(display_mode))
    {
        OrynLogFail("Project [Run] Display value contains unsupported characters.");
        OrynLogWarn("Use Display=none for headless or Display=sdl for a QEMU window.");
        return 0;
    }

    if (!TextEqualsIgnoreCaseCommand(project->run_vm, "QEMU"))
    {
        OrynLogFail("Only VM=QEMU is currently supported by Oryn WSL run.");
        return 0;
    }

    if (!IsSafeQemuArgumentValue(project->run_memory) ||
        !IsSafeQemuArgumentValue(project->run_cpu) ||
        !IsSafeQemuArgumentValue(project->run_smp))
    {
        OrynLogFail("VMSettings Memory, CPU, or SMP contains unsupported characters.");
        return 0;
    }

    if (!TextEqualsIgnoreCaseCommand(project->run_disk_format, "raw"))
    {
        OrynLogFail("Only DiskFormat=raw is currently supported by the built-in FAT32 image writer.");
        return 0;
    }

    if (!TextEqualsIgnoreCaseCommand(project->run_storage_interface, "ide"))
    {
        OrynLogFail("Only StorageInterface=ide is currently supported by the UEFI FAT32 image runner.");
        return 0;
    }

    int vm_pic = ProjectBoolEnabled(project->run_pic, 1);
    int vm_apic = ProjectBoolEnabled(project->run_apic, 1);
    int vm_apic2 = ProjectBoolEnabled(project->run_apic2, 1) && vm_apic;
    int vm_hpet = ProjectBoolEnabled(project->run_hpet, 1);
    unsigned int vm_smp_count = ProjectCpuCount(project);
    const char* qemu_cpu_model = ResolveQemuCpuModel(project->run_cpu);
    if (QemuCpuWasTranslated(project->run_cpu, qemu_cpu_model))
    {
        OrynLogWarn("CPU=host/native requires KVM/HVF/WHX acceleration; using CPU=max for Windows QEMU from WSL.");
    }

    char smp_text[32];
    snprintf(smp_text, sizeof(smp_text), "%u", vm_smp_count);
    OrynLogKeyValue("Display", display_mode);
    OrynLogKeyValue("SMP CPUs", smp_text);
    OrynLogKeyValue("Memory", project->run_memory);
    OrynLogKeyValue("CPU requested", project->run_cpu);
    OrynLogKeyValue("CPU resolved", qemu_cpu_model);
    OrynLogKeyValue("PIC", OnOffText(vm_pic));
    OrynLogKeyValue("APIC", OnOffText(vm_apic));
    OrynLogKeyValue("APIC2/x2APIC", OnOffText(vm_apic2));
    OrynLogKeyValue("HPET", OnOffText(vm_hpet));
    OrynLogKeyValue("Disk format", project->run_disk_format);
    OrynLogKeyValue("Storage interface", project->run_storage_interface);

    char debug_log[ORYN_MAX_PATH];
    char boot_report[ORYN_MAX_PATH];
    OrynJoinPath(debug_log, sizeof(debug_log), project->output_dir, "Debug.log");
    OrynJoinPath(boot_report, sizeof(boot_report), project->output_dir, "BootReport.txt");
    remove(debug_log);
    remove(boot_report);

    OrynLogKeyValue("Serial", "live WSL terminal stdio");
    OrynLogKeyValue("Debug log", debug_log);
    OrynLogKeyValue("Debug log staged", stage_debug_windows);
    OrynLogKeyValue("Boot report", boot_report);

    char firmware_argument[ORYN_MAX_PATH * 2];
    char drive_argument[ORYN_MAX_PATH * 2];
    char debug_argument[ORYN_MAX_PATH * 2];
    snprintf(firmware_argument, sizeof(firmware_argument), "if=pflash,format=raw,readonly=on,file=%s", ovmf_qemu);
    snprintf(drive_argument, sizeof(drive_argument), "format=raw,if=ide,file=%s", disk_qemu);
    snprintf(debug_argument, sizeof(debug_argument), "file:%s", stage_debug_qemu);

    char qemu_quoted[ORYN_MAX_PATH + 16];
    char firmware_quoted[(ORYN_MAX_PATH * 2) + 16];
    char drive_quoted[(ORYN_MAX_PATH * 2) + 16];
    char debug_quoted[(ORYN_MAX_PATH * 2) + 16];
    OrynShellQuote(qemu_quoted, sizeof(qemu_quoted), qemu_path);
    OrynShellQuote(firmware_quoted, sizeof(firmware_quoted), firmware_argument);
    OrynShellQuote(drive_quoted, sizeof(drive_quoted), drive_argument);
    OrynShellQuote(debug_quoted, sizeof(debug_quoted), debug_argument);

    char machine_argument[128];
    char cpu_argument[256];
    snprintf(machine_argument, sizeof(machine_argument), "q35,hpet=%s", vm_hpet ? "on" : "off");
    snprintf(cpu_argument, sizeof(cpu_argument), "%s,%sapic,%sx2apic",
        qemu_cpu_model,
        vm_apic ? "+" : "-",
        vm_apic2 ? "+" : "-");

    char command[ORYN_MAX_PATH * 8];
    snprintf(command, sizeof(command),
        "%s -machine %s -cpu %s -smp %u -m %s -drive %s -no-reboot -display %s "
        "-monitor none -serial stdio -debugcon %s -global isa-debugcon.iobase=0xe9 "
        "-device isa-debug-exit,iobase=0xf4,iosize=0x04 -drive %s",
        qemu_quoted,
        machine_argument,
        cpu_argument,
        vm_smp_count,
        project->run_memory,
        firmware_quoted,
        display_mode,
        debug_quoted,
        drive_quoted);

    int exit_code = -1;
    int command_ok = RunQemuAndGetExitCode(command, &exit_code);

    if (OrynFileExists(stage_debug_log))
    {
        OrynCopyFile(stage_debug_log, debug_log);
    }

    size_t debug_text_capacity = 1024U * 1024U;
    char* debug_text = (char*)malloc(debug_text_capacity);
    if (debug_text == 0)
    {
        OrynLogFail("Could not allocate debug log read buffer.");
        return 0;
    }

    ReadFileText(debug_log, debug_text, debug_text_capacity);
    WriteBootReport(project, boot_report, qemu_path, ovmf_windows, ovmf_qemu, disk_windows,
        debug_log, qemu_cpu_model, command, debug_text, exit_code, command_ok);

    PrintFileIfPresent("QEMU debug output", debug_log);
    PrintFileIfPresent("Boot report", boot_report);

    int boot_pass = (TextContains(debug_text, "[KERNEL] PASS: Kernel entered successfully") ||
        TextContains(debug_text, "[KERNEL] Oryn Kernel-5 entered.")) &&
        TextContains(debug_text, "[KERNEL] PASS: BootInfo received") &&
        TextContains(debug_text, "[KERNEL] PASS: GDT installed.") &&
        TextContains(debug_text, "[KERNEL] GDT entries: 7") &&
        TextContains(debug_text, "[KERNEL] PASS: TSS loaded.") &&
        TextContains(debug_text, "[KERNEL] PASS: IDT installed.") &&
        TextContains(debug_text, "[KERNEL] PASS: Interrupt dispatcher initialized.") &&
        (ProjectBoolEnabled(project->run_pic, 1) ? TextContains(debug_text, "[KERNEL] PASS: PIC IRQ0 interrupt fired through IDT dispatch.") : TextContains(debug_text, "[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.")) &&
        ((ProjectBoolEnabled(project->run_apic2, 1) && ProjectBoolEnabled(project->run_apic, 1)) ? TextContains(debug_text, "[KERNEL] PASS: CPU APIC2/x2APIC feature present.") : 1) &&
        (ProjectBoolEnabled(project->run_pic, 1) ? TextContains(debug_text, "[KERNEL] PASS: PIC masked/disabled for APIC handoff.") : 1) &&
        ((ProjectBoolEnabled(project->run_apic2, 1) && ProjectBoolEnabled(project->run_apic, 1)) ? TextContains(debug_text, "[KERNEL] PASS: APIC2/x2APIC mode enabled.") : 1) &&
        (ProjectBoolEnabled(project->run_apic, 1) ? TextContains(debug_text, "[KERNEL] PASS: APIC timer counter moved in masked probe.") : TextContains(debug_text, "[KERNEL] INFO: APIC proofs skipped by VMSettings.")) &&
        (ProjectBoolEnabled(project->run_hpet, 1) ? TextContains(debug_text, "[KERNEL] PASS: HPET counter advanced in probe.") : TextContains(debug_text, "[KERNEL] INFO: HPET proof skipped by VMSettings.")) &&
        (ProjectBoolEnabled(project->run_apic, 1) ? TextContains(debug_text, "[KERNEL] PASS: APIC timer interrupt fired through IDT dispatch.") : 1) &&
        (TextContains(debug_text, "[KERNEL] PASS: Interrupts work from PIC upward through APIC/APIC2.") ||
            TextContains(debug_text, "[KERNEL] PASS: VMSettings interrupt/timer profile applied.")) &&
        TextContains(debug_text, "[KERNEL] PASS: SysCalls use Get/Set/Event message packets.") &&
        TextContains(debug_text, "[KERNEL] PASS: Platform syscalls translate into Get/Set/Event packets.") &&
        TextContains(debug_text, "[KERNEL] PASS: SysCall header counts listed.") &&
        TextContains(debug_text, "[KERNEL] PASS: Unknown syscall debug logging path executed.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI ACPI MCFG table discovered.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCIe ECAM descriptor captured.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI config mechanism #1 responded.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI bus/device/function scan completed.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI devices discovered.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI class-code decoding ready.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI device output uses English labels.") &&
        TextContains(debug_text, "[KERNEL] PASS: PCI Discovery complete.") &&
        ((ProjectCpuCount(project) > 1U && ProjectBoolEnabled(project->run_apic, 1)) ?
            (TextContains(debug_text, "[KERNEL] PASS: SMP multi-core CPU topology discovered.") &&
             TextContains(debug_text, "[KERNEL] PASS: SMP application processors entered kernel AP loop.") &&
             TextContains(debug_text, "[KERNEL] PASS: Multi-Core processing initialized.")) :
            TextContains(debug_text, "[KERNEL] INFO: SMP AP startup skipped by VMSettings.")) &&
        TextContains(debug_text, "\033[32m[KERNEL] PASS") &&
        TextContains(debug_text, "\033[0m") &&
        !TextContains(debug_text, "[KERNEL] EXCEPTION:") &&
        TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success") && command_ok;

    if (boot_pass)
    {
        free(debug_text);
        OrynLogOk("Boot proof passed. Kernel output was captured and QEMU exited cleanly.");
        return 1;
    }

    free(debug_text);
    OrynLogFail("Boot proof failed. See Output/BootReport.txt and Output/Debug.log.");
    return 0;
}

int OrynCleanProject(const OrynProject* project)
{
    OrynLogStep("Cleaning project build/output folders.");
    OrynRemoveDirectoryRecursive(project->build_dir);
    OrynRemoveDirectoryRecursive(project->output_dir);
    OrynLogOk("Project cleaned.");
    return 1;
}
