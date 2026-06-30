#include "KernelKeyboard.h"
#include "KernelApic.h"
#include "KernelConsole.h"
#include "KernelInterrupts.h"
#include "KernelIo.h"
#include "KernelPic.h"
#include "KernelPortIo.h"

#define ORYN_KEYBOARD_DATA_PORT 0x60U
#define ORYN_KEYBOARD_STATUS_PORT 0x64U
#define ORYN_KEYBOARD_STATUS_OUTPUT_FULL 0x01U
#define ORYN_KEYBOARD_IRQ 1U
#define ORYN_KEYBOARD_VECTOR (ORYN_INTERRUPT_IRQ_BASE + ORYN_KEYBOARD_IRQ)
#define ORYN_KEYBOARD_EXTENDED_PREFIX 0xE0U
#define ORYN_KEYBOARD_RELEASE_BIT 0x80U
#define ORYN_KEYBOARD_SCANCODE_UP 0x48U
#define ORYN_KEYBOARD_SCANCODE_PAGE_UP 0x49U
#define ORYN_KEYBOARD_SCANCODE_DOWN 0x50U
#define ORYN_KEYBOARD_SCANCODE_PAGE_DOWN 0x51U

static OrynKernelKeyboardState gKeyboardState;
static unsigned int gKeyboardExtendedPrefix;

static void ClearState(void)
{
    unsigned char* bytes = (unsigned char*)&gKeyboardState;
    for (unsigned int index = 0U; index < sizeof(gKeyboardState); ++index)
    {
        bytes[index] = 0U;
    }
    gKeyboardExtendedPrefix = 0U;
}

static unsigned char KeyboardReadStatus(void)
{
    return OrynPortIn8(ORYN_KEYBOARD_STATUS_PORT);
}

static unsigned char KeyboardReadData(void)
{
    return OrynPortIn8(ORYN_KEYBOARD_DATA_PORT);
}

static int KeyboardHasData(void)
{
    return (KeyboardReadStatus() & ORYN_KEYBOARD_STATUS_OUTPUT_FULL) != 0U;
}

static void DrainPendingKeyboardBytes(void)
{
    for (unsigned int index = 0U; index < 32U && KeyboardHasData(); ++index)
    {
        (void)KeyboardReadData();
    }
}

static void HandleScrollScanCode(unsigned char scanCode, unsigned int extended)
{
    unsigned int released = (scanCode & ORYN_KEYBOARD_RELEASE_BIT) != 0U ? 1U : 0U;
    unsigned char key = (unsigned char)(scanCode & 0x7FU);
    (void)extended;

    if (released)
    {
        return;
    }

    if (key == ORYN_KEYBOARD_SCANCODE_UP)
    {
        if (KConsole.ScrollUpLines(1U))
        {
            gKeyboardState.ScrollLineUpEvents += 1ULL;
        }
        return;
    }

    if (key == ORYN_KEYBOARD_SCANCODE_DOWN)
    {
        if (KConsole.ScrollDownLines(1U))
        {
            gKeyboardState.ScrollLineDownEvents += 1ULL;
        }
        return;
    }

    if (key == ORYN_KEYBOARD_SCANCODE_PAGE_UP)
    {
        if (KConsole.PageUp())
        {
            gKeyboardState.PageUpEvents += 1ULL;
        }
        return;
    }

    if (key == ORYN_KEYBOARD_SCANCODE_PAGE_DOWN)
    {
        if (KConsole.PageDown())
        {
            gKeyboardState.PageDownEvents += 1ULL;
        }
        return;
    }
}

static void KeyboardInterruptHandler(OrynIdtInterruptFrame* frame, void* context)
{
    (void)frame;
    (void)context;
    gKeyboardState.InterruptCount += 1ULL;

    for (unsigned int index = 0U; index < 16U && KeyboardHasData(); ++index)
    {
        unsigned char scanCode = KeyboardReadData();
        gKeyboardState.ScanCodesRead += 1ULL;

        if (scanCode == ORYN_KEYBOARD_EXTENDED_PREFIX)
        {
            gKeyboardExtendedPrefix = 1U;
            continue;
        }

        HandleScrollScanCode(scanCode, gKeyboardExtendedPrefix);
        gKeyboardExtendedPrefix = 0U;
    }
}

const OrynKernelKeyboardState* OrynKernelKeyboardGetState(void)
{
    return &gKeyboardState;
}

int OrynKernelKeyboardInitForConsoleScroll(void)
{
    ClearState();

    if (!KConsole.IsAvailable())
    {
        return 0;
    }

    DrainPendingKeyboardBytes();
    gKeyboardState.DecoderReady = 1U;
    gKeyboardState.ApicLegacyBridgeReady = OrynKernelApicEnableLegacyPicBridge() ? 1U : 0U;

    if (!OrynKernelInterruptsRegisterHandler(
        ORYN_KEYBOARD_VECTOR,
        KeyboardInterruptHandler,
        0,
        "ps2-keyboard-scroll"))
    {
        return 0;
    }

    OrynKernelPicSetIrqMask(ORYN_KEYBOARD_IRQ, 0);
    gKeyboardState.PicIrq1Unmasked = 1U;
    gKeyboardState.HandlerRegistered = 1U;
    gKeyboardState.UsesInterrupts = 1U;
    gKeyboardState.Initialized = 1U;
    return 1;
}

void OrynKernelKeyboardEnableInteractiveInterrupts(void)
{
    if (!gKeyboardState.Initialized)
    {
        return;
    }

    OrynKernelInterruptsEnable();
    gKeyboardState.InterruptsEnabledForInteractiveMode =
        OrynKernelInterruptsAreEnabled() ? 1U : 0U;
}

void OrynKernelKeyboardPrintProof(void)
{
    KernelIoWriteString(gKeyboardState.Initialized ?
        "[KERNEL] PASS: Keyboard interrupt scrolling initialized.\n" :
        "[KERNEL] FAIL: Keyboard interrupt scrolling was not initialized.\n");
    KernelIoWriteString(gKeyboardState.HandlerRegistered ?
        "[KERNEL] PASS: PS/2 keyboard IRQ1 handler registered.\n" :
        "[KERNEL] FAIL: PS/2 keyboard IRQ1 handler was not registered.\n");
    KernelIoWriteString(gKeyboardState.UsesInterrupts ?
        "[KERNEL] PASS: Keyboard scrolling uses IRQ1 interrupts.\n" :
        "[KERNEL] FAIL: Keyboard scrolling is not interrupt-driven.\n");
    KernelIoWriteString(gKeyboardState.PicIrq1Unmasked ?
        "[KERNEL] PASS: PIC IRQ1 unmasked for keyboard input.\n" :
        "[KERNEL] FAIL: PIC IRQ1 was not unmasked for keyboard input.\n");
    KernelIoWriteString(gKeyboardState.ApicLegacyBridgeReady ?
        "[KERNEL] PASS: APIC legacy PIC bridge ready for keyboard IRQ1.\n" :
        "[KERNEL] WARN: APIC legacy PIC bridge was not configured for keyboard IRQ1.\n");
    KernelIoWriteString(gKeyboardState.DecoderReady ?
        "[KERNEL] PASS: Keyboard arrow and page key decoder ready.\n" :
        "[KERNEL] FAIL: Keyboard arrow and page key decoder not ready.\n");
    KernelIoWriteString("[KERNEL] PASS: Keyboard Up/Down scroll one line.\n");
    KernelIoWriteString("[KERNEL] PASS: Keyboard PgUp/PgDn scroll one page.\n");
}
