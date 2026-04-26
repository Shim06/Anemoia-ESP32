#ifndef CONTROLLERTYPES_H
#define CONTROLLERTYPES_H
#include <stdint.h>

enum ControllerType : uint8_t {
    CT_NC = 255,  // no input device, always outputs 0x00 so code operates properly when a controller is not connected.
    CT_GPIO = 0,  // custom input device
    CT_NES = 1,   // wired NES controller
    CT_SNES = 2,  // wired SNES controller
    CT_PSX = 3,   // wired playstation controller using playstation connector
    CT_UART = 4,  // button presses send over serial connection
};

#endif
