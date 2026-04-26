#ifndef CONFIG_CYD_H
#define CONFIG_CYD_H

#include "src/ControllerTypes.h"
// Controller Configuration
// Because of the limited pins brought out by the CYD, it is only practical to use a
// NES controller if wiring a controller directly to the board is desired.
// #define CONTROLLER_TYPE CT_NES
// #define CONTROLLER_TYPE CT_NC  // no input device, always outputs 0x00 so code operates properly when a controller is not connected.
#define CONTROLLER_TYPE CT_UART  // reads button presses over USB to serial connection or a dedicated UART https://github.com/jethomson/SerialGameControllerAdapter


// Screen Configuration
#define TFT_BACKLIGHT_ENABLE
#define TFT_BACKLIGHT_PIN 21
#define SCREEN_ROTATION 1 // Screen orientation: 1 or 3 (1 = landscape, 3 = landscape flipped)
#define SCREEN_SWAP_BYTES

// MicroSD card module Pins
#define SD_FREQ 80000000 // SD card SPI frequency (try lower if you have issues with SD card initialization, e.g. 4000000)
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_SCLK_PIN 18
#define SD_CS_PIN 5
#define SD_SPI_PORT VSPI 

// NES controller pins (CYD easily accessible GPIO pins)
#define CONTROLLER_NES_CLK 22
#define CONTROLLER_NES_LATCH 27
#define CONTROLLER_NES_DATA 35

// Unused button pins (set to -1 for CYD)
#define A_BUTTON -1
#define B_BUTTON -1
#define LEFT_BUTTON -1
#define RIGHT_BUTTON -1
#define UP_BUTTON -1
#define DOWN_BUTTON -1
#define START_BUTTON -1
#define SELECT_BUTTON -1

// Unused SNES controller pins (set to -1 for CYD)
#define CONTROLLER_SNES_CLK -1
#define CONTROLLER_SNES_LATCH -1
#define CONTROLLER_SNES_DATA -1

// Unused PS1/PS2 controller pins (set to -1 for CYD)
#define CONTROLLER_PSX_DATA -1
#define CONTROLLER_PSX_COMMAND -1
#define CONTROLLER_PSX_ATTENTION -1
#define CONTROLLER_PSX_CLK -1

// For Serial1 connection to receive button presses from a separate controller adapter device.
// Using a controller adapter allows for easier wiring and makes it possible to use bluetooth controllers.
#define CONTROLLER_UART_TX 27
#define CONTROLLER_UART_RX 22

#define DAC_PIN 1 // 0 = GPIO25, 1 = GPIO26 (CYD uses GPIO26)

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial

#endif
