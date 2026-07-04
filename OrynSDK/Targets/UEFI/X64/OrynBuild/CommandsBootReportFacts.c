#include "TargetBuildInternal.h"

static int KernelOkFact(const char* debug_text, const char* message)
{
    return TextContains(debug_text, message);
}

void CollectBootReportFacts(
    const OrynProject* project,
    const char* debug_text,
    int exit_code,
    int command_ok,
    OrynBootReportFacts* facts)
{
    facts->loader_started = TextContains(debug_text, "[BOOT] Stage 01");
    facts->kernel_loaded = TextContains(debug_text, "[BOOT] Kernel loaded physical base");
    facts->entry_printed = TextContains(debug_text, "[BOOT] Kernel virtual entry address");
    facts->virtual_map_prepared = TextContains(debug_text, "[BOOT] PASS: Temporary higher-half/chosen virtual map prepared");
    facts->virtual_map_active = TextContains(debug_text, "[BOOT] PASS: Temporary higher-half/chosen virtual map activated");
    facts->boot_services_exited = TextContains(debug_text, "[BOOT] Stage 07: ExitBootServices succeeded");
    facts->bootinfo_created = TextContains(debug_text, "[BOOT] BootInfo allocated at");
    facts->boot_config_prepared = TextContains(debug_text, "[BOOT] PASS: Boot configuration block prepared");
    facts->memory_map_requested = !TextContains(debug_text, "[BOOT] BootInfo selection memory map: disabled");
    facts->bootinfo_memory_map = !facts->memory_map_requested || TextContains(debug_text, "[BOOT] BootInfo memory map entries");
    facts->kernel_jump = TextContains(debug_text, "[BOOT] Stage 08: Jumping to kernel entry");
    facts->kernel_entered = KernelOkFact(debug_text, "Kernel entered successfully") ||
        TextContains(debug_text, "[KERNEL] Oryn Kernel-5 entered.");
    facts->serial_ok = KernelOkFact(debug_text, "Serial/debug output path is working");
    facts->bootinfo_received = KernelOkFact(debug_text, "BootInfo received");
    facts->kernel_boot_config = KernelOkFact(debug_text, "Boot configuration block received");
    facts->kernel_command_line = KernelOkFact(debug_text, "Kernel command line received");
    facts->gdt_installing = TextContains(debug_text, "[KERNEL] GDT: installing");
    facts->gdt_installed = KernelOkFact(debug_text, "GDT installed.");
    facts->gdt_entries = TextContains(debug_text, "[KERNEL] GDT entries: 7");
    facts->tss_loaded = KernelOkFact(debug_text, "TSS loaded.");
    facts->idt_installing = TextContains(debug_text, "[KERNEL] IDT: installing");
    facts->idt_installed = KernelOkFact(debug_text, "IDT installed.");
    facts->idt_entries = TextContains(debug_text, "[KERNEL] IDT entries: 256");
    facts->interrupt_dispatcher = KernelOkFact(debug_text, "Interrupt dispatcher initialized.");
    facts->interrupt_handlers = KernelOkFact(debug_text, "Interrupt handler table ready for 256 vectors.");
    facts->interrupt_controlled = KernelOkFact(debug_text, "CPU interrupts are currently disabled for controlled boot.");
    facts->cpu_exception = TextContains(debug_text, "[KERNEL] EXCEPTION:");
    facts->cpu_local_apic = KernelOkFact(debug_text, "CPU local APIC feature present.");
    facts->cpu_apic2 = KernelOkFact(debug_text, "CPU APIC2/x2APIC feature present.");
    facts->pic_initialized = KernelOkFact(debug_text, "PIC initialized.");
    facts->pic_remapped = KernelOkFact(debug_text, "PIC remapped to vectors 0x20-0x2F.");
    facts->pic_masked = KernelOkFact(debug_text, "PIC masked/disabled for APIC handoff.");
    facts->apic_available = KernelOkFact(debug_text, "APIC CPU feature available.");
    facts->apic2_enabled = KernelOkFact(debug_text, "APIC2/x2APIC mode enabled.");
    facts->local_apic_enabled = KernelOkFact(debug_text, "Local APIC software enable bit set.");
    facts->apic_timer_probe = KernelOkFact(debug_text, "APIC timer counter moved in masked probe.");
    facts->hpet_rsdp = KernelOkFact(debug_text, "HPET ACPI RSDP input present.");
    facts->hpet_checksum = KernelOkFact(debug_text, "HPET ACPI checksum validation passed.");
    facts->hpet_table = KernelOkFact(debug_text, "HPET ACPI table discovered.");
    facts->hpet_enabled = KernelOkFact(debug_text, "HPET main counter enabled.");
    facts->hpet_counter = KernelOkFact(debug_text, "HPET counter advanced in probe.");
    facts->pic_irq0 = KernelOkFact(debug_text, "PIC IRQ0 interrupt fired through IDT dispatch.");
    facts->pic_irq0_count = KernelOkFact(debug_text, "PIC IRQ0 handler counter updated.");
    facts->pic_eoi = KernelOkFact(debug_text, "PIC EOI path executed.");
    facts->apic_timer_interrupt = KernelOkFact(debug_text, "APIC timer interrupt fired through IDT dispatch.");
    facts->apic_irq_counters = KernelOkFact(debug_text, "APIC timer IRQ counter updated by interrupt dispatch.");
    facts->apic_eoi = KernelOkFact(debug_text, "APIC EOI path executed.");
    facts->interrupt_chain = KernelOkFact(debug_text, "Interrupts work from PIC upward through APIC/APIC2.");
    facts->syscall_core = KernelOkFact(debug_text, "SysCall core initialized.");
    facts->syscall_packet = KernelOkFact(debug_text, "SysCall message packet ABI ready.");
    facts->syscall_get = KernelOkFact(debug_text, "SysCallGet packet handled.");
    facts->syscall_set = KernelOkFact(debug_text, "SysCallSet packet handled.");
    facts->syscall_event = KernelOkFact(debug_text, "SysCallEvent packet handled.");
    facts->syscall_unknown = KernelOkFact(debug_text, "Unknown syscall debug logging path executed.");
    facts->linux_translator = KernelOkFact(debug_text, "LinuxSysCall translator registered.");
    facts->ms_translator = KernelOkFact(debug_text, "MSSysCall translator registered.");
    facts->linux_vector = KernelOkFact(debug_text, "Linux syscall vector 0x80 received and translated.");
    facts->ms_vector = KernelOkFact(debug_text, "MS syscall vector 0x81 received and translated.");
    facts->unknown_linux = KernelOkFact(debug_text, "Unknown Linux syscall was reported as debug.");
    facts->unknown_ms = KernelOkFact(debug_text, "Unknown MS syscall was reported as debug.");
    facts->syscall_three = KernelOkFact(debug_text, "SysCalls use Get/Set/Event message packets.");
    facts->platform_syscalls = KernelOkFact(debug_text, "Platform syscalls translate into Get/Set/Event packets.");
    facts->syscall_counts = KernelOkFact(debug_text, "SysCall header counts listed.");
    facts->pci_started = TextContains(debug_text, "[KERNEL] PCI: discovery starting.");
    facts->pci_rsdp = KernelOkFact(debug_text, "PCI ACPI RSDP input present.");
    facts->pci_checksum = KernelOkFact(debug_text, "PCI ACPI checksum validation passed.");
    facts->pci_mcfg = KernelOkFact(debug_text, "PCI ACPI MCFG table discovered.");
    facts->pci_ecam = KernelOkFact(debug_text, "PCIe ECAM descriptor captured.");
    facts->pci_config = KernelOkFact(debug_text, "PCI config mechanism #1 responded.");
    facts->pci_scan = KernelOkFact(debug_text, "PCI bus/device/function scan completed.");
    facts->pci_devices = KernelOkFact(debug_text, "PCI devices discovered.");
    facts->pci_class = KernelOkFact(debug_text, "PCI class-code decoding ready.");
    facts->pci_complete = KernelOkFact(debug_text, "PCI Discovery complete.");
    facts->pci_english = KernelOkFact(debug_text, "PCI device output uses English labels.");
    facts->smp_started = TextContains(debug_text, "[KERNEL] SMP: multi-core processing discovery starting.");
    facts->smp_started_early = TextContains(debug_text, "[KERNEL] SMP: starting early after APIC/APIC2 enable.");
    facts->smp_rsdp = KernelOkFact(debug_text, "SMP ACPI RSDP input present.");
    facts->smp_checksum = KernelOkFact(debug_text, "SMP ACPI checksum validation passed.");
    facts->smp_madt = KernelOkFact(debug_text, "SMP ACPI MADT table discovered.");
    facts->smp_cached = KernelOkFact(debug_text, "SMP ACPI topology cached before virtual memory switch.");
    facts->smp_topology = KernelOkFact(debug_text, "SMP multi-core CPU topology discovered.");
    facts->smp_early_stage = KernelOkFact(debug_text, "SMP AP startup moved before PCI/HPET/console/memory proof.");
    facts->smp_trampoline = KernelOkFact(debug_text, "SMP AP startup trampoline prepared below 1MB.");
    facts->smp_cr3 = KernelOkFact(debug_text, "SMP startup CR3 is reachable by the AP trampoline.");
    facts->smp_ipi = KernelOkFact(debug_text, "SMP Local APIC IPI path ready.");
    facts->smp_init_ipi = KernelOkFact(debug_text, "SMP INIT IPI sent to application processors.");
    facts->smp_startup_ipi = KernelOkFact(debug_text, "SMP STARTUP IPI sent to application processors.");
    facts->smp_aps_started = KernelOkFact(debug_text, "SMP application processors entered kernel AP loop.");
    facts->smp_early_complete = KernelOkFact(debug_text, "SMP AP startup completed before PCI/HPET/console/memory proof.");
    facts->smp_complete = KernelOkFact(debug_text, "Multi-Core processing initialized.");
    facts->qemu_debug_colour = (TextContains(debug_text, "\033[32m[KERNEL] OK") ||
        TextContains(debug_text, "\033[36m[KERNEL] OK") ||
        TextContains(debug_text, "\033[32m[KERNEL] PASS")) && TextContains(debug_text, "\033[0m");
    facts->kernel_console = KernelOkFact(debug_text, "Kernel console initialized.");
    facts->screen_scrollback = KernelOkFact(debug_text, "Kernel screen scrollback buffer initialized.");
    facts->screen_coloured_cells = KernelOkFact(debug_text, "Kernel screen scrollback stores coloured cells.");
    facts->screen_scroll_lines = KernelOkFact(debug_text, "Kernel screen scroll up/down works.");
    facts->screen_page_scroll = KernelOkFact(debug_text, "Kernel screen page up/down works.");
    facts->screen_bottom = KernelOkFact(debug_text, "Kernel screen scroll-to-bottom works.");
    facts->screen_stable_proof = KernelOkFact(debug_text, "Kernel screen scroll proof keeps visible output stable.") ||
        KernelOkFact(debug_text, "Kernel screen proof keeps visible output stable.");
    facts->screen_scrolling = KernelOkFact(debug_text, "Kernel screen scrolling implemented.");
    facts->screen_back_buffer = KernelOkFact(debug_text, "Kernel screen back buffer allocated.");
    facts->screen_renders_back = KernelOkFact(debug_text, "Kernel screen renders into back buffer first.");
    facts->screen_present = KernelOkFact(debug_text, "Kernel screen presents completed frame to visible output.");
    facts->screen_atomic_present = KernelOkFact(debug_text, "Kernel screen visible presents are atomic.");
    facts->screen_deferred_flip = KernelOkFact(debug_text, "Kernel screen defers visible flip while line is being written.");
    facts->screen_line_flip = KernelOkFact(debug_text, "Kernel screen flips after completed line.");
    facts->screen_dirty_line = KernelOkFact(debug_text, "Kernel screen presents dirty completed line only.");
    facts->screen_fast_scroll = KernelOkFact(debug_text, "Kernel screen uses fast scroll path after visible area is full.");
    facts->screen_refresh_optimized = KernelOkFact(debug_text, "Kernel screen refresh is line/scroll optimized.");
    facts->screen_line_buffered = KernelOkFact(debug_text, "Kernel screen line-buffered double buffering implemented.");
    facts->screen_double_buffer = KernelOkFact(debug_text, "Kernel screen double buffering implemented.");
    facts->physical_capacity = KernelOkFact(debug_text, "Physical allocator tracking capacity is sufficient.");
    facts->keyboard_initialized = KernelOkFact(debug_text, "Keyboard interrupt scrolling initialized.");
    facts->keyboard_irq1 = KernelOkFact(debug_text, "PS/2 keyboard IRQ1 handler registered.");
    facts->keyboard_interrupts = KernelOkFact(debug_text, "Keyboard scrolling uses IRQ1 interrupts.");
    facts->keyboard_pic_unmasked = KernelOkFact(debug_text, "PIC IRQ1 unmasked for keyboard input.");
    facts->keyboard_decoder = KernelOkFact(debug_text, "Keyboard arrow and page key decoder ready.");
    facts->keyboard_make_break = KernelOkFact(debug_text, "Keyboard scroll keys use make/break state tracking.");
    facts->keyboard_release_stop = KernelOkFact(debug_text, "Keyboard release scan codes stop scrolling immediately.");
    facts->keyboard_line = KernelOkFact(debug_text, "Keyboard Up/Down scroll one line while held.");
    facts->keyboard_page = KernelOkFact(debug_text, "Keyboard PgUp/PgDn scroll one page while held.");
    facts->keyboard_stops_on_release = KernelOkFact(debug_text, "Keyboard scrolling stops when key is released.");
    facts->interactive_interrupts = KernelOkFact(debug_text, "Interactive halt loop leaves interrupts enabled for keyboard scrolling.");
    facts->report_display_mode = ResolveQemuDisplayMode(project);
    facts->interactive_display = IsInteractiveDisplayMode(facts->report_display_mode);
    facts->headless_timeout_seconds = ResolveQemuHeadlessTimeoutSeconds();
    facts->qemu_timeout = (!facts->interactive_display) && (exit_code == 124 || exit_code == 137);
    facts->qemu_no_output_timeout = facts->qemu_timeout && !facts->loader_started && DebugTextIsEmpty(debug_text);
    facts->qemu_preboot_failure = (!facts->qemu_no_output_timeout) && (!command_ok) && !facts->loader_started && DebugTextIsEmpty(debug_text);
    facts->interactive_hold = KernelOkFact(debug_text, "Interactive QEMU display mode keeps VM open for scroll testing.");
    facts->qemu_completion_ok = command_ok && (facts->interactive_display || exit_code == 0 || exit_code == 33);
    facts->want_pic = ProjectBoolEnabled(project->run_pic, 1);
    facts->want_apic = ProjectBoolEnabled(project->run_apic, 1);
    facts->want_apic2 = ProjectBoolEnabled(project->run_apic2, 1);
    facts->want_local_apic = facts->want_apic || facts->want_apic2;
    facts->want_hpet = ProjectBoolEnabled(project->run_hpet, 1);
    facts->wanted_cpu_count = ProjectCpuCount(project);
    facts->want_smp = (facts->wanted_cpu_count > 1U) && facts->want_local_apic;
    facts->pic_skipped = TextContains(debug_text, "[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.");
    facts->apic_skipped = TextContains(debug_text, "[KERNEL] INFO: APIC proofs skipped by VMSettings.");
    facts->apic2_skipped = TextContains(debug_text, "[KERNEL] INFO: APIC2/x2APIC disabled by VMSettings.");
    facts->hpet_skipped = TextContains(debug_text, "[KERNEL] INFO: HPET proof skipped by VMSettings.");
    facts->smp_skipped = TextContains(debug_text, "[KERNEL] INFO: SMP AP startup skipped by VMSettings.") ||
        TextContains(debug_text, "[KERNEL] INFO: SMP discovery skipped by VMSettings.");
    facts->cpu_local_apic_ok = facts->want_local_apic ? facts->cpu_local_apic : 1;
    facts->pic_ok = facts->want_pic ? (facts->pic_initialized && facts->pic_remapped && facts->pic_irq0 && facts->pic_irq0_count && facts->pic_eoi && facts->pic_masked) : facts->pic_skipped;
    facts->apic_ok = facts->want_local_apic ? (facts->apic_available && facts->local_apic_enabled && facts->apic_timer_probe && facts->apic_timer_interrupt && facts->apic_irq_counters && facts->apic_eoi) : facts->apic_skipped;
    facts->apic2_ok = facts->want_apic2 ? (facts->cpu_apic2 && facts->apic2_enabled) : (facts->apic2_skipped || !facts->want_local_apic);
    facts->hpet_ok = facts->want_hpet ? (facts->hpet_rsdp && facts->hpet_checksum && facts->hpet_table && facts->hpet_enabled && facts->hpet_counter) : facts->hpet_skipped;
    facts->interrupt_chain_ok = KernelOkFact(debug_text, "Interrupts work from PIC upward through APIC/APIC2.") ||
        KernelOkFact(debug_text, "VMSettings interrupt/timer profile applied.");
    facts->smp_ok = facts->want_smp ? (facts->smp_started_early && facts->smp_started && facts->smp_rsdp && facts->smp_checksum && facts->smp_madt && facts->smp_cached &&
        facts->smp_topology && facts->smp_early_stage && facts->smp_trampoline && facts->smp_cr3 && facts->smp_ipi && facts->smp_init_ipi &&
        facts->smp_startup_ipi && facts->smp_aps_started && facts->smp_early_complete && facts->smp_complete) : facts->smp_skipped;
    facts->virtual_memory_started = TextContains(debug_text, "[KERNEL] Virtual memory: starting");
    facts->virtual_memory_required_mapped = TextContains(debug_text, "[KERNEL] Virtual memory: required ranges mapped");
    facts->virtual_memory_switching_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: switching CR3 to kernel-owned PML4");
    facts->virtual_memory_switched_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: CR3 switched to kernel-owned PML4");
    facts->virtual_memory_active = TextContains(debug_text, "[KERNEL] Virtual memory: active");
    facts->system_halted = TextContains(debug_text, "[KERNEL] System halted by Kernel-5");
    facts->debug_exit = TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success");
    facts->qemu_exit_or_hold = facts->interactive_display ? facts->interactive_hold : facts->debug_exit;

    int minimal_sdk_hello_world = facts->qemu_completion_ok && facts->loader_started &&
        facts->kernel_loaded && facts->entry_printed && facts->virtual_map_prepared && facts->virtual_map_active &&
        facts->bootinfo_created && facts->boot_config_prepared && facts->bootinfo_memory_map &&
        facts->boot_services_exited && facts->kernel_jump &&
        TextContains(debug_text, "Hello World") &&
        !TextContains(debug_text, "[KERNEL] FAIL:") &&
        !TextContains(debug_text, "[KERNEL] EXCEPTION:");

    facts->boot_pass = minimal_sdk_hello_world || (facts->qemu_completion_ok && facts->loader_started &&
        facts->kernel_loaded && facts->entry_printed && facts->virtual_map_prepared && facts->virtual_map_active &&
        facts->bootinfo_created && facts->boot_config_prepared && facts->bootinfo_memory_map && facts->boot_services_exited && facts->kernel_jump &&
        !TextContains(debug_text, "[KERNEL] FAIL:") && facts->kernel_entered && facts->serial_ok && facts->bootinfo_received && facts->kernel_boot_config && facts->kernel_command_line &&
        facts->gdt_installing && facts->gdt_installed && facts->gdt_entries && facts->tss_loaded &&
        facts->idt_installing && facts->idt_installed && facts->idt_entries &&
        facts->interrupt_dispatcher && facts->interrupt_handlers && facts->interrupt_controlled && !facts->cpu_exception &&
        facts->cpu_local_apic_ok && facts->pic_ok && facts->apic_ok && facts->apic2_ok && facts->hpet_ok &&
        facts->interrupt_chain_ok && facts->syscall_core &&
        facts->syscall_packet && facts->syscall_get && facts->syscall_set && facts->syscall_event && facts->syscall_unknown &&
        facts->linux_translator && facts->ms_translator && facts->linux_vector && facts->ms_vector &&
        facts->unknown_linux && facts->unknown_ms && facts->syscall_three && facts->platform_syscalls && facts->syscall_counts &&
        facts->pci_started && facts->pci_rsdp && facts->pci_checksum && facts->pci_mcfg && facts->pci_ecam && facts->pci_config &&
        facts->pci_scan && facts->pci_devices && facts->pci_class && facts->pci_complete && facts->pci_english &&
        facts->smp_ok && facts->qemu_debug_colour && facts->kernel_console && facts->screen_scrollback &&
        facts->screen_coloured_cells && facts->screen_scroll_lines && facts->screen_page_scroll &&
        facts->screen_bottom && facts->screen_stable_proof && facts->screen_scrolling && facts->screen_back_buffer && facts->screen_renders_back &&
        facts->screen_deferred_flip && facts->screen_line_flip && facts->screen_dirty_line && facts->screen_fast_scroll &&
        facts->screen_refresh_optimized && facts->screen_line_buffered && facts->screen_present && facts->screen_atomic_present && facts->screen_double_buffer &&
        facts->physical_capacity && facts->keyboard_initialized && facts->keyboard_irq1 && facts->keyboard_interrupts && facts->keyboard_pic_unmasked &&
        facts->keyboard_decoder && facts->keyboard_make_break && facts->keyboard_release_stop && facts->keyboard_line && facts->keyboard_page &&
        facts->keyboard_stops_on_release && (!facts->interactive_display || facts->interactive_interrupts) &&
        facts->virtual_memory_active && facts->qemu_exit_or_hold);
}
