#include "KernelPortIo.h"

unsigned char OrynPortIn8(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

unsigned short OrynPortIn16(unsigned short port)
{
    unsigned short value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

unsigned int OrynPortIn32(unsigned short port)
{
    unsigned int value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void OrynPortOut8(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void OrynPortOut16(unsigned short port, unsigned short value)
{
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

void OrynPortOut32(unsigned short port, unsigned int value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

void OrynPortIoWait(void)
{
    OrynPortOut8(0x80U, 0U);
}
