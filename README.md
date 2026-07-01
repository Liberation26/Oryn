# Oryn SDK

[Overview](#overview) | [What Oryn Helps You Build](#what-oryn-helps-you-build) | [Languages](#languages) | [Current Status](#current-status) | [How It Works](#how-it-works) | [Repository Layout](#repository-layout) | [Commands](#commands) | [Documentation](#documentation) | [Roadmap](#roadmap)

## Overview

Oryn SDK is a freestanding operating-system development kit for people who want to write their own OS instead of building an application on top of an existing one.

The SDK provides the build system, boot path, target layout, runtime foundations, kernel modules, proof output, and repeatable VM test harness needed to move from source code to a bootable kernel image. The goal is to make low-level OS development practical, inspectable, and repeatable while still keeping the kernel real: the active target boots through UEFI x64, enters a native kernel, and then the kernel owns the machine.

Oryn is not a hosted application framework. It is not a managed runtime hidden inside an operating system. It is a native, freestanding SDK for building kernels, kernel modules, drivers, libraries, command loaders, and eventually full userland systems.

## What Oryn Helps You Build

Oryn SDK is designed to help developers create their own operating systems by providing reusable foundations that are normally painful to write from scratch every time.

It is intended to help with:

- creating a bootable OS project with a clear project structure;
- building a native kernel from normal source files;
- linking freestanding runtime and libc pieces into the kernel;
- loading the kernel through a target-specific boot path;
- passing a stable BootInfo handoff block into the kernel;
- separating shared SDK code from target-specific hardware code;
- generating FAT32 boot images with room for many files;
- running the result in QEMU for repeatable testing;
- proving boot, memory, interrupt, process, and module status through serial/debug reports;
- keeping the graphical kernel screen readable by showing only OK, WARN, or FAIL category lines;
- growing toward external commands, a file system, processes, scheduling, networking, and POSIX-style compatibility wrappers.

The long-term aim is that a developer can choose a target, choose a language, create a kernel project, add modules, compile it, boot it, test it, and then keep expanding it into a full OS without rewriting the whole toolchain each time.

## Languages

Oryn SDK separates the language used to write OS code from the target used to boot and run that code.

### Currently provided

The current OrynWsl SDK provides:

- **C**: the active supported language for kernel and SDK source code. The build path is native C, freestanding, and compiled object by object.
- **Freestanding C runtime pieces**: OrynLibC provides function-level libc units that can be linked only when needed.
- **x86_64 assembly where required**: used only for target-critical CPU entry, descriptors, interrupts, paging, and low-level transitions that cannot be expressed safely in C alone.
- **Shell scripts and native build tooling**: used to drive the SDK, build, image, run, matrix-test, and package the project from WSL.

The current SDK is intentionally focused on native C first because it gives OS developers direct control over ABI, calling convention, memory layout, object files, linker scripts, and hardware-facing code.

### Planned language support

Oryn is being shaped so additional languages can be added without changing the OS project model.

Planned language support includes:

- **C as the primary systems language** for kernels, drivers, runtime libraries, and low-level modules.
- **Assembly as a supported companion language** for architecture-specific entry points and CPU control paths.
- **C++ later**, once the freestanding runtime, constructors, destructors, exception policy, RTTI policy, and allocation model are explicit and kernel-safe.
- **Higher-level OS-facing languages later**, provided they compile down to the Oryn freestanding ABI and do not require a hidden hosted runtime.
- **POSIX-style compatibility wrappers later**, implemented over Oryn's own kernel ABI rather than replacing it.

Oryn's kernel ABI aim remains deliberately small: **Get**, **Set**, and **Event**. Other compatibility layers should translate into those concepts instead of becoming separate permanent syscall families.

## Current Status

The active project is `Kernel-5`.

The current target is a UEFI x64 PC/VM kernel built in WSL Ubuntu and booted through Windows QEMU. UEFI is used by the loader to load the kernel and collect handoff data. After handoff, the kernel must not depend on UEFI services.

The SDK currently includes foundations for:

- UEFI x64 loader and ELF64 kernel loading;
- stable versioned BootInfo ABI and checksum validation;
- higher-half or chosen virtual-address kernel layout;
- kernel-owned lifecycle, panic, shutdown, and halt paths;
- GDT, IDT, interrupt, timer, PIC, APIC, APIC2/x2APIC, and HPET profile handling;
- SMP topology discovery and AP startup preparation where the profile supports it;
- physical memory allocation, ownership records, reference counts, and DMA-safe constraints;
- virtual memory address spaces, user mapping APIs, page-fault policy, demand allocation, and copy-on-write foundation;
- kernel heap, guarded stack support, slab/object caches, and leak accounting foundations;
- process/thread structures and scheduler-ready kernel thread stacks;
- framebuffer/VGA console, screen report categories, keyboard interrupt scroll control, fonts, and text foundations;
- OrynLibC archive resolution and function-level manifests;
- package/update flow with validation intentionally removed from the normal update path.

Some areas are intentionally still foundations rather than finished operating-system subsystems. The roadmap continues through full process lifecycle integration, scheduler policy, FAT32/VFS expansion, external command loading, drivers, networking, security, and userland.

## How It Works

The intended boot flow is:

```text
UEFI firmware
  -> BOOTX64.EFI loader
      -> load ELF64 kernel from FAT32
      -> build OrynBootInfo
      -> exit UEFI boot services
      -> jump to KernelStart
          -> kernel owns the machine
          -> kernel runs, panics, shuts down, or halts itself
```

The kernel must not return to the loader. Returning from the kernel entry is treated as a failure.

The build flow is:

```text
source files
  -> per-file object compilation
  -> reusable SDK/libc object selection
  -> kernel ELF link
  -> FAT32 image staging
  -> QEMU boot/run
  -> serial/debug proof report
```

The kernel screen is for status categories only:

```text
[OK] Category
[WARN] Category
[FAIL] Category
```

Detailed proof output belongs in serial/debug logs and generated boot reports, not on the graphical kernel screen.

## Repository Layout

```text
OrynSDK/
  Common/
    Docs/                       permanent SDK documentation
    Handoff/                    target-neutral handoff ABI
    Kernel/                     shared kernel modules
    OrynBuild/                  native build tooling
    OrynLibC/                   freestanding libc source units
  Targets/
    UEFI/X64/                   UEFI x64 loader, linker, hardware support

OrynProjects/
  Kernel-5/                     active kernel project
```

Shared code belongs under `OrynSDK/Common`.

Target-specific code belongs under `OrynSDK/Targets/<Target>/<Arch>`.

Project-specific code belongs under `OrynProjects/<ProjectName>`.

## Commands

From the repository root:

```bash
./Oryn.sh doctor
./Oryn.sh rebuild
./Oryn.sh build
./Oryn.sh image
./Oryn.sh run
./Oryn.sh matrix
./Oryn.sh matrix-screen
./Oryn.sh matrix-all
./Oryn.sh gitpush
```

Common command purposes:

- `doctor` checks the toolchain and expected host tools.
- `build` compiles and links the current kernel.
- `image` stages the boot image.
- `run` boots the current project in QEMU.
- `matrix` runs headless serial/debug profile checks.
- `matrix-screen` runs graphical screen/profile checks.
- `matrix-all` runs the wider available matrix.
- `gitpush` commits and pushes the current source tree when GitHub is ready.

## Documentation

Permanent documentation is kept in the source tree so it travels with every SDK package.

Start here:

- `README.md` - permanent overview of what Oryn SDK is and where it is going.
- `OrynSDK/Common/Docs/QuickStart.txt` - setup and first-use guide.
- `OrynSDK/Common/Docs/SdkSourceLayout.txt` - source-tree and module layout guide.
- `ToDo.txt` - broad kernel requirement checklist and status.
- `OrynSDK/ToDo2.txt` - detailed Linux-like kernel roadmap and version history.
- `OrynSDK/Common/Kernel/ModuleManifests/` - kernel module manifests.
- `OrynSDK/Common/OrynLibC/FunctionManifests/` - libc function manifests.

## Roadmap

Oryn SDK is moving toward a full OS-development platform with:

- complete module manifests for every kernel and SDK item;
- stronger dependency and prerequisite selection before module startup;
- full FAT32 and VFS-backed external command loading;
- process lifecycle, fork-like creation, scheduling, and user memory integration;
- POSIX-style wrappers over the Oryn Get/Set/Event ABI;
- driver growth for storage, USB, PCI, network, display, time, and input;
- reusable project templates for multiple OS shapes;
- more target back ends after UEFI x64 is strong enough;
- language expansion after the native C ABI, runtime, and package model are stable.

The permanent direction is simple: Oryn SDK should let software developers build their own operating systems from understandable source code, prove what the kernel actually started, and keep growing from a small bootable kernel into a complete OS.

[Overview](#overview) | [What Oryn Helps You Build](#what-oryn-helps-you-build) | [Languages](#languages) | [Current Status](#current-status) | [How It Works](#how-it-works) | [Repository Layout](#repository-layout) | [Commands](#commands) | [Documentation](#documentation) | [Roadmap](#roadmap)
