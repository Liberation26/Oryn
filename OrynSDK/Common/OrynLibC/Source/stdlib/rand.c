#include "stdlib.h"

static unsigned int gRandState = 1U;

void srand(unsigned int seed)
{
    gRandState = (seed == 0U) ? 1U : seed;
}

int rand(void)
{
    gRandState = gRandState * 1103515245U + 12345U;
    return (int)((gRandState / 65536U) % ((unsigned int)RAND_MAX + 1U));
}
