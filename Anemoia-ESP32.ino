
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <vector>

#include "src/core/bus.h"
#include "src/controller.h"
#include "src/ui.h"
#include "config.h"
#include "driver/i2s.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#ifndef OPTIMIZATION_FLAGS
#error The optimization flags were not applied! Please refer to *Step 4* of the README how to build and upload section.
#endif

TFT_eSPI screen = TFT_eSPI();
SPIClass SD_SPI(HSPI);
UI ui(&screen);
Cartridge* cart;
void setup() 
{
    // Turn off Wifi and Bluetooth to reduce CPU overhead
    #ifdef DEBUG
        Serial.begin(115200);
    #endif
    
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    esp_wifi_deinit();
    btStop();
    esp_bt_controller_disable();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    setupI2SDAC();

    // Initialize TFT screen
    screen.begin();
    screen.setRotation(SCREEN_ROTATION);
    #ifndef TFT_PARALLEL
        screen.initDMA();
    #endif
    screen.fillScreen(BG_COLOR);
    screen.startWrite();
    screen.setSwapBytes(SCREEN_SWAP_BYTES);

    // Initialize microsd card
    if(!initSD()) while (true);

    // Setup buttons
    initController();
}

void loop() 
{
    cart = ui.selectGame();
    emulate();
}

#ifdef DEBUG
    unsigned long last_frame_time = 0;
    unsigned long current_frame_time = 0;
    unsigned long total_frame_time = 0;
    unsigned long frame_count = 0;
#endif
IRAM_ATTR void emulate()
{
    Bus nes;
    nes.insertCartridge(cart);
    nes.connectScreen(&screen);
    nes.reset();
    ui.initializeSettings(&nes);

    TaskHandle_t apu_task_handle;
    xTaskCreatePinnedToCore(
    apuTask,
    "APU Task",
    1024,
    &nes.cpu.apu,
    1,
    &apu_task_handle,
    0
    );

    #ifdef DEBUG
        last_frame_time = esp_timer_get_time();
    #endif

    // Target frame time: 16639µs (60.098 FPS)
    #define FRAME_TIME 16639
    uint64_t next_frame = esp_timer_get_time();
    // Emulation Loop
    while (true) 
    {
        // Read button input
        nes.controller = 0;
        nes.controller = controllerRead();

        // Start + Select opens the pause menu
        if ((nes.controller & CONTROLLER::Start) && (nes.controller & CONTROLLER::Select)) 
        {
            if (!ui.paused)
            {
                vTaskSuspend(apu_task_handle);
                ui.pauseMenu(&nes);
                vTaskResume(apu_task_handle);
                next_frame = esp_timer_get_time() + FRAME_TIME;
                nes.controller = 0;
            }
        }

        // Generate one frame
        nes.clock();

        #ifdef DEBUG
            current_frame_time = esp_timer_get_time();
            total_frame_time += (current_frame_time - last_frame_time);
            frame_count++;

            if ((frame_count & 63) == 0)
            {
                float avg_fps = (1000000.0 * frame_count) / total_frame_time;
                Serial.printf("FPS: %.2f\n", avg_fps);
                total_frame_time = 0;
                frame_count = 0;
            }

            last_frame_time = current_frame_time;
        #endif

        // Frame limiting
        uint64_t now = esp_timer_get_time();
        if (now < next_frame) ets_delay_us(next_frame - now);
        next_frame += FRAME_TIME;
    }
    #undef FRAME_TIME
}

bool initSD() 
{
    Serial.println("Initializing SD...");
    SD_SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SD_SPI, SD_FREQ)) 
    {
        #ifdef DEBUG
            Serial.println("SD Card Mount Failed");
        #endif

        screen.setTextSize(2);
        const char* txt1 = "SD Init failed!";
        const char* txt2 = "Insert SD card or";
        const char* txt3 = "lower SD frequency";
        const char* txt4 = "in config.h";
        int w1 = screen.textWidth(txt1, 2);
        int w2 = screen.textWidth(txt2, 2);
        int w3 = screen.textWidth(txt3, 2);
        int w4 = screen.textWidth(txt4, 2);
        
        int x1 = (320 - w1) / 2;
        int x2 = (320 - w2) / 2;
        int x3 = (320 - w3) / 2;
        int x4 = (320 - w4) / 2;

        screen.setTextColor(TFT_BLACK);
        screen.drawString(txt1, x1, 56, 2);
        screen.drawString(txt2, x2, 88, 2);
        screen.drawString(txt3, x3, 120, 2);
        screen.drawString(txt4, x4, 152, 2);
        return false;
    }

    return true;
}

void setupI2SDAC()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    #if DAC_PIN == 0
        i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    #elif DAC_PIN == 1
        i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
        i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
    #else
        #error "Invalid DAC PIN is used."
    #endif
}

void apuTask(void* param) 
{
    Apu2A03* apu = (Apu2A03*)param;

    while (true)
    {
        apu->clock();
    }
}