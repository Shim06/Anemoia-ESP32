#ifndef CONFIG_MODULE_H
#define CONFIG_MODULE_H

#include "src/ControllerTypes.h"
// Controller Configuration
#define CONTROLLER_TYPE CT_GPIO

// Screen Configuration
// #define TFT_BACKLIGHT_ENABLE
#define TFT_BACKLIGHT_PIN 21
#define SCREEN_ROTATION 1 // Screen orientation: 1 or 3 (1 = landscape, 3 = landscape flipped)
#define SCREEN_SWAP_BYTES

// MicroSD card module Pins
#define SD_FREQ 80000000 // SD card SPI frequency (try lower if you have issues with SD card initialization, e.g. 4000000)
#define SD_MOSI_PIN 13
#define SD_MISO_PIN 12
#define SD_SCLK_PIN 14
#define SD_CS_PIN -1
#define SD_SPI_PORT HSPI 

// Button pins
#define A_BUTTON 19
#define B_BUTTON 4
#define LEFT_BUTTON 22
#define RIGHT_BUTTON 32
#define UP_BUTTON 33
#define DOWN_BUTTON 21
#define START_BUTTON 27
#define SELECT_BUTTON 25

// Unused NES controller pins
#define CONTROLLER_NES_CLK -1
#define CONTROLLER_NES_LATCH -1
#define CONTROLLER_NES_DATA -1

// Unused SNES controller pins
#define CONTROLLER_SNES_CLK -1
#define CONTROLLER_SNES_LATCH -1
#define CONTROLLER_SNES_DATA -1

// Unused PS1/PS2 controller pins
#define CONTROLLER_PSX_DATA -1
#define CONTROLLER_PSX_COMMAND -1
#define CONTROLLER_PSX_ATTENTION -1
#define CONTROLLER_PSX_CLK -1

// For Serial1 connection to receive button presses from a separate controller adapter device.
// Using a controller adapter allows for easier wiring and makes it possible to use bluetooth controllers.
#define CONTROLLER_UART_TX 27
#define CONTROLLER_UART_RX 22

#define DAC_PIN 1 // 0 = GPIO25, 1 = GPIO26

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial

#endif
