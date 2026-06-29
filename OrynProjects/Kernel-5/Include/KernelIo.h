#ifndef ORYN_KERNEL_4_IO_H
#define ORYN_KERNEL_4_IO_H

void KernelIoInit(void);
void KernelIoWriteChar(char value);
void KernelIoWriteString(const char* text);
void KernelIoWriteHex64(unsigned long long value);
void KernelIoWriteDec64(unsigned long long value);
void KernelIoExitQemuSuccess(void);

#endif
