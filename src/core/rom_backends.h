#ifndef ROM_BACKENDS_H
#define ROM_BACKENDS_H

#include <stdint.h>

enum class ROMBackend : uint8_t
{
    LRU,
    FLASH
};

#endif
