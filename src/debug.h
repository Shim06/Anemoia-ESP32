#ifndef DEBUG_H
#define DEBUG_H

#include "../config.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

#ifdef DEBUG
    #define LOG(msg)       Serial.println(msg)
    #define LOGF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)

inline void log_pin_config()
{
    LOG("========== PIN CONFIGURATION ==========");
    LOG("--- SD ---");
    LOGF("SD_SCLK_PIN:              %d\n", SD_SCLK_PIN);
    LOGF("SD_MISO_PIN:              %d\n", SD_MISO_PIN);
    LOGF("SD_MOSI_PIN:              %d\n", SD_MOSI_PIN);
    LOGF("SD_CS_PIN:                %d\n", SD_CS_PIN);
    LOGF("SD_SPI_PORT:              %d\n", SD_SPI_PORT);
    LOG("--- TFT ---");
    LOGF("TFT_MOSI:                 %d\n", TFT_MOSI);
    LOGF("TFT_MISO:                 %d\n", TFT_MISO);
    LOGF("TFT_SCLK:                 %d\n", TFT_SCLK);
    LOGF("TFT_CS:                   %d\n", TFT_CS);
    LOGF("TFT_DC:                   %d\n", TFT_DC);
    LOGF("TFT_RST:                  %d\n", TFT_RST);
    LOGF("TFT_BACKLIGHT_PIN:        %d\n", TFT_BACKLIGHT_PIN);
    LOG("--- Controllers ---");
    LOGF("CONTROLLER_NES_CLK:       %d\n", CONTROLLER_NES_CLK);
    LOGF("CONTROLLER_NES_LATCH:     %d\n", CONTROLLER_NES_LATCH);
    LOGF("CONTROLLER_NES_DATA:      %d\n", CONTROLLER_NES_DATA);
    LOGF("CONTROLLER_SNES_CLK:      %d\n", CONTROLLER_SNES_CLK);
    LOGF("CONTROLLER_SNES_LATCH:    %d\n", CONTROLLER_SNES_LATCH);
    LOGF("CONTROLLER_SNES_DATA:     %d\n", CONTROLLER_SNES_DATA);
    LOGF("CONTROLLER_PSX_DATA:      %d\n", CONTROLLER_PSX_DATA);
    LOGF("CONTROLLER_PSX_COMMAND:   %d\n", CONTROLLER_PSX_COMMAND);
    LOGF("CONTROLLER_PSX_ATTENTION: %d\n", CONTROLLER_PSX_ATTENTION);
    LOGF("CONTROLLER_PSX_CLK:       %d\n", CONTROLLER_PSX_CLK);
    LOG("--- GPIO Buttons ---");
    LOGF("A_BUTTON:                 %d\n", A_BUTTON);
    LOGF("B_BUTTON:                 %d\n", B_BUTTON);
    LOGF("LEFT_BUTTON:              %d\n", LEFT_BUTTON);
    LOGF("RIGHT_BUTTON:             %d\n", RIGHT_BUTTON);
    LOGF("UP_BUTTON:                %d\n", UP_BUTTON);
    LOGF("DOWN_BUTTON:              %d\n", DOWN_BUTTON);
    LOGF("START_BUTTON:             %d\n", START_BUTTON);
    LOGF("SELECT_BUTTON:            %d\n", SELECT_BUTTON);
    LOG("--- DAC ---");
    LOGF("DAC_PIN:                  %d\n", DAC_PIN);
    LOG("=======================================");
}

#else
    #define LOG(msg)
    #define LOGF(fmt, ...)
inline void log_pin_config()
{
}
#endif

#endif