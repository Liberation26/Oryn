static void BuildQemuStageProjectName(const OrynProject* project, char* output, size_t output_size)
{
    char output_base[ORYN_MAX_PATH];
    OrynGetBaseName(output_base, sizeof(output_base), project->output_dir);

    if (output_base[0] != 0 && strcmp(output_base, "Output") != 0)
    {
        snprintf(output, output_size, "%s-%s", project->name, output_base);
        return;
    }

    snprintf(output, output_size, "%s", project->name);
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
    int smp_started_early = TextContains(debug_text, "[KERNEL] SMP: starting early after APIC/APIC2 enable.");
    int smp_rsdp = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI RSDP input present.");
    int smp_checksum = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI checksum validation passed.");
    int smp_madt = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI MADT table discovered.");
    int smp_cached = TextContains(debug_text, "[KERNEL] PASS: SMP ACPI topology cached before virtual memory switch.");
    int smp_topology = TextContains(debug_text, "[KERNEL] PASS: SMP multi-core CPU topology discovered.");
    int smp_early_stage = TextContains(debug_text, "[KERNEL] PASS: SMP AP startup moved before PCI/HPET/console/memory proof.");
    int smp_trampoline = TextContains(debug_text, "[KERNEL] PASS: SMP AP startup trampoline prepared below 1MB.");
    int smp_cr3 = TextContains(debug_text, "[KERNEL] PASS: SMP startup CR3 is reachable by the AP trampoline.");
    int smp_ipi = TextContains(debug_text, "[KERNEL] PASS: SMP Local APIC IPI path ready.");
    int smp_init_ipi = TextContains(debug_text, "[KERNEL] PASS: SMP INIT IPI sent to application processors.");
    int smp_startup_ipi = TextContains(debug_text, "[KERNEL] PASS: SMP STARTUP IPI sent to application processors.");
    int smp_aps_started = TextContains(debug_text, "[KERNEL] PASS: SMP application processors entered kernel AP loop.");
    int smp_early_complete = TextContains(debug_text, "[KERNEL] PASS: SMP AP startup completed before PCI/HPET/console/memory proof.");
    int smp_complete = TextContains(debug_text, "[KERNEL] PASS: Multi-Core processing initialized.");
    int qemu_debug_colour = TextContains(debug_text, "\033[32m[KERNEL] PASS") &&
        TextContains(debug_text, "\033[0m");
    int kernel_console = TextContains(debug_text, "[KERNEL] PASS: Kernel console initialized.");
    int screen_scrollback = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scrollback buffer initialized.");
    int screen_coloured_cells = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scrollback stores coloured cells.");
    int screen_scroll_lines = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scroll up/down works.");
    int screen_page_scroll = TextContains(debug_text, "[KERNEL] PASS: Kernel screen page up/down works.");
    int screen_bottom = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scroll-to-bottom works.");
    int screen_stable_proof = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scroll proof keeps visible output stable.");
    int screen_scrolling = TextContains(debug_text, "[KERNEL] PASS: Kernel screen scrolling implemented.");
    int screen_back_buffer = TextContains(debug_text, "[KERNEL] PASS: Kernel screen back buffer allocated.");
    int screen_renders_back = TextContains(debug_text, "[KERNEL] PASS: Kernel screen renders into back buffer first.");
    int screen_present = TextContains(debug_text, "[KERNEL] PASS: Kernel screen presents completed frame to visible output.");
    int screen_atomic_present = TextContains(debug_text, "[KERNEL] PASS: Kernel screen visible presents are atomic.");
    int screen_deferred_flip = TextContains(debug_text, "[KERNEL] PASS: Kernel screen defers visible flip while line is being written.");
    int screen_line_flip = TextContains(debug_text, "[KERNEL] PASS: Kernel screen flips after completed line.");
    int screen_dirty_line = TextContains(debug_text, "[KERNEL] PASS: Kernel screen presents dirty completed line only.");
    int screen_fast_scroll = TextContains(debug_text, "[KERNEL] PASS: Kernel screen uses fast scroll path after visible area is full.");
    int screen_refresh_optimized = TextContains(debug_text, "[KERNEL] PASS: Kernel screen refresh is line/scroll optimized.");
    int screen_line_buffered = TextContains(debug_text, "[KERNEL] PASS: Kernel screen line-buffered double buffering implemented.");
    int screen_double_buffer = TextContains(debug_text, "[KERNEL] PASS: Kernel screen double buffering implemented.");
    int physical_capacity = TextContains(debug_text, "[KERNEL] PASS: Physical allocator tracking capacity is sufficient.");
    int keyboard_initialized = TextContains(debug_text, "[KERNEL] PASS: Keyboard interrupt scrolling initialized.");
    int keyboard_irq1 = TextContains(debug_text, "[KERNEL] PASS: PS/2 keyboard IRQ1 handler registered.");
    int keyboard_interrupts = TextContains(debug_text, "[KERNEL] PASS: Keyboard scrolling uses IRQ1 interrupts.");
    int keyboard_pic_unmasked = TextContains(debug_text, "[KERNEL] PASS: PIC IRQ1 unmasked for keyboard input.");
    int keyboard_decoder = TextContains(debug_text, "[KERNEL] PASS: Keyboard arrow and page key decoder ready.");
    int keyboard_make_break = TextContains(debug_text, "[KERNEL] PASS: Keyboard scroll keys use make/break state tracking.");
    int keyboard_release_stop = TextContains(debug_text, "[KERNEL] PASS: Keyboard release scan codes stop scrolling immediately.");
    int keyboard_line = TextContains(debug_text, "[KERNEL] PASS: Keyboard Up/Down scroll one line while held.");
    int keyboard_page = TextContains(debug_text, "[KERNEL] PASS: Keyboard PgUp/PgDn scroll one page while held.");
    int keyboard_stops_on_release = TextContains(debug_text, "[KERNEL] PASS: Keyboard scrolling stops when key is released.");
    int interactive_interrupts = TextContains(debug_text, "[KERNEL] PASS: Interactive halt loop leaves interrupts enabled for keyboard scrolling.");
    const char* report_display_mode = ResolveQemuDisplayMode(project);
    int interactive_display = IsInteractiveDisplayMode(report_display_mode);
    unsigned int headless_timeout_seconds = ResolveQemuHeadlessTimeoutSeconds();
    int qemu_timeout = (!interactive_display) && (exit_code == 124 || exit_code == 137);
    int qemu_no_output_timeout = qemu_timeout && !loader_started && DebugTextIsEmpty(debug_text);
    int qemu_preboot_failure = (!qemu_no_output_timeout) && (!command_ok) && !loader_started && DebugTextIsEmpty(debug_text);
    int interactive_hold = TextContains(debug_text, "[KERNEL] PASS: Interactive QEMU display mode keeps VM open for scroll testing.");
    int qemu_completion_ok = command_ok && (interactive_display || exit_code == 0 || exit_code == 33);

    int want_pic = ProjectBoolEnabled(project->run_pic, 1);
    int want_apic = ProjectBoolEnabled(project->run_apic, 1);
    int want_apic2 = ProjectBoolEnabled(project->run_apic2, 1);
    int want_local_apic = want_apic || want_apic2;
    int want_hpet = ProjectBoolEnabled(project->run_hpet, 1);
    unsigned int wanted_cpu_count = ProjectCpuCount(project);
    int want_smp = (wanted_cpu_count > 1U) && want_local_apic;
    int pic_skipped = TextContains(debug_text, "[KERNEL] INFO: PIC IRQ0 proof skipped by VMSettings.");
    int apic_skipped = TextContains(debug_text, "[KERNEL] INFO: APIC proofs skipped by VMSettings.");
    int apic2_skipped = TextContains(debug_text, "[KERNEL] INFO: APIC2/x2APIC disabled by VMSettings.");
    int hpet_skipped = TextContains(debug_text, "[KERNEL] INFO: HPET proof skipped by VMSettings.");
    int smp_skipped = TextContains(debug_text, "[KERNEL] INFO: SMP AP startup skipped by VMSettings.") ||
        TextContains(debug_text, "[KERNEL] INFO: SMP discovery skipped by VMSettings.");

    int cpu_local_apic_ok = want_local_apic ? cpu_local_apic : 1;
    int pic_ok = want_pic ? (pic_initialized && pic_remapped && pic_irq0 && pic_irq0_count && pic_eoi && pic_masked) : pic_skipped;
    int apic_ok = want_local_apic ? (apic_available && local_apic_enabled && apic_timer_probe && apic_timer_interrupt && apic_irq_counters && apic_eoi) : apic_skipped;
    int apic2_ok = want_apic2 ? (cpu_apic2 && apic2_enabled) : (apic2_skipped || !want_local_apic);
    int hpet_ok = want_hpet ? (hpet_rsdp && hpet_checksum && hpet_table && hpet_enabled && hpet_counter) : hpet_skipped;
    int interrupt_chain_ok = TextContains(debug_text, "[KERNEL] PASS: Interrupts work from PIC upward through APIC/APIC2.") ||
        TextContains(debug_text, "[KERNEL] PASS: VMSettings interrupt/timer profile applied.");
    int smp_ok = want_smp ? (smp_started_early && smp_started && smp_rsdp && smp_checksum && smp_madt && smp_cached &&
        smp_topology && smp_early_stage && smp_trampoline && smp_cr3 && smp_ipi && smp_init_ipi &&
        smp_startup_ipi && smp_aps_started && smp_early_complete && smp_complete) : smp_skipped;

    int virtual_memory_started = TextContains(debug_text, "[KERNEL] Virtual memory: starting");
    int virtual_memory_required_mapped = TextContains(debug_text, "[KERNEL] Virtual memory: required ranges mapped");
    int virtual_memory_switching_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: switching CR3 to kernel-owned PML4");
    int virtual_memory_switched_cr3 = TextContains(debug_text, "[KERNEL] Virtual memory: CR3 switched to kernel-owned PML4");
    int virtual_memory_active = TextContains(debug_text, "[KERNEL] Virtual memory: active");
    int system_halted = TextContains(debug_text, "[KERNEL] System halted by Kernel-5");
    int debug_exit = TextContains(debug_text, "[KERNEL] Requesting QEMU debug-exit success");
    int qemu_exit_or_hold = interactive_display ? interactive_hold : debug_exit;
    int boot_pass = qemu_completion_ok && loader_started &&
        kernel_loaded && entry_printed && virtual_map_prepared && virtual_map_active &&
        bootinfo_created && boot_config_prepared && bootinfo_memory_map && boot_services_exited && kernel_jump &&
        kernel_entered && serial_ok && bootinfo_received && kernel_boot_config && kernel_command_line &&
        gdt_installing && gdt_installed && gdt_entries && tss_loaded &&
        idt_installing && idt_installed && idt_entries &&
        interrupt_dispatcher && interrupt_handlers && interrupt_controlled && !cpu_exception &&
        cpu_local_apic_ok && pic_ok && apic_ok && apic2_ok && hpet_ok &&
        interrupt_chain_ok && syscall_core &&
        syscall_packet && syscall_get && syscall_set && syscall_event && syscall_unknown &&
        linux_translator && ms_translator && linux_vector && ms_vector &&
        unknown_linux && unknown_ms && syscall_three && platform_syscalls && syscall_counts &&
        pci_started && pci_rsdp && pci_checksum && pci_mcfg && pci_ecam && pci_config &&
        pci_scan && pci_devices && pci_class && pci_complete && pci_english &&
        smp_ok && qemu_debug_colour && kernel_console && screen_scrollback &&
        screen_coloured_cells && screen_scroll_lines && screen_page_scroll &&
        screen_bottom && screen_stable_proof && screen_scrolling && screen_back_buffer && screen_renders_back &&
        screen_deferred_flip && screen_line_flip && screen_dirty_line && screen_fast_scroll &&
        screen_refresh_optimized && screen_line_buffered && screen_present && screen_atomic_present && screen_double_buffer &&
        physical_capacity && keyboard_initialized && keyboard_irq1 && keyboard_interrupts && keyboard_pic_unmasked &&
        keyboard_decoder && keyboard_make_break && keyboard_release_stop && keyboard_line && keyboard_page &&
        keyboard_stops_on_release && (!interactive_display || interactive_interrupts) &&
        virtual_memory_active && qemu_exit_or_hold;

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
    fprintf(file, "VM Display: %s\n", report_display_mode);
    fprintf(file, "VM Output mode: %s\n", interactive_display ?
        "graphical framebuffer screen window" :
        "headless terminal serial/debug output");
    fprintf(file, "VM Interactive display hold: %s\n", interactive_display ? "yes" : "no");
    fprintf(file, "VM Headless timeout: %s", interactive_display ? "not used\n" : "");
    if (!interactive_display)
    {
        fprintf(file, "%u seconds\n", headless_timeout_seconds);
    }
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
    fprintf(file, "  QEMU completed before headless timeout: %s\n", PassFail(!qemu_timeout));
    if (qemu_no_output_timeout)
    {
        fprintf(file, "  Firmware, loader, and kernel checks: SKIPPED - no serial/debugcon output before timeout.\n");
        fprintf(file, "\nFailure hint:\n");
        fprintf(file, "  QEMU boot produced no serial or debugcon output within %u seconds.\n", headless_timeout_seconds);
        fprintf(file, "  Profile: %s\n", project->name);
        fprintf(file, "  Display: %s\n", report_display_mode);
        fprintf(file, "  Output mode: headless terminal serial/debug output, not graphical framebuffer screen output.\n");
        fprintf(file, "  The UEFI loader marker [BOOT] Stage 01 was not seen. Check OVMF, the staged FAT32 image, BOOTX64.EFI, and the QEMU command.\n\n");
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
    fprintf(file, "  CPU local APIC feature present: %s\n", PassFail(cpu_local_apic_ok));
    fprintf(file, "  CPU APIC2/x2APIC feature present: %s\n", PassFail(want_apic2 ? cpu_apic2 : 1));
    fprintf(file, "  PIC initialized: %s\n", PassFail(want_pic ? pic_initialized : pic_skipped));
    fprintf(file, "  PIC remapped to 0x20-0x2F: %s\n", PassFail(want_pic ? pic_remapped : pic_skipped));
    fprintf(file, "  PIC IRQ0 interrupt fired: %s\n", PassFail(want_pic ? pic_irq0 : pic_skipped));
    fprintf(file, "  PIC IRQ0 counter updated: %s\n", PassFail(want_pic ? pic_irq0_count : pic_skipped));
    fprintf(file, "  PIC EOI path executed: %s\n", PassFail(want_pic ? pic_eoi : pic_skipped));
    fprintf(file, "  PIC masked for APIC handoff: %s\n", PassFail(want_pic ? pic_masked : pic_skipped));
    fprintf(file, "  APIC CPU feature available: %s\n", PassFail(want_local_apic ? apic_available : apic_skipped));
    fprintf(file, "  APIC2/x2APIC mode enabled: %s\n", PassFail(want_apic2 ? apic2_enabled : 1));
    fprintf(file, "  Local APIC software enabled: %s\n", PassFail(want_local_apic ? local_apic_enabled : apic_skipped));
    fprintf(file, "  APIC timer masked probe counted down: %s\n", PassFail(want_local_apic ? apic_timer_probe : apic_skipped));
    fprintf(file, "  HPET RSDP input present: %s\n", PassFail(want_hpet ? hpet_rsdp : hpet_skipped));
    fprintf(file, "  HPET ACPI checksum validated: %s\n", PassFail(want_hpet ? hpet_checksum : hpet_skipped));
    fprintf(file, "  HPET table discovered: %s\n", PassFail(want_hpet ? hpet_table : hpet_skipped));
    fprintf(file, "  HPET main counter enabled: %s\n", PassFail(want_hpet ? hpet_enabled : hpet_skipped));
    fprintf(file, "  HPET counter advanced: %s\n", PassFail(want_hpet ? hpet_counter : hpet_skipped));
    fprintf(file, "  APIC timer interrupt fired: %s\n", PassFail(want_local_apic ? apic_timer_interrupt : apic_skipped));
    fprintf(file, "  APIC timer IRQ counter updated: %s\n", PassFail(want_local_apic ? apic_irq_counters : apic_skipped));
    fprintf(file, "  APIC EOI path executed: %s\n", PassFail(want_local_apic ? apic_eoi : apic_skipped));
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
    fprintf(file, "  SMP early startup requested after APIC/APIC2: %s\n", PassFail(want_smp ? smp_started_early : smp_skipped));
    fprintf(file, "  SMP discovery started: %s\n", PassFail(want_smp ? smp_started : smp_skipped));
    fprintf(file, "  SMP ACPI RSDP present: %s\n", PassFail(want_smp ? smp_rsdp : smp_skipped));
    fprintf(file, "  SMP ACPI checksum validated: %s\n", PassFail(want_smp ? smp_checksum : smp_skipped));
    fprintf(file, "  SMP ACPI MADT table discovered: %s\n", PassFail(want_smp ? smp_madt : smp_skipped));
    fprintf(file, "  SMP ACPI topology cached before VM: %s\n", PassFail(want_smp ? smp_cached : smp_skipped));
    fprintf(file, "  SMP multi-core topology discovered: %s\n", PassFail(want_smp ? smp_topology : smp_skipped));
    fprintf(file, "  SMP moved before PCI/HPET/console/memory: %s\n", PassFail(want_smp ? smp_early_stage : smp_skipped));
    fprintf(file, "  SMP AP trampoline prepared: %s\n", PassFail(want_smp ? smp_trampoline : smp_skipped));
    fprintf(file, "  SMP startup CR3 reachable: %s\n", PassFail(want_smp ? smp_cr3 : smp_skipped));
    fprintf(file, "  SMP Local APIC IPI path ready: %s\n", PassFail(want_smp ? smp_ipi : smp_skipped));
    fprintf(file, "  SMP INIT IPI sent: %s\n", PassFail(want_smp ? smp_init_ipi : smp_skipped));
    fprintf(file, "  SMP STARTUP IPI sent: %s\n", PassFail(want_smp ? smp_startup_ipi : smp_skipped));
    fprintf(file, "  SMP APs entered kernel loop: %s\n", PassFail(want_smp ? smp_aps_started : smp_skipped));
    fprintf(file, "  SMP AP startup completed before late boot proofs: %s\n", PassFail(want_smp ? smp_early_complete : smp_skipped));
    fprintf(file, "  Multi-Core processing initialized: %s\n", PassFail(want_smp ? smp_complete : smp_skipped));
    fprintf(file, "  QEMU debug output includes ANSI colour: %s\n", PassFail(qemu_debug_colour));
    fprintf(file, "  Kernel console initialized: %s\n", PassFail(kernel_console));
    fprintf(file, "  Kernel screen scrollback buffer initialized: %s\n", PassFail(screen_scrollback));
    fprintf(file, "  Kernel screen coloured scrollback cells: %s\n", PassFail(screen_coloured_cells));
    fprintf(file, "  Kernel screen line scroll up/down: %s\n", PassFail(screen_scroll_lines));
    fprintf(file, "  Kernel screen page up/down: %s\n", PassFail(screen_page_scroll));
    fprintf(file, "  Kernel screen scroll-to-bottom: %s\n", PassFail(screen_bottom));
    fprintf(file, "  Kernel screen stable scroll proof: %s\n", PassFail(screen_stable_proof));
    fprintf(file, "  Kernel screen scrolling implemented: %s\n", PassFail(screen_scrolling));
    fprintf(file, "  Kernel screen back buffer allocated: %s\n", PassFail(screen_back_buffer));
    fprintf(file, "  Kernel screen renders into back buffer first: %s\n", PassFail(screen_renders_back));
    fprintf(file, "  Kernel screen defers flip while line is being written: %s\n", PassFail(screen_deferred_flip));
    fprintf(file, "  Kernel screen flips after completed line: %s\n", PassFail(screen_line_flip));
    fprintf(file, "  Kernel screen presents dirty completed line only: %s\n", PassFail(screen_dirty_line));
    fprintf(file, "  Kernel screen fast scroll path active: %s\n", PassFail(screen_fast_scroll));
    fprintf(file, "  Kernel screen refresh line/scroll optimized: %s\n", PassFail(screen_refresh_optimized));
    fprintf(file, "  Kernel screen line-buffered double buffering implemented: %s\n", PassFail(screen_line_buffered));
    fprintf(file, "  Kernel screen presents completed frame: %s\n", PassFail(screen_present));
    fprintf(file, "  Kernel screen visible presents are atomic: %s\n", PassFail(screen_atomic_present));
    fprintf(file, "  Kernel screen double buffering implemented: %s\n", PassFail(screen_double_buffer));
    fprintf(file, "  Physical allocator tracking capacity sufficient: %s\n", PassFail(physical_capacity));
    fprintf(file, "  Keyboard interrupt scrolling initialized: %s\n", PassFail(keyboard_initialized));
    fprintf(file, "  PS/2 keyboard IRQ1 handler registered: %s\n", PassFail(keyboard_irq1));
    fprintf(file, "  Keyboard scrolling uses IRQ1 interrupts: %s\n", PassFail(keyboard_interrupts));
    fprintf(file, "  PIC IRQ1 unmasked for keyboard input: %s\n", PassFail(keyboard_pic_unmasked));
    fprintf(file, "  Keyboard arrow/page decoder ready: %s\n", PassFail(keyboard_decoder));
    fprintf(file, "  Keyboard make/break state tracking: %s\n", PassFail(keyboard_make_break));
    fprintf(file, "  Keyboard release stops scrolling immediately: %s\n", PassFail(keyboard_release_stop));
    fprintf(file, "  Keyboard Up/Down scroll one line while held: %s\n", PassFail(keyboard_line));
    fprintf(file, "  Keyboard PgUp/PgDn scroll one page while held: %s\n", PassFail(keyboard_page));
    fprintf(file, "  Keyboard scrolling stops when key released: %s\n", PassFail(keyboard_stops_on_release));
    fprintf(file, "  Interactive halt leaves interrupts enabled: %s\n", interactive_display ? PassFail(interactive_interrupts) : "SKIPPED - headless run");
    fprintf(file, "  Kernel virtual memory started: %s\n", PassFail(virtual_memory_started));
    fprintf(file, "  Kernel virtual memory required ranges mapped: %s\n", PassFail(virtual_memory_required_mapped));
    fprintf(file, "  Kernel virtual memory CR3 switch requested: %s\n", PassFail(virtual_memory_switching_cr3));
    fprintf(file, "  Kernel virtual memory CR3 switch completed: %s\n", PassFail(virtual_memory_switched_cr3));
