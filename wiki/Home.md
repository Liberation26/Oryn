# Oryn SDK Wiki

Welcome to the Oryn SDK Wiki — your complete guide to building operating systems with the Oryn Development Kit.

## Getting Started

- **[Quick Start Guide](Quick-Start-Guide)** — Set up your Oryn environment and build your first kernel
- **[Installation & Setup](Installation-Setup)** — Detailed system requirements and configuration
- **[Core Concepts](Core-Concepts)** — Essential terminology and architecture overview

## Architecture & Design

- **[System Architecture](System-Architecture)** — Kernel structure, boot flow, and component overview
- **[Boot Process](Boot-Process)** — From UEFI firmware through kernel entry
- **[Memory Management](Memory-Management)** — Physical and virtual memory, allocation, paging
- **[Interrupt & Timer Handling](Interrupt-Timer-Handling)** — IRQs, APIC, x2APIC, and HPET

## Kernel Modules & Subsystems

- **[Kernel Modules Overview](Kernel-Modules)** — Available modules and initialization order
- **[Console System](Console-System)** — Display, framebuffer, VGA, and TTF support
- **[CPU Detection & Features](CPU-Detection)** — CPUID, feature detection, multiprocessing
- **[Serial I/O](Serial-IO)** — Low-level serial communication and debugging
- **[Process & Scheduling](Process-Scheduling)** — Thread structures and kernel scheduling foundations

## Build & Development

- **[Build System](Build-System)** — Understanding the build pipeline and scripts
- **[Project Structure](Project-Structure)** — SDK layout and where to add code
- **[Targets & Platforms](Targets-Platforms)** — Available targets and architecture support
- **[Debugging & Testing](Debugging-Testing)** — QEMU, serial logs, matrix testing, and profiling

## Runtime & Libraries

- **[Freestanding C Runtime](Freestanding-C-Runtime)** — OrynLibC and function manifests
- **[Kernel ABI](Kernel-ABI)** — Get/Set/Event model and syscall interface
- **[Module Manifests](Module-Manifests)** — Declaring dependencies and prerequisites

## Reference

- **[Command Reference](Command-Reference)** — Oryn.sh commands and build options
- **[Configuration Options](Configuration-Options)** — Build flags and kernel tuning
- **[File Format Specifications](File-Format-Specs)** — BootInfo, ELF, FAT32 layouts
- **[Glossary](Glossary)** — Technical terms and abbreviations

## Contributing

- **[Contributing Guide](Contributing-Guide)** — How to contribute to Oryn SDK
- **[Code Style & Standards](Code-Style)** — Coding conventions and review guidelines
- **[Roadmap & TODO](Roadmap)** — Future directions and active work items

---

**Documentation Status:** Main branch (Kernel-5 active)  
**Last Updated:** 2026-07-01
