#include "inttypes.h"

intmax_t imaxabs(intmax_t value)
{
    return value < 0 ? -value : value;
}
