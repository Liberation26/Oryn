#ifndef ORYN_KERNEL_PORT_IO_H
#define ORYN_KERNEL_PORT_IO_H

unsigned char OrynPortIn8(unsigned short port);
unsigned short OrynPortIn16(unsigned short port);
unsigned int OrynPortIn32(unsigned short port);
void OrynPortOut8(unsigned short port, unsigned char value);
void OrynPortOut16(unsigned short port, unsigned short value);
void OrynPortOut32(unsigned short port, unsigned int value);
void OrynPortIoWait(void);

#endif
