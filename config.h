#ifndef CONFIG_H
#define CONFIG_H

// Controller Configuration
// 0 = CONTROLLER_GPIO, 1 = CONTROLLER_NES, 
// 2 = CONTROLLER_SNES, 3 = CONTROLLER_PSX
#define CONTROLLER_TYPE 0

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

// Button pins
#define A_BUTTON 19
#define B_BUTTON 22
#define LEFT_BUTTON 32
#define RIGHT_BUTTON 33
#define UP_BUTTON 15
#define DOWN_BUTTON 4
#define START_BUTTON 27
#define SELECT_BUTTON 16

// NES controller pins
#define CONTROLLER_NES_CLK 22
#define CONTROLLER_NES_LATCH 27
#define CONTROLLER_NES_DATA 35

// SNES controller pins
#define CONTROLLER_SNES_CLK 22
#define CONTROLLER_SNES_LATCH 27
#define CONTROLLER_SNES_DATA 35

// PS1/PS2 controller pins
#define CONTROLLER_PSX_DATA 35
#define CONTROLLER_PSX_COMMAND 22
#define CONTROLLER_PSX_ATTENTION 19
#define CONTROLLER_PSX_CLK 23

#define DAC_PIN 0 // 0 = GPIO25, 1 = GPIO26

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial


// If using an ESP32-S3
// External DAC pin configuration
#define I2S_BCLK_PIN   38   // Bit clock (BCLK)
#define I2S_LRC_PIN    39   // Word select / Left-Right clock (LRC / WS)
#define I2S_DOUT_PIN   40   // Serial data output (DIN)

#endif