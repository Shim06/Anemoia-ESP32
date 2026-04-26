#ifndef CONFIG_H
#define CONFIG_H

// #define CHEAP_YELLOW_DISPLAY_CONF // Uncomment this line if using the CYD
// #define MODULE_BASED_PCB_CONF // Uncomment this line if using the module PCB
// #define DISCRETE_PCB_CONF // Uncomment this line if using the discrete PCB
#if defined(CHEAP_YELLOW_DISPLAY_CONF)
  #include "config_cyd.h"
#elif defined(MODULE_BASED_PCB_CONF)
  #include "config_module.h"
#elif defined(DISCRETE_PCB_CONF)
  #include "config_discrete.h"
#else

#include "src/ControllerTypes.h"
// Controller Configuration
// see src/ControllerTypes.h for options
#define CONTROLLER_TYPE CT_GPIO

// Screen Configuration
// #define TFT_BACKLIGHT_ENABLE // Uncomment this line if using a screen with backlight pin
#define TFT_BACKLIGHT_PIN 21
#define SCREEN_ROTATION 1 // Screen orientation: 1 or 3 (1 = landscape, 3 = landscape flipped)
#define SCREEN_SWAP_BYTES // Uncomment if colors appear wrong
// #define DISABLE_DMA // Uncomment this line if using parallel communication instead of SPI communication

// MicroSD card module Pins
#define SD_FREQ 80000000 // SD card SPI frequency (try lower if you have issues with SD card initialization, e.g. 4000000)
#define SD_MOSI_PIN 13
#define SD_MISO_PIN 12
#define SD_SCLK_PIN 14
#define SD_CS_PIN -1
#define SD_SPI_PORT HSPI 
// Use VSPI or HSPI depending on your wiring.
// SD_SPI_PORT should be the opposite SPI port from the one used by the display (e.g. if using HSPI for display, use VSPI for SD card, and vice versa).
// VSPI: MOSI=23, MISO=19, SCLK=18, CS=5 
// HSPI: MOSI=13, MISO=12, SCLK=14, CS=15

// Button pins
#define A_BUTTON 19
#define B_BUTTON 26
#define LEFT_BUTTON 32
#define RIGHT_BUTTON 33
#define UP_BUTTON 15
#define DOWN_BUTTON 4
#define START_BUTTON 27
#define SELECT_BUTTON 16

// NES controller pins
#define CONTROLLER_NES_CLK 32
#define CONTROLLER_NES_LATCH 33
#define CONTROLLER_NES_DATA 35

// SNES controller pins
#define CONTROLLER_SNES_CLK 32
#define CONTROLLER_SNES_LATCH 33
#define CONTROLLER_SNES_DATA 35

// PS1/PS2 controller pins
#define CONTROLLER_PSX_DATA 32
#define CONTROLLER_PSX_COMMAND 33
#define CONTROLLER_PSX_ATTENTION 26
#define CONTROLLER_PSX_CLK 27

// For Serial1 connection to receive button presses from a separate controller adapter device.
// Using a controller adapter allows for easier wiring and makes it possible to use bluetooth controllers.
#define CONTROLLER_UART_TX 27
#define CONTROLLER_UART_RX 22

#define DAC_PIN 0 // 0 = GPIO25, 1 = GPIO26

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial


// If using an ESP32-S3
// External DAC pin configuration
#define I2S_BCLK_PIN   38   // Bit clock (BCLK)
#define I2S_LRC_PIN    39   // Word select / Left-Right clock (LRC / WS)
#define I2S_DOUT_PIN   40   // Serial data output (DIN)

#endif

#endif
