#ifndef RTCONFIG_H
#define RTCONFIG_H

#include "config.h"
#include "src/debug.h"
#include <FS.h>
#include <LittleFS.h>

struct __attribute__((packed)) RuntimeConfig
{
    uint8_t rotation;
    uint8_t dac_pin;
    ControllerType controller_type;
    uint8_t sd_freq;
    bool backlight;
    bool demo_mode;
};

inline RuntimeConfig loadConfig()
{
    RuntimeConfig cfg = {
        .rotation = SCREEN_ROTATION,
        .dac_pin = DAC_PIN,
        .controller_type = CONTROLLER_TYPE,
        .sd_freq = SD_FREQ / 1000000,
#ifdef TFT_BACKLIGHT_ENABLE
        .backlight = true,
#else
        .backlight = false,
#endif
#ifdef DEMO_MODE_UNLOCKED
        .demo_mode = true,
#else
        .demo_mode = false,
#endif
    };
    // runtime_config.bin can make development difficult because it will override #defines
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
    fs::File f = LittleFS.open("/runtime_config.bin", "r");
    if (!f)
    {
        LOG("runtime_config.bin not found, using defines in config.h");
        return cfg;
    }

    LOG("runtime_config.bin opened");
    f.read((uint8_t*)&cfg, sizeof(cfg));
    f.close();
    LOG("runtime_config.bin read successfully");

    LOGF("runtime_config.rotation:   %d\n", cfg.rotation);
    LOGF("runtime_config.dac_pin:    %d\n", cfg.dac_pin);
    LOGF("runtime_config.controller: %d\n", cfg.controller_type);
    LOGF("runtime_config.sd_freq:    %dMHz\n", cfg.sd_freq);
    LOGF("runtime_config.backlight:  %d\n", cfg.backlight);
    LOGF("runtime_config.demo_mode:  %d\n", cfg.demo_mode);
    return cfg;
}

#endif
