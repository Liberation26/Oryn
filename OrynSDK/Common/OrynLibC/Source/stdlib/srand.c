#include "OrynLibCRandState.h"
#include "stdlib.h"

void srand(unsigned int seed)
{
    OrynLibCRandSeed = seed ? seed : 1U;
}
