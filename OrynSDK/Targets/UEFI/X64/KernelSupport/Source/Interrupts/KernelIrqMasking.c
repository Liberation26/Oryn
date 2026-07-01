#include "KernelInterrupts.h"
#include "KernelInterruptLock.h"
#include "KernelPic.h"
#include "KernelScreenReport.h"

#define ORYN_KERNEL_IRQ_MASK_LIMIT 256U

typedef struct OrynKernelIrqMaskApiState
{
    unsigned int Initialized;
    unsigned int LastIrq;
    unsigned int Masked[ORYN_KERNEL_IRQ_MASK_LIMIT];
    unsigned int MaskOperations;
    unsigned int UnmaskOperations;
    unsigned int RejectedOperations;
} OrynKernelIrqMaskApiState;

static OrynKernelIrqMaskApiState gIrqMaskState;
static OrynKernelInterruptLock gIrqMaskLock;

static void EnsureIrqMaskApi(void)
{
    if (gIrqMaskState.Initialized == 0U)
    {
        OrynKernelInterruptLockInit(&gIrqMaskLock);
        gIrqMaskState.Initialized = 1U;
    }
}

int OrynKernelInterruptsSetIrqMasked(unsigned int irq, unsigned int masked)
{
    EnsureIrqMaskApi();
    if (irq >= ORYN_KERNEL_IRQ_MASK_LIMIT)
    {
        gIrqMaskState.RejectedOperations += 1U;
        return 0;
    }
    OrynKernelInterruptLockAcquire(&gIrqMaskLock);
    gIrqMaskState.LastIrq = irq;
    gIrqMaskState.Masked[irq] = masked ? 1U : 0U;
    if (irq < 16U)
    {
        OrynKernelPicSetIrqMask(irq, masked ? 1 : 0);
    }
    if (masked)
    {
        gIrqMaskState.MaskOperations += 1U;
    }
    else
    {
        gIrqMaskState.UnmaskOperations += 1U;
    }
    OrynKernelInterruptLockRelease(&gIrqMaskLock);
    return 1;
}

int OrynKernelInterruptsMaskIrq(unsigned int irq)
{
    return OrynKernelInterruptsSetIrqMasked(irq, 1U);
}

int OrynKernelInterruptsUnmaskIrq(unsigned int irq)
{
    return OrynKernelInterruptsSetIrqMasked(irq, 0U);
}

void OrynKernelInterruptsPrintIrqMaskProof(void)
{
    EnsureIrqMaskApi();
    (void)OrynKernelInterruptsMaskIrq(1U);
    (void)OrynKernelInterruptsUnmaskIrq(1U);
    (void)OrynKernelInterruptsMaskIrq(ORYN_KERNEL_IRQ_MASK_LIMIT);
    OrynKernelScreenReportOkOrFail(gIrqMaskState.MaskOperations > 0U &&
        gIrqMaskState.UnmaskOperations > 0U && gIrqMaskState.RejectedOperations > 0U,
        "Safe IRQ masking/unmasking APIs validate IRQs and serialize mask changes.",
        "Safe IRQ masking/unmasking API proof failed.");
}
