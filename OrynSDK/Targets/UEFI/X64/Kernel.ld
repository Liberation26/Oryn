ENTRY(KernelStart)

ORYN_KERNEL_PHYSICAL_BASE = DEFINED(ORYN_KERNEL_PHYSICAL_BASE) ? ORYN_KERNEL_PHYSICAL_BASE : 0x4000000;
ORYN_KERNEL_VIRTUAL_BASE = DEFINED(ORYN_KERNEL_VIRTUAL_BASE) ? ORYN_KERNEL_VIRTUAL_BASE : 0xFFFFFFFF80000000;

PHDRS
{
    kernel PT_LOAD FLAGS(7);
}

SECTIONS
{
    . = ORYN_KERNEL_VIRTUAL_BASE;
    __oryn_kernel_virtual_base = .;
    __oryn_kernel_physical_base = ORYN_KERNEL_PHYSICAL_BASE;

    .text ALIGN(4K) : AT(ORYN_KERNEL_PHYSICAL_BASE + (ADDR(.text) - ORYN_KERNEL_VIRTUAL_BASE))
    {
        *(.text*)
    } :kernel

    .rodata ALIGN(4K) : AT(ORYN_KERNEL_PHYSICAL_BASE + (ADDR(.rodata) - ORYN_KERNEL_VIRTUAL_BASE))
    {
        *(.rodata*)
    } :kernel

    .data ALIGN(4K) : AT(ORYN_KERNEL_PHYSICAL_BASE + (ADDR(.data) - ORYN_KERNEL_VIRTUAL_BASE))
    {
        *(.data*)
    } :kernel

    .bss ALIGN(4K) : AT(ORYN_KERNEL_PHYSICAL_BASE + (ADDR(.bss) - ORYN_KERNEL_VIRTUAL_BASE))
    {
        *(COMMON)
        *(.bss*)
    } :kernel

    . = ALIGN(4K);
    __oryn_kernel_virtual_end = .;
    __oryn_kernel_physical_end = ORYN_KERNEL_PHYSICAL_BASE + (__oryn_kernel_virtual_end - __oryn_kernel_virtual_base);

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment*)
        *(.note*)
    }
}
