    fprintf(file, "  Kernel virtual memory active proof printed: %s\n", PassFail(virtual_memory_active));
    fprintf(file, "  Kernel reached halt path: %s\n", PassFail(system_halted));
    if (interactive_display)
    {
        fprintf(file, "  Interactive display keeps QEMU open for scroll testing: %s\n", PassFail(interactive_hold));
        fprintf(file, "  Kernel requested QEMU debug-exit: SKIPPED - interactive display mode\n");
    }
    else
    {
        fprintf(file, "  Interactive display keeps QEMU open for scroll testing: SKIPPED - headless run\n");
        fprintf(file, "  Kernel requested QEMU debug-exit: %s\n", PassFail(debug_exit));
    }

    const char* final_halt_hint = interactive_display ?
        "The kernel halted but did not print the interactive display hold marker." :
        "The kernel halted but did not request QEMU debug-exit success.";

    fprintf(file, "\nFailure hint:\n  %s\n\n",
        boot_pass ?
            "No kernel-side failure detected by the boot-proof markers." :
        qemu_timeout ?
            "QEMU did not complete before the headless timeout. Check the last printed [BOOT]/[KERNEL] marker, Debug.log, and BootReport.txt." :
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
        !screen_stable_proof ?
            "The kernel screen scroll proof visibly replayed old rows instead of staying stable." :
        !screen_scrolling ?
            "Kernel screen scrolling did not complete." :
        !screen_back_buffer ?
            "The kernel screen back buffer was not allocated." :
        !screen_renders_back ?
            "The kernel screen did not render into the back buffer first." :
        !screen_deferred_flip ?
            "The kernel screen still flipped while a line was being written." :
        !screen_line_flip ?
            "The kernel screen did not flip after a completed line." :
        !screen_dirty_line ?
            "The kernel screen still presents more than the dirty completed line during normal output." :
        !screen_fast_scroll ?
            "The kernel screen did not use the fast scroll path after the visible area filled." :
        !screen_refresh_optimized ?
            "Kernel screen refresh optimization did not complete." :
        !screen_line_buffered ?
            "Kernel screen line-buffered double buffering did not complete." :
        !screen_present ?
            "The kernel screen did not present completed frames to visible output." :
        !screen_atomic_present ?
            "The kernel screen visible framebuffer presents were not atomic." :
        !screen_double_buffer ?
            "Kernel screen double buffering did not complete." :
        !physical_capacity ?
            "The physical allocator did not have enough static tracking capacity for this VM memory size." :
        !keyboard_initialized ?
            "Keyboard interrupt scrolling was not initialized." :
        !keyboard_irq1 ?
            "The PS/2 keyboard IRQ1 handler was not registered." :
        !keyboard_interrupts ?
            "Keyboard scrolling is not using interrupt-driven IRQ1 input." :
        !keyboard_pic_unmasked ?
            "PIC IRQ1 was not unmasked for keyboard input." :
        !keyboard_decoder ?
            "The keyboard arrow/page scan-code decoder was not initialized." :
        !keyboard_make_break ?
            "Keyboard scroll keys do not use make/break state tracking." :
        !keyboard_release_stop ?
            "Keyboard release scan codes do not stop held scrolling immediately." :
        !keyboard_line ?
            "Keyboard Up/Down held line scrolling was not configured." :
        !keyboard_page ?
            "Keyboard PgUp/PgDn held page scrolling was not configured." :
        !keyboard_stops_on_release ?
            "Keyboard scrolling does not stop when the key is released." :
        interactive_display && !interactive_interrupts ?
            "Interactive display mode did not leave interrupts enabled for keyboard scrolling." :
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
        !qemu_exit_or_hold ?
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

