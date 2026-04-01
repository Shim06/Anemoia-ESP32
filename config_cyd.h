#ifndef CONFIG_CYD_H
#define CONFIG_CYD_H

// Controller Configuration
// Only NES controller supported for CYD
#define CONTROLLER_TYPE 1 // 1 = CONTROLLER_NES

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

#define DAC_PIN 1 // 0 = GPIO25, 1 = GPIO26 (CYD uses GPIO26)

#define FRAMESKIP
// #define DEBUG // Uncomment this line if you want debug prints from serial

#endif
