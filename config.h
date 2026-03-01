#ifndef CONFIG_H
#define CONFIG_H

// Controller Configuration
// 0 = CONTROLLER_GPIO, 1 = CONTROLLER_NES, 
// 2 = CONTROLLER_SNES, 3 = CONTROLLER_PSX
#define CONTROLLER_TYPE 0

// MicroSD card configuration
#define SD_FREQ 80000000

// Screen Configuration
// #define TFT_BACKLIGHT_ENABLE // Uncomment this line if using a screen with backlight pin
#define TFT_BACKLIGHT_PIN 21
#define SCREEN_ROTATION 1 // Screen orientation: 1 or 3 (1 = landscape, 3 = landscape flipped)
#define SCREEN_SWAP_BYTES // Uncomment if colors appear wrong
// #define TFT_PARALLEL // Uncomment this line if using parallel communication instead of SPI communication

// MicroSD card module Pins
#define SD_MOSI_PIN 13
#define SD_MISO_PIN 34
#define SD_SCLK_PIN 26
#define SD_CS_PIN -1

// Button pins
#define A_BUTTON 22
#define B_BUTTON 21
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 12
#define UP_BUTTON 15
#define DOWN_BUTTON 5
#define START_BUTTON 32
#define SELECT_BUTTON 27

// NES controller pins
#define CONTROLLER_NES_CLK 5
#define CONTROLLER_NES_LATCH 19
#define CONTROLLER_NES_DATA 21

// SNES controller pins
#define CONTROLLER_SNES_CLK 5
#define CONTROLLER_SNES_LATCH 19
#define CONTROLLER_SNES_DATA 21

// PS1/PS2 controller pins
#define CONTROLLER_PSX_DATA 5
#define CONTROLLER_PSX_COMMAND 19
#define CONTROLLER_PSX_ATTENTION 21
#define CONTROLLER_PSX_CLK 22

#define DAC_PIN 1 // 0 = GPIO25, 1 = GPIO26

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial

#endif