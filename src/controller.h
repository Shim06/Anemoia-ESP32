#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "ControllerTypes.h"
#include <stdint.h>

enum class CONTROLLER : uint8_t
{
    A = (1 << 0),      // A Button
    B = (1 << 1),      // B Button
    Select = (1 << 2), // Select Button
    Start = (1 << 3),  // Start Button
    Up = (1 << 4),     // Up Button
    Down = (1 << 5),   // Down Button
    Left = (1 << 6),   // Left Button
    Right = (1 << 7)   // Right Button
};

void initController(ControllerType controller_type);
uint8_t controllerRead();
bool isDownPressed(CONTROLLER button);

#endif
