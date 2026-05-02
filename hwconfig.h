#ifndef HWCONFIG_H
#define HWCONFIG_H

#include "config.h"
#include "src/debug.h"
#include <LittleFS.h>

struct __attribute__((packed)) HWConfig
{
    uint8_t rotation;
    uint8_t dac_pin;
    ControllerType controller_type;
    uint8_t sd_freq;
    bool backlight;
};

inline HWConfig loadConfig()
{
    HWConfig cfg = {
        .rotation = SCREEN_ROTATION,
        .dac_pin = DAC_PIN,
        .controller_type = CONTROLLER_TYPE,
        .sd_freq = SD_FREQ / 1000000,
#ifdef TFT_BACKLIGHT_ENABLE
        .backlight = true,
#else
        .backlight = false,
#endif
    };
    // hwconfig.bin can make development difficult because it will override #defines
    // if you return cfg here early you can avoid that issue
    // the alternative is to erase the entire flash before starting development
    // return cfg;

    if (!LittleFS.begin())
    {
        LOG("LittleFS mount failed, attempting format...");
        if (!LittleFS.format())
        {
            LOG("LittleFS.format() failed");
            return cfg;
        }
        if (!LittleFS.begin())
        {
            LOG("LittleFS mount failed after format, using defines in config.h");
            return cfg;
        }
    }
    LOG("LittleFS mounted");
    File f = LittleFS.open("/hwconfig.bin", "r");
    if (!f)
    {
        LOG("hwconfig.bin not found, using defines in config.h");
        return cfg;
    }

    LOG("hwconfig.bin opened");
    f.read((uint8_t*)&cfg, sizeof(cfg));
    f.close();
    LOG("hwconfig.bin read successfully");

    LOGF("hw_config.rotation:   %d\n", cfg.rotation);
    LOGF("hw_config.dac_pin:    %d\n", cfg.dac_pin);
    LOGF("hw_config.controller: %d\n", cfg.controller_type);
    LOGF("hw_config.sd_freq:    %dMHz\n", cfg.sd_freq);
    LOGF("hw_config.backlight:  %d\n", cfg.backlight);
    return cfg;
}

#endif
