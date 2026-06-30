#include "TargetBuildInternal.h"
void WriteBootReport(
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
    OrynBootReportFacts facts;
    CollectBootReportFacts(project, debug_text, exit_code, command_ok, &facts);

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
    fprintf(file, "VM Display: %s\n", facts.report_display_mode);
    fprintf(file, "VM Output mode: %s\n", facts.interactive_display ?
        "graphical framebuffer screen window" :
        "headless terminal serial/debug output");
    fprintf(file, "VM Interactive display hold: %s\n", facts.interactive_display ? "yes" : "no");
    fprintf(file, "VM Headless timeout: %s", facts.interactive_display ? "not used\n" : "");
    if (!facts.interactive_display)
    {
        fprintf(file, "%u seconds\n", facts.headless_timeout_seconds);
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
    fprintf(file, "Result: %s\n", PassFail(facts.boot_pass));
    fprintf(file, "QEMU exit code: %d\n\n", exit_code);

    fprintf(file, "Checks:\n");
    fprintf(file, "  QEMU command accepted: %s\n", PassFail(command_ok));
    fprintf(file, "  QEMU completed before headless timeout: %s\n", PassFail(!facts.qemu_timeout));
    if (facts.qemu_no_output_timeout)
    {
        fprintf(file, "  Firmware, loader, and kernel checks: SKIPPED - no serial/debugcon output before timeout.\n");
        fprintf(file, "\nFailure hint:\n");
        fprintf(file, "  QEMU boot produced no serial or debugcon output within %u seconds.\n", facts.headless_timeout_seconds);
        fprintf(file, "  Profile: %s\n", project->name);
        fprintf(file, "  Display: %s\n", facts.report_display_mode);
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
    if (facts.qemu_preboot_failure)
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
    fprintf(file, "  Loader started: %s\n", PassFail(facts.loader_started));
    fprintf(file, "  Kernel loaded physical address printed: %s\n", PassFail(facts.kernel_loaded));
    fprintf(file, "  Kernel virtual entry address printed: %s\n", PassFail(facts.entry_printed));
    fprintf(file, "  Loader prepared temporary higher-half/chosen map: %s\n", PassFail(facts.virtual_map_prepared));
    fprintf(file, "  Loader activated temporary higher-half/chosen map: %s\n", PassFail(facts.virtual_map_active));
    fprintf(file, "  BootInfo allocated: %s\n", PassFail(facts.bootinfo_created));
    fprintf(file, "  Loader prepared boot configuration block: %s\n", PassFail(facts.boot_config_prepared));
    fprintf(file, "  BootInfo memory map requested: %s\n", PassFail(facts.memory_map_requested));
    fprintf(file, "  BootInfo memory map captured or intentionally omitted: %s\n", PassFail(facts.bootinfo_memory_map));
    fprintf(file, "  ExitBootServices succeeded: %s\n", PassFail(facts.boot_services_exited));
    fprintf(file, "  Loader jumped to kernel: %s\n", PassFail(facts.kernel_jump));
    fprintf(file, "  Kernel entered: %s\n", PassFail(facts.kernel_entered));
    fprintf(file, "  Serial/debug output reached kernel: %s\n", PassFail(facts.serial_ok));
    fprintf(file, "  Kernel received BootInfo: %s\n", PassFail(facts.bootinfo_received));
    fprintf(file, "  Kernel received boot configuration block: %s\n", PassFail(facts.kernel_boot_config));
    fprintf(file, "  Kernel received command line: %s\n", PassFail(facts.kernel_command_line));
    fprintf(file, "  Kernel started GDT install: %s\n", PassFail(facts.gdt_installing));
    fprintf(file, "  Kernel installed and verified GDT: %s\n", PassFail(facts.gdt_installed));
    fprintf(file, "  Kernel installed all 7 GDT entries: %s\n", PassFail(facts.gdt_entries));
    fprintf(file, "  Kernel loaded TSS: %s\n", PassFail(facts.tss_loaded));
    fprintf(file, "  Kernel started IDT install: %s\n", PassFail(facts.idt_installing));
    fprintf(file, "  Kernel installed and verified IDT: %s\n", PassFail(facts.idt_installed));
    fprintf(file, "  Kernel installed all 256 IDT entries: %s\n", PassFail(facts.idt_entries));
    fprintf(file, "  Interrupt dispatcher initialized: %s\n", PassFail(facts.interrupt_dispatcher));
    fprintf(file, "  Interrupt handler table ready: %s\n", PassFail(facts.interrupt_handlers));
    fprintf(file, "  CPU interrupts controlled during boot: %s\n", PassFail(facts.interrupt_controlled));
    fprintf(file, "  Kernel had no trapped CPU exception: %s\n", PassFail(!facts.cpu_exception));
    fprintf(file, "  CPU local APIC feature present: %s\n", PassFail(facts.cpu_local_apic_ok));
    fprintf(file, "  CPU APIC2/x2APIC feature present: %s\n", PassFail(facts.want_apic2 ? facts.cpu_apic2 : 1));
    fprintf(file, "  PIC initialized: %s\n", PassFail(facts.want_pic ? facts.pic_initialized : facts.pic_skipped));
    fprintf(file, "  PIC remapped to 0x20-0x2F: %s\n", PassFail(facts.want_pic ? facts.pic_remapped : facts.pic_skipped));
    fprintf(file, "  PIC IRQ0 interrupt fired: %s\n", PassFail(facts.want_pic ? facts.pic_irq0 : facts.pic_skipped));
    fprintf(file, "  PIC IRQ0 counter updated: %s\n", PassFail(facts.want_pic ? facts.pic_irq0_count : facts.pic_skipped));
    fprintf(file, "  PIC EOI path executed: %s\n", PassFail(facts.want_pic ? facts.pic_eoi : facts.pic_skipped));
    fprintf(file, "  PIC masked for APIC handoff: %s\n", PassFail(facts.want_pic ? facts.pic_masked : facts.pic_skipped));
    fprintf(file, "  APIC CPU feature available: %s\n", PassFail(facts.want_local_apic ? facts.apic_available : facts.apic_skipped));
    fprintf(file, "  APIC2/x2APIC mode enabled: %s\n", PassFail(facts.want_apic2 ? facts.apic2_enabled : 1));
    fprintf(file, "  Local APIC software enabled: %s\n", PassFail(facts.want_local_apic ? facts.local_apic_enabled : facts.apic_skipped));
    fprintf(file, "  APIC timer masked probe counted down: %s\n", PassFail(facts.want_local_apic ? facts.apic_timer_probe : facts.apic_skipped));
    fprintf(file, "  HPET RSDP input present: %s\n", PassFail(facts.want_hpet ? facts.hpet_rsdp : facts.hpet_skipped));
    fprintf(file, "  HPET ACPI checksum validated: %s\n", PassFail(facts.want_hpet ? facts.hpet_checksum : facts.hpet_skipped));
    fprintf(file, "  HPET table discovered: %s\n", PassFail(facts.want_hpet ? facts.hpet_table : facts.hpet_skipped));
    fprintf(file, "  HPET main counter enabled: %s\n", PassFail(facts.want_hpet ? facts.hpet_enabled : facts.hpet_skipped));
    fprintf(file, "  HPET counter advanced: %s\n", PassFail(facts.want_hpet ? facts.hpet_counter : facts.hpet_skipped));
    fprintf(file, "  APIC timer interrupt fired: %s\n", PassFail(facts.want_local_apic ? facts.apic_timer_interrupt : facts.apic_skipped));
    fprintf(file, "  APIC timer IRQ counter updated: %s\n", PassFail(facts.want_local_apic ? facts.apic_irq_counters : facts.apic_skipped));
    fprintf(file, "  APIC EOI path executed: %s\n", PassFail(facts.want_local_apic ? facts.apic_eoi : facts.apic_skipped));
    fprintf(file, "  Interrupt chain PIC upward complete: %s\n", PassFail(facts.interrupt_chain_ok));
    fprintf(file, "  SysCall core initialized: %s\n", PassFail(facts.syscall_core));
    fprintf(file, "  SysCall message packet ABI ready: %s\n", PassFail(facts.syscall_packet));
    fprintf(file, "  SysCallGet packet handled: %s\n", PassFail(facts.syscall_get));
    fprintf(file, "  SysCallSet packet handled: %s\n", PassFail(facts.syscall_set));
    fprintf(file, "  SysCallEvent packet handled: %s\n", PassFail(facts.syscall_event));
    fprintf(file, "  Unknown syscall debug log path: %s\n", PassFail(facts.syscall_unknown));
    fprintf(file, "  LinuxSysCall translator registered: %s\n", PassFail(facts.linux_translator));
    fprintf(file, "  MSSysCall translator registered: %s\n", PassFail(facts.ms_translator));
    fprintf(file, "  Linux syscall vector 0x80 translated: %s\n", PassFail(facts.linux_vector));
    fprintf(file, "  MS syscall vector 0x81 translated: %s\n", PassFail(facts.ms_vector));
    fprintf(file, "  Unknown Linux syscall debug report: %s\n", PassFail(facts.unknown_linux));
    fprintf(file, "  Unknown MS syscall debug report: %s\n", PassFail(facts.unknown_ms));
    fprintf(file, "  Internal SysCalls use Get/Set/Event packets: %s\n", PassFail(facts.syscall_three));
    fprintf(file, "  Platform syscalls translate to Get/Set/Event: %s\n", PassFail(facts.platform_syscalls));
    fprintf(file, "  SysCall header counts listed: %s\n", PassFail(facts.syscall_counts));
    fprintf(file, "  PCI discovery started: %s\n", PassFail(facts.pci_started));
    fprintf(file, "  PCI ACPI RSDP present: %s\n", PassFail(facts.pci_rsdp));
    fprintf(file, "  PCI ACPI checksum validated: %s\n", PassFail(facts.pci_checksum));
    fprintf(file, "  PCI ACPI MCFG table discovered: %s\n", PassFail(facts.pci_mcfg));
    fprintf(file, "  PCIe ECAM descriptor captured: %s\n", PassFail(facts.pci_ecam));
    fprintf(file, "  PCI config mechanism #1 responded: %s\n", PassFail(facts.pci_config));
    fprintf(file, "  PCI bus/device/function scan completed: %s\n", PassFail(facts.pci_scan));
    fprintf(file, "  PCI devices discovered: %s\n", PassFail(facts.pci_devices));
    fprintf(file, "  PCI class-code decoding ready: %s\n", PassFail(facts.pci_class));
    fprintf(file, "  PCI discovery complete: %s\n", PassFail(facts.pci_complete));
    fprintf(file, "  PCI device output uses English labels: %s\n", PassFail(facts.pci_english));
    fprintf(file, "  SMP early startup requested after APIC/APIC2: %s\n", PassFail(facts.want_smp ? facts.smp_started_early : facts.smp_skipped));
    fprintf(file, "  SMP discovery started: %s\n", PassFail(facts.want_smp ? facts.smp_started : facts.smp_skipped));
    fprintf(file, "  SMP ACPI RSDP present: %s\n", PassFail(facts.want_smp ? facts.smp_rsdp : facts.smp_skipped));
    fprintf(file, "  SMP ACPI checksum validated: %s\n", PassFail(facts.want_smp ? facts.smp_checksum : facts.smp_skipped));
    fprintf(file, "  SMP ACPI MADT table discovered: %s\n", PassFail(facts.want_smp ? facts.smp_madt : facts.smp_skipped));
    fprintf(file, "  SMP ACPI topology cached before VM: %s\n", PassFail(facts.want_smp ? facts.smp_cached : facts.smp_skipped));
    fprintf(file, "  SMP multi-core topology discovered: %s\n", PassFail(facts.want_smp ? facts.smp_topology : facts.smp_skipped));
    fprintf(file, "  SMP moved before PCI/HPET/console/memory: %s\n", PassFail(facts.want_smp ? facts.smp_early_stage : facts.smp_skipped));
    fprintf(file, "  SMP AP trampoline prepared: %s\n", PassFail(facts.want_smp ? facts.smp_trampoline : facts.smp_skipped));
    fprintf(file, "  SMP startup CR3 reachable: %s\n", PassFail(facts.want_smp ? facts.smp_cr3 : facts.smp_skipped));
    fprintf(file, "  SMP Local APIC IPI path ready: %s\n", PassFail(facts.want_smp ? facts.smp_ipi : facts.smp_skipped));
    fprintf(file, "  SMP INIT IPI sent: %s\n", PassFail(facts.want_smp ? facts.smp_init_ipi : facts.smp_skipped));
    fprintf(file, "  SMP STARTUP IPI sent: %s\n", PassFail(facts.want_smp ? facts.smp_startup_ipi : facts.smp_skipped));
    fprintf(file, "  SMP APs entered kernel loop: %s\n", PassFail(facts.want_smp ? facts.smp_aps_started : facts.smp_skipped));
    fprintf(file, "  SMP AP startup completed before late boot proofs: %s\n", PassFail(facts.want_smp ? facts.smp_early_complete : facts.smp_skipped));
    fprintf(file, "  Multi-Core processing initialized: %s\n", PassFail(facts.want_smp ? facts.smp_complete : facts.smp_skipped));
    fprintf(file, "  QEMU debug output includes ANSI colour: %s\n", PassFail(facts.qemu_debug_colour));
    fprintf(file, "  Kernel console initialized: %s\n", PassFail(facts.kernel_console));
    fprintf(file, "  Kernel screen scrollback buffer initialized: %s\n", PassFail(facts.screen_scrollback));
    fprintf(file, "  Kernel screen coloured scrollback cells: %s\n", PassFail(facts.screen_coloured_cells));
    fprintf(file, "  Kernel screen line scroll up/down: %s\n", PassFail(facts.screen_scroll_lines));
    fprintf(file, "  Kernel screen page up/down: %s\n", PassFail(facts.screen_page_scroll));
    fprintf(file, "  Kernel screen scroll-to-bottom: %s\n", PassFail(facts.screen_bottom));
    fprintf(file, "  Kernel screen stable scroll proof: %s\n", PassFail(facts.screen_stable_proof));
    fprintf(file, "  Kernel screen scrolling implemented: %s\n", PassFail(facts.screen_scrolling));
    fprintf(file, "  Kernel screen back buffer allocated: %s\n", PassFail(facts.screen_back_buffer));
    fprintf(file, "  Kernel screen renders into back buffer first: %s\n", PassFail(facts.screen_renders_back));
    fprintf(file, "  Kernel screen defers flip while line is being written: %s\n", PassFail(facts.screen_deferred_flip));
    fprintf(file, "  Kernel screen flips after completed line: %s\n", PassFail(facts.screen_line_flip));
    fprintf(file, "  Kernel screen presents dirty completed line only: %s\n", PassFail(facts.screen_dirty_line));
    fprintf(file, "  Kernel screen fast scroll path active: %s\n", PassFail(facts.screen_fast_scroll));
    fprintf(file, "  Kernel screen refresh line/scroll optimized: %s\n", PassFail(facts.screen_refresh_optimized));
    fprintf(file, "  Kernel screen line-buffered double buffering implemented: %s\n", PassFail(facts.screen_line_buffered));
    fprintf(file, "  Kernel screen presents completed frame: %s\n", PassFail(facts.screen_present));
    fprintf(file, "  Kernel screen visible presents are atomic: %s\n", PassFail(facts.screen_atomic_present));
    fprintf(file, "  Kernel screen double buffering implemented: %s\n", PassFail(facts.screen_double_buffer));
    fprintf(file, "  Physical allocator tracking capacity sufficient: %s\n", PassFail(facts.physical_capacity));
    fprintf(file, "  Keyboard interrupt scrolling initialized: %s\n", PassFail(facts.keyboard_initialized));
    fprintf(file, "  PS/2 keyboard IRQ1 handler registered: %s\n", PassFail(facts.keyboard_irq1));
    fprintf(file, "  Keyboard scrolling uses IRQ1 interrupts: %s\n", PassFail(facts.keyboard_interrupts));
    fprintf(file, "  PIC IRQ1 unmasked for keyboard input: %s\n", PassFail(facts.keyboard_pic_unmasked));
    fprintf(file, "  Keyboard arrow/page decoder ready: %s\n", PassFail(facts.keyboard_decoder));
    fprintf(file, "  Keyboard make/break state tracking: %s\n", PassFail(facts.keyboard_make_break));
    fprintf(file, "  Keyboard release stops scrolling immediately: %s\n", PassFail(facts.keyboard_release_stop));
    fprintf(file, "  Keyboard Up/Down scroll one line while held: %s\n", PassFail(facts.keyboard_line));
    fprintf(file, "  Keyboard PgUp/PgDn scroll one page while held: %s\n", PassFail(facts.keyboard_page));
    fprintf(file, "  Keyboard scrolling stops when key released: %s\n", PassFail(facts.keyboard_stops_on_release));
    fprintf(file, "  Interactive halt leaves interrupts enabled: %s\n", facts.interactive_display ? PassFail(facts.interactive_interrupts) : "SKIPPED - headless run");
    fprintf(file, "  Kernel virtual memory started: %s\n", PassFail(facts.virtual_memory_started));
    fprintf(file, "  Kernel virtual memory required ranges mapped: %s\n", PassFail(facts.virtual_memory_required_mapped));
    fprintf(file, "  Kernel virtual memory CR3 switch requested: %s\n", PassFail(facts.virtual_memory_switching_cr3));
    fprintf(file, "  Kernel virtual memory CR3 switch completed: %s\n", PassFail(facts.virtual_memory_switched_cr3));

    fprintf(file, "  Kernel virtual memory active proof printed: %s\n", PassFail(facts.virtual_memory_active));
    fprintf(file, "  Kernel reached halt path: %s\n", PassFail(facts.system_halted));
    if (facts.interactive_display)
    {
        fprintf(file, "  Interactive display keeps QEMU open for scroll testing: %s\n", PassFail(facts.interactive_hold));
        fprintf(file, "  Kernel requested QEMU debug-exit: SKIPPED - interactive display mode\n");
    }
    else
    {
        fprintf(file, "  Interactive display keeps QEMU open for scroll testing: SKIPPED - headless run\n");
        fprintf(file, "  Kernel requested QEMU debug-exit: %s\n", PassFail(facts.debug_exit));
    }

    const char* final_halt_hint = facts.interactive_display ?
        "The kernel halted but did not print the interactive display hold marker." :
        "The kernel halted but did not request QEMU debug-exit success.";

    fprintf(file, "\nFailure hint:\n  %s\n\n",
        facts.boot_pass ?
            "No kernel-side failure detected by the boot-proof markers." :
        facts.qemu_timeout ?
            "QEMU did not complete before the headless timeout. Check the last printed [BOOT]/[KERNEL] marker, Debug.log, and BootReport.txt." :
        !command_ok ?
            "QEMU did not accept the VM command line. Check CPU, memory, and acceleration settings." :
        !facts.loader_started ?
            "The loader did not start." :
        !facts.kernel_jump ?
            "The loader did not reach the kernel jump." :
        !facts.kernel_entered ?
            "The kernel did not print the entry success marker." :
        !facts.kernel_boot_config ?
            "The kernel did not validate the boot configuration block." :
        !facts.kernel_command_line ?
            "The kernel did not validate the command line." :
        !facts.gdt_installing ?
            "The kernel did not start GDT installation." :
        !facts.gdt_installed ?
            "The kernel did not verify the GDT install." :
        !facts.gdt_entries ?
            "The kernel did not install the expected GDT entries." :
        !facts.tss_loaded ?
            "The kernel did not load the TSS selector." :
        !facts.idt_installing ?
            "The kernel did not start IDT installation." :
        !facts.idt_installed ?
            "The kernel did not verify the IDT install." :
        !facts.idt_entries ?
            "The kernel did not install all 256 IDT entries." :
        !facts.interrupt_dispatcher ?
            "The interrupt dispatcher did not initialize." :
        !facts.interrupt_handlers ?
            "The interrupt handler table was not prepared." :
        !facts.interrupt_controlled ?
            "CPU interrupts were not under controlled CLI/STI boot state." :
        facts.cpu_exception ?
            "The IDT trapped a CPU exception. Check the exception vector and register dump." :
        !facts.cpu_local_apic ?
            "The CPU did not expose the local APIC feature." :
        !facts.cpu_apic2 ?
            "QEMU did not expose APIC2/x2APIC. Check the -cpu qemu64,+x2apic option." :
        !facts.pic_initialized ?
            "The kernel did not initialize the 8259 PIC path." :
        !facts.pic_remapped ?
            "The kernel did not remap the PIC vectors to 0x20-0x2F." :
        !facts.pic_masked ?
            "The kernel did not mask the PIC for APIC handoff." :
        !facts.pic_irq0 ?
            "The remapped PIC did not deliver IRQ0 through IDT vector 0x20." :
        !facts.pic_irq0_count ?
            "The PIC IRQ0 handler did not update its dispatch counter." :
        !facts.pic_eoi ?
            "The PIC interrupt EOI path did not execute." :
        !facts.apic_available ?
            "The kernel did not reach APIC feature validation." :
        !facts.apic2_enabled ?
            "The kernel did not enable APIC2/x2APIC mode." :
        !facts.local_apic_enabled ?
            "The kernel did not set the local APIC software enable bit." :
        !facts.apic_timer_probe ?
            "The APIC timer masked probe did not count down." :
        !facts.hpet_rsdp ?
            "The kernel did not receive the ACPI RSDP needed for HPET discovery." :
        !facts.hpet_checksum ?
            "The HPET ACPI table walk failed checksum validation." :
        !facts.hpet_table ?
            "The HPET ACPI table was not discovered." :
        !facts.hpet_enabled ?
            "The HPET main counter was not enabled." :
        !facts.hpet_counter ?
            "The HPET counter did not advance during the probe." :
        !facts.apic_timer_interrupt ?
            "The APIC timer interrupt did not fire through the IDT dispatcher." :
        !facts.apic_irq_counters ?
            "The APIC timer IRQ counter did not update." :
        !facts.apic_eoi ?
            "The APIC EOI path did not execute." :
        !facts.interrupt_chain ?
            "The interrupt chain did not pass from PIC upward through APIC/APIC2." :
        !facts.syscall_core ?
            "The internal SysCall core did not initialize." :
        !facts.syscall_packet ?
            "The SysCall message-packet ABI was not prepared." :
        !facts.syscall_get ?
            "SysCallGet did not handle its proof packet." :
        !facts.syscall_set ?
            "SysCallSet did not handle its proof packet." :
        !facts.syscall_event ?
            "SysCallEvent did not handle its proof packet." :
        !facts.syscall_unknown ?
            "Unknown syscall debug logging did not run." :
        !facts.linux_translator ?
            "LinuxSysCall.h translator was not registered." :
        !facts.ms_translator ?
            "MSSysCall.h translator was not registered." :
        !facts.linux_vector ?
            "The Linux syscall vector 0x80 did not translate into the internal syscall system." :
        !facts.ms_vector ?
            "The MS syscall vector 0x81 did not translate into the internal syscall system." :
        !facts.unknown_linux ?
            "Unknown Linux syscall reporting did not produce a debug path." :
        !facts.unknown_ms ?
            "Unknown MS syscall reporting did not produce a debug path." :
        !facts.syscall_three ?
            "The internal syscall proof did not use Get/Set/Event packets." :
        !facts.platform_syscalls ?
            "Platform syscall compatibility did not translate into Get/Set/Event packets." :
        !facts.syscall_counts ?
            "The LinuxSysCall.h and MSSysCall.h count markers were not printed." :
        !facts.pci_started ?
            "The PCI discovery module did not start." :
        !facts.pci_rsdp ?
            "The PCI discovery module did not receive the ACPI RSDP." :
        !facts.pci_checksum ?
            "The PCI ACPI table walk failed checksum validation." :
        !facts.pci_mcfg ?
            "The ACPI MCFG table for PCIe ECAM was not discovered." :
        !facts.pci_ecam ?
            "The PCIe ECAM descriptor was not captured from MCFG." :
        !facts.pci_config ?
            "PCI config mechanism #1 did not respond with any devices." :
        !facts.pci_scan ?
            "The PCI bus/device/function scan did not complete." :
        !facts.pci_devices ?
            "The PCI scan completed but found no devices." :
        !facts.pci_class ?
            "The PCI class-code decoder did not initialize." :
        !facts.pci_complete ?
            "The PCI discovery module did not emit its completion marker." :
        !facts.pci_english ?
            "The PCI device output did not emit the English-label proof marker." :
        !facts.smp_started ?
            "The SMP discovery module did not start." :
        !facts.smp_rsdp ?
            "The SMP module did not receive the ACPI RSDP." :
        !facts.smp_checksum ?
            "The SMP ACPI table walk failed checksum validation." :
        !facts.smp_madt ?
            "The ACPI MADT table was not discovered." :
        !facts.smp_cached ?
            "SMP did not cache ACPI topology before the virtual-memory CR3 switch." :
        !facts.smp_topology ?
            "SMP did not discover more than one enabled CPU. Check the QEMU -smp option." :
        !facts.smp_trampoline ?
            "The SMP AP startup trampoline was not prepared." :
        !facts.smp_cr3 ?
            "The SMP startup CR3 was not reachable from the AP trampoline." :
        !facts.smp_ipi ?
            "The Local APIC IPI path was unavailable for SMP startup." :
        !facts.smp_init_ipi ?
            "The SMP INIT IPI was not sent to application processors." :
        !facts.smp_startup_ipi ?
            "The SMP STARTUP IPI was not sent to application processors." :
        !facts.smp_aps_started ?
            "Application processors did not enter the kernel AP loop." :
        !facts.smp_complete ?
            "The SMP module did not emit its completion marker." :
        !facts.qemu_debug_colour ?
            "The QEMU debug log did not include ANSI colour sequences for status lines." :
        !facts.kernel_console ?
            "The kernel console did not initialize." :
        !facts.screen_scrollback ?
            "The kernel screen scrollback buffer was not initialized." :
        !facts.screen_coloured_cells ?
            "The kernel screen did not preserve coloured scrollback cells." :
        !facts.screen_scroll_lines ?
            "The kernel screen line scrolling proof did not pass." :
        !facts.screen_page_scroll ?
            "The kernel screen page scrolling proof did not pass." :
        !facts.screen_bottom ?
            "The kernel screen did not return to the live bottom view." :
        !facts.screen_stable_proof ?
            "The kernel screen scroll proof visibly replayed old rows instead of staying stable." :
        !facts.screen_scrolling ?
            "Kernel screen scrolling did not complete." :
        !facts.screen_back_buffer ?
            "The kernel screen back buffer was not allocated." :
        !facts.screen_renders_back ?
            "The kernel screen did not render into the back buffer first." :
        !facts.screen_deferred_flip ?
            "The kernel screen still flipped while a line was being written." :
        !facts.screen_line_flip ?
            "The kernel screen did not flip after a completed line." :
        !facts.screen_dirty_line ?
            "The kernel screen still presents more than the dirty completed line during normal output." :
        !facts.screen_fast_scroll ?
            "The kernel screen did not use the fast scroll path after the visible area filled." :
        !facts.screen_refresh_optimized ?
            "Kernel screen refresh optimization did not complete." :
        !facts.screen_line_buffered ?
            "Kernel screen line-buffered double buffering did not complete." :
        !facts.screen_present ?
            "The kernel screen did not present completed frames to visible output." :
        !facts.screen_atomic_present ?
            "The kernel screen visible framebuffer presents were not atomic." :
        !facts.screen_double_buffer ?
            "Kernel screen double buffering did not complete." :
        !facts.physical_capacity ?
            "The physical allocator did not have enough static tracking capacity for this VM memory size." :
        !facts.keyboard_initialized ?
            "Keyboard interrupt scrolling was not initialized." :
        !facts.keyboard_irq1 ?
            "The PS/2 keyboard IRQ1 handler was not registered." :
        !facts.keyboard_interrupts ?
            "Keyboard scrolling is not using interrupt-driven IRQ1 input." :
        !facts.keyboard_pic_unmasked ?
            "PIC IRQ1 was not unmasked for keyboard input." :
        !facts.keyboard_decoder ?
            "The keyboard arrow/page scan-code decoder was not initialized." :
        !facts.keyboard_make_break ?
            "Keyboard scroll keys do not use make/break state tracking." :
        !facts.keyboard_release_stop ?
            "Keyboard release scan codes do not stop held scrolling immediately." :
        !facts.keyboard_line ?
            "Keyboard Up/Down held line scrolling was not configured." :
        !facts.keyboard_page ?
            "Keyboard PgUp/PgDn held page scrolling was not configured." :
        !facts.keyboard_stops_on_release ?
            "Keyboard scrolling does not stop when the key is released." :
        facts.interactive_display && !facts.interactive_interrupts ?
            "Interactive display mode did not leave interrupts enabled for keyboard scrolling." :
        !facts.virtual_memory_started ?
            "The kernel stopped before virtual-memory initialization." :
        !facts.virtual_memory_required_mapped ?
            "The kernel stopped while building required virtual-memory mappings." :
        !facts.virtual_memory_switching_cr3 ?
            "The kernel stopped after mapping required ranges and before attempting the CR3 switch." :
        !facts.virtual_memory_switched_cr3 ?
            "The kernel stopped during the CR3 switch to the kernel-owned PML4." :
        !facts.system_halted ?
            "The kernel passed the CR3 switch but did not reach its halt path." :
        !facts.qemu_exit_or_hold ?
            final_halt_hint :
            "No kernel-side failure detected by the boot-proof markers.");

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

