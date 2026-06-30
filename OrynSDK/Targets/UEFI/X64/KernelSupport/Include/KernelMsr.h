#ifndef ORYN_KERNEL_MSR_H
#define ORYN_KERNEL_MSR_H

unsigned long long OrynMsrRead(unsigned int msr);
void OrynMsrWrite(unsigned int msr, unsigned long long value);

#endif
