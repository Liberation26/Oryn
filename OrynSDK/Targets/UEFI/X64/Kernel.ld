ENTRY(KernelStart)

PHDRS
{
    kernel PT_LOAD FLAGS(7);
}

SECTIONS
{
    . = 0x4000000;

    .text : ALIGN(4K)
    {
        *(.text*)
    } :kernel

    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    } :kernel

    .data : ALIGN(4K)
    {
        *(.data*)
    } :kernel

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss*)
    } :kernel

    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.comment*)
        *(.note*)
    }
}
