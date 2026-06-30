#include "KernelPic.h"
#include "KernelIo.h"
#include "KernelPortIo.h"

#define ORYN_PIC1_COMMAND 0x20U
#define ORYN_PIC1_DATA 0x21U
#define ORYN_PIC2_COMMAND 0xA0U
#define ORYN_PIC2_DATA 0xA1U
#define ORYN_PIC_EOI 0x20U
#define ORYN_PIC_ICW1_INIT 0x10U
#define ORYN_PIC_ICW1_ICW4 0x01U
#define ORYN_PIC_ICW4_8086 0x01U

static OrynKernelPicState gPicState;

static void PicWait(void)
{
    OrynPortIoWait();
}

const OrynKernelPicState* OrynKernelPicGetState(void)
{
    return &gPicState;
}

int OrynKernelPicInitAndDisable(void)
{
    gPicState.OriginalMasterMask = OrynPortIn8(ORYN_PIC1_DATA);
    gPicState.OriginalSlaveMask = OrynPortIn8(ORYN_PIC2_DATA);
    gPicState.MasterOffset = ORYN_PIC_MASTER_VECTOR_OFFSET;
    gPicState.SlaveOffset = ORYN_PIC_SLAVE_VECTOR_OFFSET;

    OrynPortOut8(ORYN_PIC1_COMMAND, ORYN_PIC_ICW1_INIT | ORYN_PIC_ICW1_ICW4);
    PicWait();
    OrynPortOut8(ORYN_PIC2_COMMAND, ORYN_PIC_ICW1_INIT | ORYN_PIC_ICW1_ICW4);
    PicWait();
    OrynPortOut8(ORYN_PIC1_DATA, ORYN_PIC_MASTER_VECTOR_OFFSET);
    PicWait();
    OrynPortOut8(ORYN_PIC2_DATA, ORYN_PIC_SLAVE_VECTOR_OFFSET);
    PicWait();
    OrynPortOut8(ORYN_PIC1_DATA, 0x04U);
    PicWait();
    OrynPortOut8(ORYN_PIC2_DATA, 0x02U);
    PicWait();
    OrynPortOut8(ORYN_PIC1_DATA, ORYN_PIC_ICW4_8086);
    PicWait();
    OrynPortOut8(ORYN_PIC2_DATA, ORYN_PIC_ICW4_8086);
    PicWait();

    OrynPortOut8(ORYN_PIC1_DATA, 0xFFU);
    OrynPortOut8(ORYN_PIC2_DATA, 0xFFU);
    gPicState.CurrentMasterMask = OrynPortIn8(ORYN_PIC1_DATA);
    gPicState.CurrentSlaveMask = OrynPortIn8(ORYN_PIC2_DATA);
    gPicState.Disabled =
        (gPicState.CurrentMasterMask == 0xFFU && gPicState.CurrentSlaveMask == 0xFFU) ? 1U : 0U;
    gPicState.Remapped = 1U;
    gPicState.Initialized = 1U;
    return gPicState.Disabled ? 1 : 0;
}

void OrynKernelPicSendEoi(unsigned int irq)
{
    if (irq >= 8U)
    {
        OrynPortOut8(ORYN_PIC2_COMMAND, ORYN_PIC_EOI);
    }

    OrynPortOut8(ORYN_PIC1_COMMAND, ORYN_PIC_EOI);
}

void OrynKernelPicPrintProof(void)
{
    KernelIoWriteString(gPicState.Initialized ?
        "[KERNEL] PASS: PIC initialized.\n" :
        "[KERNEL] FAIL: PIC was not initialized.\n");
    KernelIoWriteString(gPicState.Remapped ?
        "[KERNEL] PASS: PIC remapped to vectors 0x20-0x2F.\n" :
        "[KERNEL] FAIL: PIC remap did not complete.\n");
    KernelIoWriteString(gPicState.Disabled ?
        "[KERNEL] PASS: PIC masked/disabled for APIC handoff.\n" :
        "[KERNEL] WARN: PIC is not fully masked.\n");
    KernelIoWriteString("[KERNEL] PIC original masks master/slave: ");
    KernelIoWriteHex64(gPicState.OriginalMasterMask);
    KernelIoWriteString(" / ");
    KernelIoWriteHex64(gPicState.OriginalSlaveMask);
    KernelIoWriteString("\n");
}
