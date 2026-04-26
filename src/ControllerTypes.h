#ifndef CONTROLLERTYPES_H
#define CONTROLLERTYPES_H
#include <stdint.h>

enum ControllerType : uint8_t {
    GP_NC = 255,  // no input device, always outputs 0x00 so code operates properly when a controller is not connected.
    GP_GPIO = 0,  // custom input device
    GP_NES = 1,   // wired NES controller
    GP_SNES = 2,  // wired SNES controller
    GP_PSX = 3,   // wired playstation controller using playstation connector
};

#endif
