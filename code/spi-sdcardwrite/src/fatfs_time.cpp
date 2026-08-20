#include "ff.h"

extern "C" DWORD get_fattime()
{
    // Dummy timestamp: 2026-01-01 00:00:00

    return ((2026 - 1980) << 25) |
           (1 << 21) |
           (1 << 16);
}