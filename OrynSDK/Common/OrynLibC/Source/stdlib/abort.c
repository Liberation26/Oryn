#include "stdlib.h"

void abort(void)
{
    for (;;)
    {
        __asm__ __volatile__("cli; hlt");
    }
}
