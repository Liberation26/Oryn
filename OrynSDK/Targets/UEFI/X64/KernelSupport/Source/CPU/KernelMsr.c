#include "KernelMsr.h"

unsigned long long OrynMsrRead(unsigned int msr)
{
    unsigned int low;
    unsigned int high;
    __asm__ volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((unsigned long long)high << 32) | (unsigned long long)low;
}

void OrynMsrWrite(unsigned int msr, unsigned long long value)
{
    unsigned int low = (unsigned int)(value & 0xFFFFFFFFULL);
    unsigned int high = (unsigned int)(value >> 32);
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high) : "memory");
}
