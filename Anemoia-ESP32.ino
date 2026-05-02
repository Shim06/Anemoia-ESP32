#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <string>
#include <vector>

#include "config.h"
#include "driver/i2s.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_wifi.h"
#include "hwconfig.h"
#include "src/controller.h"
#include "src/core/bus.h"
#include "src/debug.h"
#include "src/ui.h"

#ifndef OPTIMIZATION_FLAGS
    #error The optimization flags were not applied! Please refer to *Step 4* of the README how to build and upload section.
#endif

HWConfig hw_config;
TFT_eSPI screen = TFT_eSPI();
SPIClass SD_SPI(SD_SPI_PORT);
UI ui(&screen);
Cartridge* cart;
void setup()
{
#ifdef DEBUG
    Serial.begin(115200);
    log_pin_config();
    LOGF("ESP reset reason: %d\n", esp_reset_reason());
#endif

    // Turn off Wifi and Bluetooth to reduce CPU overhead
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    esp_wifi_deinit();
    btStop();
    esp_bt_controller_disable();
    esp_bt_mem_release(ESP_BT_MODE_BTDM);
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    hw_config = loadConfig();
    setupI2SDAC();

    // Initialize TFT screen
    screen.begin();
    screen.setRotation(hw_config.rotation);
#ifndef DISABLE_DMA
    screen.initDMA();
#endif
    screen.fillScreen(BG_COLOR);
    screen.startWrite();

    if (hw_config.backlight)
    {
        pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
        ledcAttach(TFT_BACKLIGHT_PIN, BL_FREQ, BL_RESOLUTION);
        ledcWrite(TFT_BACKLIGHT_PIN, 255);
    }

    // Initialize microsd card
    if (!initSD())
        while (true);
    ui.initializeSettings();

    // Setup buttons
    initController(hw_config.controller_type);
}

void loop()
{
    cart = ui.selectGame();
    if (cart && cart->isValid()) { emulate(); }

    invalidCartridge();
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
    ui.loadEmulatorSettings(&nes);

    TaskHandle_t apu_task_handle;
    xTaskCreatePinnedToCore(apuTask, "APU Task", 1024, &nes.cpu.apu, 1, &apu_task_handle, 0);

    TaskHandle_t polling_task_handle;
    xTaskCreatePinnedToCore(pollingTask, "Polling Task", 1024, &nes, 1, &polling_task_handle, 0);

    screen.setAddrWindow(32, 0, 256, 240);

#ifdef DEBUG
    last_frame_time = esp_timer_get_time();
#endif

// Target frame time: 16639µs (60.098 FPS)
#define FRAME_TIME 16639
    uint64_t next_frame = esp_timer_get_time();
    // Emulation Loop
    while (true)
    {
        // Start + Select opens the pause menu
        if ((nes.controller & (uint8_t)CONTROLLER::Start) &&
            (nes.controller & (uint8_t)CONTROLLER::Select))
        {
            if (!ui.paused)
            {
                vTaskSuspend(apu_task_handle);
                ui.pauseMenu(&nes);
                vTaskResume(apu_task_handle);
                next_frame = esp_timer_get_time() + FRAME_TIME;
                nes.controller = 0;
                screen.setAddrWindow(32, 0, 256, 240);
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
            LOGF("FPS: %.2f\n", avg_fps);
            total_frame_time = 0;
            frame_count = 0;
        }

        last_frame_time = current_frame_time;
#endif

#ifndef DEBUG
        // Frame limiting
        uint64_t now = esp_timer_get_time();
        if (now < next_frame) ets_delay_us(next_frame - now);
#endif
        next_frame += FRAME_TIME;
    }
#undef FRAME_TIME
}

bool initSD()
{
    LOG("Initializing SD...");
    SD_SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SD_SPI, hw_config.sd_freq * 1000000))
    {
        LOG("SD Card Mount Failed");

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

        screen.setTextColor(TFT_WHITE);
        screen.drawString(txt1, x1, 56, 2);
        screen.drawString(txt2, x2, 88, 2);
        screen.drawString(txt3, x3, 120, 2);
        screen.drawString(txt4, x4, 152, 2);
        return false;
    }

    LOG("SD Card initialized.");
    return true;
}

void invalidCartridge()
{
    screen.fillScreen(BG_COLOR);
    screen.setTextColor(TFT_WHITE);
    screen.setTextDatum(MC_DATUM);
    screen.drawString("ROM Mapper not supported!", screen.width() / 2, screen.height() / 2, 2);
    delay(3000);
    ESP.restart();
}

void setupI2SDAC()
{
#if defined(CONFIG_IDF_TARGET_ESP32)
    i2s_config_t i2s_config = { .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX |
                                                     I2S_MODE_DAC_BUILT_IN),
                                .sample_rate = SAMPLE_RATE,
                                .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                                .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
                                .communication_format = I2S_COMM_FORMAT_I2S_MSB,
                                .intr_alloc_flags = 0,
                                .dma_buf_count = 2,
                                .dma_buf_len = 128,
                                .use_apll = false,
                                .tx_desc_auto_clear = true,
                                .fixed_mclk = 0 };

    if (hw_config.dac_pin == 1) i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

    if (hw_config.dac_pin == 0) i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    else if (hw_config.dac_pin == 1) i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    i2s_config_t i2s_config = { .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
                                .sample_rate = SAMPLE_RATE,
                                .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                                .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                                .communication_format = I2S_COMM_FORMAT_I2S_MSB,
                                .intr_alloc_flags = 0,
                                .dma_buf_count = 2,
                                .dma_buf_len = 128,
                                .use_apll = false,
                                .tx_desc_auto_clear = true };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK)
    {
        LOGF("I2S install failed: %d\n", err);
        return;
    }

    i2s_pin_config_t pin_config = { .bck_io_num = I2S_BCLK_PIN,
                                    .ws_io_num = I2S_LRC_PIN,
                                    .data_out_num = I2S_DOUT_PIN,
                                    .data_in_num = I2S_PIN_NO_CHANGE };
    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK)
    {
        LOGF("I2S pin config failed: %d\n", err);
        return;
    }
    LOGF("I2S initialized: BCLK=%d, LRC=%d, DOUT=%d\n", I2S_BCLK_PIN, I2S_LRC_PIN, I2S_DOUT_PIN);
#endif
}

void apuTask(void* param)
{
    Apu2A03* apu = (Apu2A03*)param;

    while (true) { apu->clock(); }
}

void pollingTask(void* param)
{
    Bus* nes = (Bus*)param;
    const TickType_t frameTicks = pdMS_TO_TICKS(1000 / 60);
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        // Read button input
        nes->controller = controllerRead();

        vTaskDelayUntil(&lastWakeTime, frameTicks);
    }
}
