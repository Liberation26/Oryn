#include "OrynLibCRandState.h"
#include "stdlib.h"

int rand(void)
{
    OrynLibCRandSeed = OrynLibCRandSeed * 1103515245U + 12345U;
    return (int)((OrynLibCRandSeed >> 16) & RAND_MAX);
}
