#ifndef ORYN_KERNEL_PIC_H
#define ORYN_KERNEL_PIC_H

#define ORYN_PIC_MASTER_VECTOR_OFFSET 0x20U
#define ORYN_PIC_SLAVE_VECTOR_OFFSET 0x28U

typedef struct OrynKernelPicState
{
    unsigned int Initialized;
    unsigned int Remapped;
    unsigned int Disabled;
    unsigned int MasterOffset;
    unsigned int SlaveOffset;
    unsigned char OriginalMasterMask;
    unsigned char OriginalSlaveMask;
    unsigned char CurrentMasterMask;
    unsigned char CurrentSlaveMask;
} OrynKernelPicState;

int OrynKernelPicInitAndDisable(void);
const OrynKernelPicState* OrynKernelPicGetState(void);
void OrynKernelPicSendEoi(unsigned int irq);
void OrynKernelPicPrintProof(void);

#endif
