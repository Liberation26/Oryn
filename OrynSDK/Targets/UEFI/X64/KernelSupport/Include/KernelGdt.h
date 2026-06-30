#ifndef ORYN_KERNEL_GDT_H
#define ORYN_KERNEL_GDT_H

#define ORYN_GDT_ENTRY_COUNT 7U
#define ORYN_GDT_KERNEL_CODE_SELECTOR 0x08U
#define ORYN_GDT_KERNEL_DATA_SELECTOR 0x10U
#define ORYN_GDT_TSS_SELECTOR 0x28U
#define ORYN_GDT_EXCEPTION_IST 1U
#define ORYN_GDT_IST_STACK_SIZE 16384U

typedef struct OrynKernelGdtState
{
    unsigned int Installed;
    unsigned int Verified;
    unsigned int TssLoaded;
    unsigned int EntryCount;
    unsigned short CodeSelector;
    unsigned short DataSelector;
    unsigned short TssSelector;
    unsigned short TaskRegister;
    unsigned short Limit;
    unsigned long long Base;
    unsigned long long LoadedBase;
    unsigned short LoadedLimit;
    unsigned long long TssBase;
    unsigned int TssLimit;
    unsigned long long ExceptionIstTop;
} OrynKernelGdtState;

int OrynKernelGdtInit(void);
const OrynKernelGdtState* OrynKernelGdtGetState(void);
void OrynKernelGdtPrintProof(void);
unsigned short OrynKernelGdtCodeSelector(void);
unsigned char OrynKernelGdtExceptionIstIndex(void);

#endif
