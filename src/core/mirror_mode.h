#ifndef MIRROR_MODE_H
#define MIRROR_MODE_H
#include <cstdint>

enum class MIRROR : uint8_t
{
    HORIZONTAL,
    VERTICAL,
    ONESCREEN_LOW,
    ONESCREEN_HIGH,
    HARDWARE
};

#endif
