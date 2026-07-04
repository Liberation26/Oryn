#include "KernelIdt.h"
#include "KernelGdt.h"
#include "KernelDiagnosticsLogger.h"
#include "KernelModuleManifest.h"
#include "KernelInterrupts.h"
#include "KernelScreenReport.h"

#define ORYN_IDT_PRESENT 0x80U
#define ORYN_IDT_TYPE_INTERRUPT 0x0EU
#define ORYN_IDT_DPL_USER 0x60U
#define ORYN_IDT_LINUX_SYSCALL_VECTOR 0x80U
#define ORYN_IDT_MS_SYSCALL_VECTOR 0x81U
#define ORYN_IDT_DEFAULT_IST 0U
#define ORYN_IDT_EXCEPTION_IST_MASK 0x07U
#define ORYN_IDT_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]

typedef struct OrynIdtEntry
{
    unsigned short OffsetLow;
    unsigned short Selector;
    unsigned char Ist;
    unsigned char TypeAttributes;
    unsigned short OffsetMiddle;
    unsigned int OffsetHigh;
    unsigned int Reserved;
} __attribute__((packed)) OrynIdtEntry;

typedef struct OrynIdtPointer
{
    unsigned short Limit;
    unsigned long long Base;
} __attribute__((packed)) OrynIdtPointer;

ORYN_IDT_ASSERT(OrynIdtEntrySizeMustBe16, sizeof(OrynIdtEntry) == 16U);
ORYN_IDT_ASSERT(OrynIdtPointerSizeMustBe10, sizeof(OrynIdtPointer) == 10U);

#define ORYN_DECLARE_IDT_STUB(n) extern void OrynIdtStub##n(void)

ORYN_DECLARE_IDT_STUB(0); ORYN_DECLARE_IDT_STUB(1); ORYN_DECLARE_IDT_STUB(2); ORYN_DECLARE_IDT_STUB(3);
ORYN_DECLARE_IDT_STUB(4); ORYN_DECLARE_IDT_STUB(5); ORYN_DECLARE_IDT_STUB(6); ORYN_DECLARE_IDT_STUB(7);
ORYN_DECLARE_IDT_STUB(8); ORYN_DECLARE_IDT_STUB(9); ORYN_DECLARE_IDT_STUB(10); ORYN_DECLARE_IDT_STUB(11);
ORYN_DECLARE_IDT_STUB(12); ORYN_DECLARE_IDT_STUB(13); ORYN_DECLARE_IDT_STUB(14); ORYN_DECLARE_IDT_STUB(15);
ORYN_DECLARE_IDT_STUB(16); ORYN_DECLARE_IDT_STUB(17); ORYN_DECLARE_IDT_STUB(18); ORYN_DECLARE_IDT_STUB(19);
ORYN_DECLARE_IDT_STUB(20); ORYN_DECLARE_IDT_STUB(21); ORYN_DECLARE_IDT_STUB(22); ORYN_DECLARE_IDT_STUB(23);
ORYN_DECLARE_IDT_STUB(24); ORYN_DECLARE_IDT_STUB(25); ORYN_DECLARE_IDT_STUB(26); ORYN_DECLARE_IDT_STUB(27);
ORYN_DECLARE_IDT_STUB(28); ORYN_DECLARE_IDT_STUB(29); ORYN_DECLARE_IDT_STUB(30); ORYN_DECLARE_IDT_STUB(31);
ORYN_DECLARE_IDT_STUB(32); ORYN_DECLARE_IDT_STUB(33); ORYN_DECLARE_IDT_STUB(34); ORYN_DECLARE_IDT_STUB(35);
ORYN_DECLARE_IDT_STUB(36); ORYN_DECLARE_IDT_STUB(37); ORYN_DECLARE_IDT_STUB(38); ORYN_DECLARE_IDT_STUB(39);
ORYN_DECLARE_IDT_STUB(40); ORYN_DECLARE_IDT_STUB(41); ORYN_DECLARE_IDT_STUB(42); ORYN_DECLARE_IDT_STUB(43);
ORYN_DECLARE_IDT_STUB(44); ORYN_DECLARE_IDT_STUB(45); ORYN_DECLARE_IDT_STUB(46); ORYN_DECLARE_IDT_STUB(47);
ORYN_DECLARE_IDT_STUB(48); ORYN_DECLARE_IDT_STUB(49); ORYN_DECLARE_IDT_STUB(50); ORYN_DECLARE_IDT_STUB(51);
ORYN_DECLARE_IDT_STUB(52); ORYN_DECLARE_IDT_STUB(53); ORYN_DECLARE_IDT_STUB(54); ORYN_DECLARE_IDT_STUB(55);
ORYN_DECLARE_IDT_STUB(56); ORYN_DECLARE_IDT_STUB(57); ORYN_DECLARE_IDT_STUB(58); ORYN_DECLARE_IDT_STUB(59);
ORYN_DECLARE_IDT_STUB(60); ORYN_DECLARE_IDT_STUB(61); ORYN_DECLARE_IDT_STUB(62); ORYN_DECLARE_IDT_STUB(63);
ORYN_DECLARE_IDT_STUB(64); ORYN_DECLARE_IDT_STUB(65); ORYN_DECLARE_IDT_STUB(66); ORYN_DECLARE_IDT_STUB(67);
ORYN_DECLARE_IDT_STUB(68); ORYN_DECLARE_IDT_STUB(69); ORYN_DECLARE_IDT_STUB(70); ORYN_DECLARE_IDT_STUB(71);
ORYN_DECLARE_IDT_STUB(72); ORYN_DECLARE_IDT_STUB(73); ORYN_DECLARE_IDT_STUB(74); ORYN_DECLARE_IDT_STUB(75);
ORYN_DECLARE_IDT_STUB(76); ORYN_DECLARE_IDT_STUB(77); ORYN_DECLARE_IDT_STUB(78); ORYN_DECLARE_IDT_STUB(79);
ORYN_DECLARE_IDT_STUB(80); ORYN_DECLARE_IDT_STUB(81); ORYN_DECLARE_IDT_STUB(82); ORYN_DECLARE_IDT_STUB(83);
ORYN_DECLARE_IDT_STUB(84); ORYN_DECLARE_IDT_STUB(85); ORYN_DECLARE_IDT_STUB(86); ORYN_DECLARE_IDT_STUB(87);
ORYN_DECLARE_IDT_STUB(88); ORYN_DECLARE_IDT_STUB(89); ORYN_DECLARE_IDT_STUB(90); ORYN_DECLARE_IDT_STUB(91);
ORYN_DECLARE_IDT_STUB(92); ORYN_DECLARE_IDT_STUB(93); ORYN_DECLARE_IDT_STUB(94); ORYN_DECLARE_IDT_STUB(95);
ORYN_DECLARE_IDT_STUB(96); ORYN_DECLARE_IDT_STUB(97); ORYN_DECLARE_IDT_STUB(98); ORYN_DECLARE_IDT_STUB(99);
ORYN_DECLARE_IDT_STUB(100); ORYN_DECLARE_IDT_STUB(101); ORYN_DECLARE_IDT_STUB(102); ORYN_DECLARE_IDT_STUB(103);
ORYN_DECLARE_IDT_STUB(104); ORYN_DECLARE_IDT_STUB(105); ORYN_DECLARE_IDT_STUB(106); ORYN_DECLARE_IDT_STUB(107);
ORYN_DECLARE_IDT_STUB(108); ORYN_DECLARE_IDT_STUB(109); ORYN_DECLARE_IDT_STUB(110); ORYN_DECLARE_IDT_STUB(111);
ORYN_DECLARE_IDT_STUB(112); ORYN_DECLARE_IDT_STUB(113); ORYN_DECLARE_IDT_STUB(114); ORYN_DECLARE_IDT_STUB(115);
ORYN_DECLARE_IDT_STUB(116); ORYN_DECLARE_IDT_STUB(117); ORYN_DECLARE_IDT_STUB(118); ORYN_DECLARE_IDT_STUB(119);
ORYN_DECLARE_IDT_STUB(120); ORYN_DECLARE_IDT_STUB(121); ORYN_DECLARE_IDT_STUB(122); ORYN_DECLARE_IDT_STUB(123);
ORYN_DECLARE_IDT_STUB(124); ORYN_DECLARE_IDT_STUB(125); ORYN_DECLARE_IDT_STUB(126); ORYN_DECLARE_IDT_STUB(127);
ORYN_DECLARE_IDT_STUB(128); ORYN_DECLARE_IDT_STUB(129); ORYN_DECLARE_IDT_STUB(130); ORYN_DECLARE_IDT_STUB(131);
ORYN_DECLARE_IDT_STUB(132); ORYN_DECLARE_IDT_STUB(133); ORYN_DECLARE_IDT_STUB(134); ORYN_DECLARE_IDT_STUB(135);
ORYN_DECLARE_IDT_STUB(136); ORYN_DECLARE_IDT_STUB(137); ORYN_DECLARE_IDT_STUB(138); ORYN_DECLARE_IDT_STUB(139);
ORYN_DECLARE_IDT_STUB(140); ORYN_DECLARE_IDT_STUB(141); ORYN_DECLARE_IDT_STUB(142); ORYN_DECLARE_IDT_STUB(143);
ORYN_DECLARE_IDT_STUB(144); ORYN_DECLARE_IDT_STUB(145); ORYN_DECLARE_IDT_STUB(146); ORYN_DECLARE_IDT_STUB(147);
ORYN_DECLARE_IDT_STUB(148); ORYN_DECLARE_IDT_STUB(149); ORYN_DECLARE_IDT_STUB(150); ORYN_DECLARE_IDT_STUB(151);
ORYN_DECLARE_IDT_STUB(152); ORYN_DECLARE_IDT_STUB(153); ORYN_DECLARE_IDT_STUB(154); ORYN_DECLARE_IDT_STUB(155);
ORYN_DECLARE_IDT_STUB(156); ORYN_DECLARE_IDT_STUB(157); ORYN_DECLARE_IDT_STUB(158); ORYN_DECLARE_IDT_STUB(159);
ORYN_DECLARE_IDT_STUB(160); ORYN_DECLARE_IDT_STUB(161); ORYN_DECLARE_IDT_STUB(162); ORYN_DECLARE_IDT_STUB(163);
ORYN_DECLARE_IDT_STUB(164); ORYN_DECLARE_IDT_STUB(165); ORYN_DECLARE_IDT_STUB(166); ORYN_DECLARE_IDT_STUB(167);
ORYN_DECLARE_IDT_STUB(168); ORYN_DECLARE_IDT_STUB(169); ORYN_DECLARE_IDT_STUB(170); ORYN_DECLARE_IDT_STUB(171);
ORYN_DECLARE_IDT_STUB(172); ORYN_DECLARE_IDT_STUB(173); ORYN_DECLARE_IDT_STUB(174); ORYN_DECLARE_IDT_STUB(175);
ORYN_DECLARE_IDT_STUB(176); ORYN_DECLARE_IDT_STUB(177); ORYN_DECLARE_IDT_STUB(178); ORYN_DECLARE_IDT_STUB(179);
ORYN_DECLARE_IDT_STUB(180); ORYN_DECLARE_IDT_STUB(181); ORYN_DECLARE_IDT_STUB(182); ORYN_DECLARE_IDT_STUB(183);
ORYN_DECLARE_IDT_STUB(184); ORYN_DECLARE_IDT_STUB(185); ORYN_DECLARE_IDT_STUB(186); ORYN_DECLARE_IDT_STUB(187);
ORYN_DECLARE_IDT_STUB(188); ORYN_DECLARE_IDT_STUB(189); ORYN_DECLARE_IDT_STUB(190); ORYN_DECLARE_IDT_STUB(191);
ORYN_DECLARE_IDT_STUB(192); ORYN_DECLARE_IDT_STUB(193); ORYN_DECLARE_IDT_STUB(194); ORYN_DECLARE_IDT_STUB(195);
ORYN_DECLARE_IDT_STUB(196); ORYN_DECLARE_IDT_STUB(197); ORYN_DECLARE_IDT_STUB(198); ORYN_DECLARE_IDT_STUB(199);
ORYN_DECLARE_IDT_STUB(200); ORYN_DECLARE_IDT_STUB(201); ORYN_DECLARE_IDT_STUB(202); ORYN_DECLARE_IDT_STUB(203);
ORYN_DECLARE_IDT_STUB(204); ORYN_DECLARE_IDT_STUB(205); ORYN_DECLARE_IDT_STUB(206); ORYN_DECLARE_IDT_STUB(207);
ORYN_DECLARE_IDT_STUB(208); ORYN_DECLARE_IDT_STUB(209); ORYN_DECLARE_IDT_STUB(210); ORYN_DECLARE_IDT_STUB(211);
ORYN_DECLARE_IDT_STUB(212); ORYN_DECLARE_IDT_STUB(213); ORYN_DECLARE_IDT_STUB(214); ORYN_DECLARE_IDT_STUB(215);
ORYN_DECLARE_IDT_STUB(216); ORYN_DECLARE_IDT_STUB(217); ORYN_DECLARE_IDT_STUB(218); ORYN_DECLARE_IDT_STUB(219);
ORYN_DECLARE_IDT_STUB(220); ORYN_DECLARE_IDT_STUB(221); ORYN_DECLARE_IDT_STUB(222); ORYN_DECLARE_IDT_STUB(223);
ORYN_DECLARE_IDT_STUB(224); ORYN_DECLARE_IDT_STUB(225); ORYN_DECLARE_IDT_STUB(226); ORYN_DECLARE_IDT_STUB(227);
ORYN_DECLARE_IDT_STUB(228); ORYN_DECLARE_IDT_STUB(229); ORYN_DECLARE_IDT_STUB(230); ORYN_DECLARE_IDT_STUB(231);
ORYN_DECLARE_IDT_STUB(232); ORYN_DECLARE_IDT_STUB(233); ORYN_DECLARE_IDT_STUB(234); ORYN_DECLARE_IDT_STUB(235);
ORYN_DECLARE_IDT_STUB(236); ORYN_DECLARE_IDT_STUB(237); ORYN_DECLARE_IDT_STUB(238); ORYN_DECLARE_IDT_STUB(239);
ORYN_DECLARE_IDT_STUB(240); ORYN_DECLARE_IDT_STUB(241); ORYN_DECLARE_IDT_STUB(242); ORYN_DECLARE_IDT_STUB(243);
ORYN_DECLARE_IDT_STUB(244); ORYN_DECLARE_IDT_STUB(245); ORYN_DECLARE_IDT_STUB(246); ORYN_DECLARE_IDT_STUB(247);
ORYN_DECLARE_IDT_STUB(248); ORYN_DECLARE_IDT_STUB(249); ORYN_DECLARE_IDT_STUB(250); ORYN_DECLARE_IDT_STUB(251);
ORYN_DECLARE_IDT_STUB(252); ORYN_DECLARE_IDT_STUB(253); ORYN_DECLARE_IDT_STUB(254); ORYN_DECLARE_IDT_STUB(255);

static void (*const gIdtStubs[ORYN_IDT_ENTRY_COUNT])(void) = {
    OrynIdtStub0, OrynIdtStub1, OrynIdtStub2, OrynIdtStub3, OrynIdtStub4, OrynIdtStub5, OrynIdtStub6, OrynIdtStub7,
    OrynIdtStub8, OrynIdtStub9, OrynIdtStub10, OrynIdtStub11, OrynIdtStub12, OrynIdtStub13, OrynIdtStub14, OrynIdtStub15,
    OrynIdtStub16, OrynIdtStub17, OrynIdtStub18, OrynIdtStub19, OrynIdtStub20, OrynIdtStub21, OrynIdtStub22, OrynIdtStub23,
    OrynIdtStub24, OrynIdtStub25, OrynIdtStub26, OrynIdtStub27, OrynIdtStub28, OrynIdtStub29, OrynIdtStub30, OrynIdtStub31,
    OrynIdtStub32, OrynIdtStub33, OrynIdtStub34, OrynIdtStub35, OrynIdtStub36, OrynIdtStub37, OrynIdtStub38, OrynIdtStub39,
    OrynIdtStub40, OrynIdtStub41, OrynIdtStub42, OrynIdtStub43, OrynIdtStub44, OrynIdtStub45, OrynIdtStub46, OrynIdtStub47,
    OrynIdtStub48, OrynIdtStub49, OrynIdtStub50, OrynIdtStub51, OrynIdtStub52, OrynIdtStub53, OrynIdtStub54, OrynIdtStub55,
    OrynIdtStub56, OrynIdtStub57, OrynIdtStub58, OrynIdtStub59, OrynIdtStub60, OrynIdtStub61, OrynIdtStub62, OrynIdtStub63,
    OrynIdtStub64, OrynIdtStub65, OrynIdtStub66, OrynIdtStub67, OrynIdtStub68, OrynIdtStub69, OrynIdtStub70, OrynIdtStub71,
    OrynIdtStub72, OrynIdtStub73, OrynIdtStub74, OrynIdtStub75, OrynIdtStub76, OrynIdtStub77, OrynIdtStub78, OrynIdtStub79,
    OrynIdtStub80, OrynIdtStub81, OrynIdtStub82, OrynIdtStub83, OrynIdtStub84, OrynIdtStub85, OrynIdtStub86, OrynIdtStub87,
    OrynIdtStub88, OrynIdtStub89, OrynIdtStub90, OrynIdtStub91, OrynIdtStub92, OrynIdtStub93, OrynIdtStub94, OrynIdtStub95,
    OrynIdtStub96, OrynIdtStub97, OrynIdtStub98, OrynIdtStub99, OrynIdtStub100, OrynIdtStub101, OrynIdtStub102, OrynIdtStub103,
    OrynIdtStub104, OrynIdtStub105, OrynIdtStub106, OrynIdtStub107, OrynIdtStub108, OrynIdtStub109, OrynIdtStub110, OrynIdtStub111,
    OrynIdtStub112, OrynIdtStub113, OrynIdtStub114, OrynIdtStub115, OrynIdtStub116, OrynIdtStub117, OrynIdtStub118, OrynIdtStub119,
    OrynIdtStub120, OrynIdtStub121, OrynIdtStub122, OrynIdtStub123, OrynIdtStub124, OrynIdtStub125, OrynIdtStub126, OrynIdtStub127,
    OrynIdtStub128, OrynIdtStub129, OrynIdtStub130, OrynIdtStub131, OrynIdtStub132, OrynIdtStub133, OrynIdtStub134, OrynIdtStub135,
    OrynIdtStub136, OrynIdtStub137, OrynIdtStub138, OrynIdtStub139, OrynIdtStub140, OrynIdtStub141, OrynIdtStub142, OrynIdtStub143,
    OrynIdtStub144, OrynIdtStub145, OrynIdtStub146, OrynIdtStub147, OrynIdtStub148, OrynIdtStub149, OrynIdtStub150, OrynIdtStub151,
    OrynIdtStub152, OrynIdtStub153, OrynIdtStub154, OrynIdtStub155, OrynIdtStub156, OrynIdtStub157, OrynIdtStub158, OrynIdtStub159,
    OrynIdtStub160, OrynIdtStub161, OrynIdtStub162, OrynIdtStub163, OrynIdtStub164, OrynIdtStub165, OrynIdtStub166, OrynIdtStub167,
    OrynIdtStub168, OrynIdtStub169, OrynIdtStub170, OrynIdtStub171, OrynIdtStub172, OrynIdtStub173, OrynIdtStub174, OrynIdtStub175,
    OrynIdtStub176, OrynIdtStub177, OrynIdtStub178, OrynIdtStub179, OrynIdtStub180, OrynIdtStub181, OrynIdtStub182, OrynIdtStub183,
    OrynIdtStub184, OrynIdtStub185, OrynIdtStub186, OrynIdtStub187, OrynIdtStub188, OrynIdtStub189, OrynIdtStub190, OrynIdtStub191,
    OrynIdtStub192, OrynIdtStub193, OrynIdtStub194, OrynIdtStub195, OrynIdtStub196, OrynIdtStub197, OrynIdtStub198, OrynIdtStub199,
    OrynIdtStub200, OrynIdtStub201, OrynIdtStub202, OrynIdtStub203, OrynIdtStub204, OrynIdtStub205, OrynIdtStub206, OrynIdtStub207,
    OrynIdtStub208, OrynIdtStub209, OrynIdtStub210, OrynIdtStub211, OrynIdtStub212, OrynIdtStub213, OrynIdtStub214, OrynIdtStub215,
    OrynIdtStub216, OrynIdtStub217, OrynIdtStub218, OrynIdtStub219, OrynIdtStub220, OrynIdtStub221, OrynIdtStub222, OrynIdtStub223,
    OrynIdtStub224, OrynIdtStub225, OrynIdtStub226, OrynIdtStub227, OrynIdtStub228, OrynIdtStub229, OrynIdtStub230, OrynIdtStub231,
    OrynIdtStub232, OrynIdtStub233, OrynIdtStub234, OrynIdtStub235, OrynIdtStub236, OrynIdtStub237, OrynIdtStub238, OrynIdtStub239,
    OrynIdtStub240, OrynIdtStub241, OrynIdtStub242, OrynIdtStub243, OrynIdtStub244, OrynIdtStub245, OrynIdtStub246, OrynIdtStub247,
    OrynIdtStub248, OrynIdtStub249, OrynIdtStub250, OrynIdtStub251, OrynIdtStub252, OrynIdtStub253, OrynIdtStub254, OrynIdtStub255
};

static OrynIdtEntry gIdt[ORYN_IDT_ENTRY_COUNT] __attribute__((aligned(16)));
static OrynKernelIdtState gIdtState;

static void ClearBytes(void* target, unsigned long long count)
{
    unsigned char* bytes = (unsigned char*)target;
    for (unsigned long long index = 0; index < count; ++index)
    {
        bytes[index] = 0U;
    }
}

static void LoadIdt(const OrynIdtPointer* pointer)
{
    __asm__ volatile ("lidt (%0)" :: "r"(pointer) : "memory");
}

static void StoreIdt(OrynIdtPointer* pointer)
{
    __asm__ volatile ("sidt %0" : "=m"(*pointer));
}

static void SetGate(
    unsigned int vector,
    void (*handler)(void),
    unsigned short selector,
    unsigned char ist)
{
    unsigned long long address = (unsigned long long)handler;
    gIdt[vector].OffsetLow = (unsigned short)(address & 0xFFFFULL);
    gIdt[vector].Selector = selector;
    gIdt[vector].Ist = ist & ORYN_IDT_EXCEPTION_IST_MASK;
    gIdt[vector].TypeAttributes = ORYN_IDT_PRESENT | ORYN_IDT_TYPE_INTERRUPT;
    if (vector == ORYN_IDT_LINUX_SYSCALL_VECTOR || vector == ORYN_IDT_MS_SYSCALL_VECTOR)
    {
        gIdt[vector].TypeAttributes |= ORYN_IDT_DPL_USER;
    }
    gIdt[vector].OffsetMiddle = (unsigned short)((address >> 16) & 0xFFFFULL);
    gIdt[vector].OffsetHigh = (unsigned int)((address >> 32) & 0xFFFFFFFFULL);
    gIdt[vector].Reserved = 0U;
}

const OrynKernelIdtState* OrynKernelIdtGetState(void)
{
    return &gIdtState;
}

int OrynKernelIdtInit(void)
{
    OrynIdtPointer pointer;
    OrynIdtPointer loaded;
    unsigned short selector;
    unsigned char exceptionIst;

    if (!OrynKernelModuleManifestBegin(OrynKernelModuleIdt))
    {
        return 0;
    }

    OrynKernelDiagnosticsLogText("[KERNEL] IDT: installing\n");
    ClearBytes(gIdt, sizeof(gIdt));
    ClearBytes(&gIdtState, sizeof(gIdtState));

    selector = OrynKernelGdtCodeSelector();
    exceptionIst = OrynKernelGdtExceptionIstIndex();
    for (unsigned int vector = 0; vector < ORYN_IDT_ENTRY_COUNT; ++vector)
    {
        unsigned char ist = vector < ORYN_IDT_EXCEPTION_COUNT ? exceptionIst : ORYN_IDT_DEFAULT_IST;
        SetGate(vector, gIdtStubs[vector], selector, ist);
    }

    pointer.Limit = (unsigned short)(sizeof(gIdt) - 1U);
    pointer.Base = (unsigned long long)gIdt;
    LoadIdt(&pointer);
    StoreIdt(&loaded);

    gIdtState.Installed = 1U;
    gIdtState.EntryCount = ORYN_IDT_ENTRY_COUNT;
    gIdtState.CodeSelector = selector;
    gIdtState.ExceptionIst = exceptionIst;
    gIdtState.Limit = pointer.Limit;
    gIdtState.Base = pointer.Base;
    gIdtState.LoadedBase = loaded.Base;
    gIdtState.LoadedLimit = loaded.Limit;
    gIdtState.Verified = (loaded.Base == pointer.Base && loaded.Limit == pointer.Limit) ? 1U : 0U;
    if (gIdtState.Verified)
    {
        OrynKernelModuleManifestReady(OrynKernelModuleIdt);
    }
    else
    {
        OrynKernelModuleManifestFailed(OrynKernelModuleIdt);
    }
    return gIdtState.Verified ? 1 : 0;
}

void OrynKernelIdtPrintProof(void)
{
    if (gIdtState.Verified)
    {
        OrynKernelScreenReportOk(0, "IDT installed.");
    }
    else
    {
        OrynKernelScreenReportFail(0, "IDT install verification failed.");
    }

    OrynKernelDiagnosticsLogText("[KERNEL] IDT entries: ");
    OrynKernelDiagnosticsLogDec64(gIdtState.EntryCount);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] IDT base: ");
    OrynKernelDiagnosticsLogHex64(gIdtState.Base);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] IDT limit: ");
    OrynKernelDiagnosticsLogHex64(gIdtState.Limit);
    OrynKernelDiagnosticsLogText("\n");
    OrynKernelDiagnosticsLogText("[KERNEL] IDT code selector: ");
    OrynKernelDiagnosticsLogHex64(gIdtState.CodeSelector);
    OrynKernelDiagnosticsLogText("\n");
}

void OrynIdtDispatch(OrynIdtInterruptFrame* frame)
{
    OrynKernelInterruptsDispatch(frame);
}

