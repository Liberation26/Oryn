#include "KernelScreenReport.h"
#include "KernelConsole.h"

#define SCREEN_STATUS_NONE 0
#define SCREEN_STATUS_OK 1
#define SCREEN_STATUS_WARN 2
#define SCREEN_STATUS_FAIL 3

#define SCREEN_CATEGORY_COUNT 20

typedef struct OrynKernelScreenCategory
{
    const char* Name;
    int Status;
    unsigned int Seen;
} OrynKernelScreenCategory;

static OrynKernelScreenCategory gScreenCategories[SCREEN_CATEGORY_COUNT] =
{
    { "Kernel Entry", SCREEN_STATUS_NONE, 0U },
    { "BootInfo", SCREEN_STATUS_NONE, 0U },
    { "Lifecycle", SCREEN_STATUS_NONE, 0U },
    { "Panic", SCREEN_STATUS_NONE, 0U },
    { "CPU", SCREEN_STATUS_NONE, 0U },
    { "GDT", SCREEN_STATUS_NONE, 0U },
    { "IDT", SCREEN_STATUS_NONE, 0U },
    { "Interrupts", SCREEN_STATUS_NONE, 0U },
    { "PIC", SCREEN_STATUS_NONE, 0U },
    { "APIC", SCREEN_STATUS_NONE, 0U },
    { "APIC2", SCREEN_STATUS_NONE, 0U },
    { "HPET", SCREEN_STATUS_NONE, 0U },
    { "SMP", SCREEN_STATUS_NONE, 0U },
    { "PCI", SCREEN_STATUS_NONE, 0U },
    { "SysCalls", SCREEN_STATUS_NONE, 0U },
    { "Screen", SCREEN_STATUS_NONE, 0U },
    { "Keyboard", SCREEN_STATUS_NONE, 0U },
    { "Memory Map", SCREEN_STATUS_NONE, 0U },
    { "Physical Memory", SCREEN_STATUS_NONE, 0U },
    { "Virtual Memory", SCREEN_STATUS_NONE, 0U }
};

static int StartsWith(const char* text, const char* prefix)
{
    while (*prefix != 0)
    {
        if (*text != *prefix)
        {
            return 0;
        }
        ++text;
        ++prefix;
    }
    return 1;
}

static int Contains(const char* text, const char* needle)
{
    const char* cursor;
    if (text == 0 || needle == 0 || *needle == 0)
    {
        return 0;
    }

    while (*text != 0 && *text != '\n' && *text != '\r')
    {
        cursor = text;
        const char* match = needle;
        while (*cursor != 0 && *match != 0 && *cursor == *match)
        {
            ++cursor;
            ++match;
        }
        if (*match == 0)
        {
            return 1;
        }
        ++text;
    }
    return 0;
}

static int StatusForLine(const char* line)
{
    if (StartsWith(line, "[KERNEL] PASS:") || StartsWith(line, "[PASS]"))
    {
        return SCREEN_STATUS_OK;
    }
    if (StartsWith(line, "[KERNEL] WARN:") || StartsWith(line, "[WARN]"))
    {
        return SCREEN_STATUS_WARN;
    }
    if (StartsWith(line, "[KERNEL] FAIL:") || StartsWith(line, "[KERNEL] EXCEPTION") ||
        StartsWith(line, "[FAIL]"))
    {
        return SCREEN_STATUS_FAIL;
    }
    return SCREEN_STATUS_NONE;
}

static int CategoryForLine(const char* line)
{
    if (Contains(line, "Panic") || Contains(line, "panic")) return 3;
    if (Contains(line, "Lifecycle")) return 2;
    if (Contains(line, "BootInfo") || Contains(line, "handoff")) return 1;
    if (Contains(line, "Kernel entered") || Contains(line, "Kernel entry") ||
        Contains(line, "Serial/debug")) return 0;
    if (Contains(line, "CPU interrupt") || Contains(line, "CPU Interrupt")) return 7;
    if (Contains(line, "CPU")) return 4;
    if (Contains(line, "GDT")) return 5;
    if (Contains(line, "IDT")) return 6;
    if (Contains(line, "Interrupt")) return 7;
    if (Contains(line, "PIC")) return 8;
    if (Contains(line, "APIC2") || Contains(line, "x2APIC")) return 10;
    if (Contains(line, "APIC")) return 9;
    if (Contains(line, "HPET")) return 11;
    if (Contains(line, "SMP")) return 12;
    if (Contains(line, "PCI")) return 13;
    if (Contains(line, "SysCall") || Contains(line, "syscall")) return 14;
    if (Contains(line, "console") || Contains(line, "screen") || Contains(line, "TTF")) return 15;
    if (Contains(line, "Keyboard") || Contains(line, "keyboard")) return 16;
    if (Contains(line, "Memory map") || Contains(line, "MemoryMap")) return 17;
    if (Contains(line, "Physical") || Contains(line, "allocator")) return 18;
    if (Contains(line, "Virtual memory") || Contains(line, "VirtualMemory")) return 19;
    return 0;
}

static const char* StatusName(int status)
{
    if (status == SCREEN_STATUS_FAIL) return "FAIL";
    if (status == SCREEN_STATUS_WARN) return "WARN";
    return "OK";
}

static unsigned int ColourForStatus(int status)
{
    if (status == SCREEN_STATUS_FAIL) return KCONSOLE_COLOUR_FAIL;
    if (status == SCREEN_STATUS_WARN) return KCONSOLE_COLOUR_WARN;
    return KCONSOLE_COLOUR_OK;
}

static void ScreenWriteText(const char* text)
{
    while (*text != 0)
    {
        KConsole.WriteChar(*text);
        ++text;
    }
}

void OrynKernelScreenReportInit(void)
{
    for (unsigned int index = 0U; index < SCREEN_CATEGORY_COUNT; ++index)
    {
        gScreenCategories[index].Status = SCREEN_STATUS_NONE;
        gScreenCategories[index].Seen = 0U;
    }
}

void OrynKernelScreenReportObserve(const char* line)
{
    int status = StatusForLine(line);
    if (status == SCREEN_STATUS_NONE)
    {
        return;
    }

    int category = CategoryForLine(line);
    gScreenCategories[category].Seen = 1U;
    if (status > gScreenCategories[category].Status)
    {
        gScreenCategories[category].Status = status;
    }
}

void OrynKernelScreenReportWriteStatusLine(
    const char* status,
    const char* category,
    unsigned int colour)
{
    if (!KConsole.IsAvailable())
    {
        return;
    }

    KConsole.SetForegroundColour(colour);
    KConsole.WriteChar('[');
    ScreenWriteText(status);
    KConsole.WriteChar(']');
    KConsole.WriteChar(' ');
    ScreenWriteText(category);
    KConsole.WriteChar('\n');
    KConsole.ResetForegroundColour();
}

void OrynKernelScreenReportPrint(void)
{
    if (!KConsole.IsAvailable())
    {
        return;
    }

    KConsole.ClearScreen();
    for (unsigned int index = 0U; index < SCREEN_CATEGORY_COUNT; ++index)
    {
        if (gScreenCategories[index].Seen)
        {
            OrynKernelScreenReportWriteStatusLine(
                StatusName(gScreenCategories[index].Status),
                gScreenCategories[index].Name,
                ColourForStatus(gScreenCategories[index].Status));
        }
    }
    KConsole.ScrollToBottom();
}
