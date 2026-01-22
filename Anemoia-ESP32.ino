
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <string>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <vector>

#include "src/core/bus.h"
#include "src/controller.h"
#include "config.h"
#include "driver/i2s.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#define BG_COLOR 0x0015
#define BAR_COLOR 0xAD55
#define TEXT_COLOR 0xFFFF
#define TEXT2_COLOR 0xA800
#define SELECTED_TEXT_COLOR 0x57CA
#define SELECTED_BG_COLOR 0x0560
#ifndef OPTIMIZATION_FLAGS
#error The optimization flags were not applied! Please refer to *Step 4* of the README how to build and upload section.
#endif

TFT_eSPI screen = TFT_eSPI();
SPIClass SD_SPI(HSPI);
std::vector<std::string> files;
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

    selectGame();

}

void loop() 
{
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
            vTaskSuspend(apu_task_handle);
            pauseMenu(&nes);
            vTaskResume(apu_task_handle);
            next_frame = esp_timer_get_time() + FRAME_TIME;
            nes.controller = 0;
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
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
}

void apuTask(void* param) 
{
    Apu2A03* apu = (Apu2A03*)param;

    while (true)
    {
        apu->clock();
    }
}

int selected = 0;
int prev_selected = 0;
int scroll_offset = 0;
constexpr int ITEM_HEIGHT = 12;
int MAX_ITEMS = (screen.width() - 56) / ITEM_HEIGHT;
void getNesFiles()
{
    File root = SD.open("/");
    while (true)
    {
        File file = root.openNextFile();
        if (!file) break;
        if (!file.isDirectory())
        {
            std::string filename = file.name();
            if (filename.rfind(".nes") == filename.size() - 4)
                files.push_back(filename);
        }

        file.close();
    }

    root.close();
}

void drawFileList()
{
    if (prev_selected != selected) screen.fillRect(10, 32, screen.width() - 20, screen.height() - 64, BG_COLOR);
    const int size = files.size();
    for (int i = 0; i < MAX_ITEMS; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;
        const char* filename = files[item].c_str();
        int y = i * ITEM_HEIGHT + 32;
        if (item == selected)
        {
            screen.setTextColor(SELECTED_TEXT_COLOR);
            screen.drawString(filename, 14, y, 1);
        }
        else
        {
            screen.setTextColor(TEXT_COLOR); 
            screen.drawString(filename, 14, y, 1);
        }
    }

    prev_selected = selected;
}

void selectGame()
{
    drawBars();
    drawWindowBox(2, 20, screen.width() - 4, screen.height() - 40);
    getNesFiles();
    drawFileList();
    unsigned int last_input_time = 0;
    constexpr unsigned int delay = 250; 
    const int size = files.size();

    while (true)
    {
        unsigned int now = millis();

        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up)) 
            {
                selected--;
                if (selected < 0)
                {
                    selected = (size - 1);
                    scroll_offset = selected - MAX_ITEMS + 1;
                }
                else if (selected < scroll_offset)  scroll_offset = selected; 
                drawFileList();
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down)) 
            {
                selected++; 
                if (selected > (size - 1))
                {
                    selected = 0;
                    scroll_offset = selected;
                }
                else if (selected >= scroll_offset + MAX_ITEMS) scroll_offset = selected - MAX_ITEMS + 1;
                drawFileList();
                last_input_time = now;
            }
            
        }

        if (isDownPressed(CONTROLLER::A) && (selected >= 0 && selected < size))
        {
            cart = new Cartridge(("/" + files[selected]).c_str());
            std::vector<std::string>().swap(files);
            return;
        }
    }
}

void drawWindowBox(int x, int y, int w, int h) 
{
    screen.drawRect(x, y, w, h, TFT_WHITE);
    screen.drawRect(x+1, y, w-2, h, TFT_WHITE);

    screen.drawRect(x+4, y+3, w-8, h-7, TFT_WHITE);
    screen.drawRect(x+5, y+3, w-10, h-7, TFT_WHITE);

    const char* text1 = " ANEMOIA.CPP ";
    screen.setTextColor(TEXT_COLOR, BG_COLOR);
    screen.setCursor((screen.width() - screen.textWidth(text1)) / 2, 20);
    screen.print(text1);
}

void drawBars() 
{
    // Top bar
    screen.fillRect(0, 0, screen.width(), 16, BAR_COLOR);
    screen.setTextColor(TFT_BLACK, BAR_COLOR);

    const char* text1 = "ANEMOIA-ESP32";
    screen.setCursor((screen.width() - screen.textWidth(text1)) / 2, 4);
    screen.print(text1);

    // Bottom bar
    screen.fillRect(0, screen.height() - 16, screen.width(), 16, BAR_COLOR);
    screen.setTextColor(TFT_BLACK, BAR_COLOR);

    int y = screen.height() - 12;
    int x = 4;

    screen.setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen.setCursor(x, y);
    screen.print("Up/Down");

    screen.setTextColor(TFT_BLACK, BAR_COLOR);
    screen.print(" Move   ");

    screen.setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen.print("A");

    screen.setTextColor(TFT_BLACK, BAR_COLOR);
    screen.print(" Select");
}

void pauseMenu(Bus* nes)
{
    // Black magic stuff
    // Padding bytes for code alignment for better performance
    __attribute__((used, section(".text"), aligned(64)))
    static const uint8_t padding[128] = {0};

    int prev_select = 0;
    int select = 0;

    screen.endWrite();

    // Draw bars with text
    drawBars();
    const char* text2 = "Pause";
    int text2_x = screen.width() - screen.textWidth(text2) - 12;
    screen.fillRect(text2_x - 4, 0, screen.textWidth(text2) + 8, 16, SELECTED_BG_COLOR);
    screen.setTextColor(TFT_BLACK, SELECTED_BG_COLOR);
    screen.setCursor(text2_x, 4);
    screen.print(text2);
    screen.setCursor(text2_x, 4);
    screen.setTextColor(TEXT2_COLOR, SELECTED_BG_COLOR);
    screen.print(text2[0]);

    // Draw pause window 
    constexpr int window_w = 124;
    constexpr int window_h = 104;
    int window_x = screen.width() - window_w;
    constexpr int window_y = 16;
    screen.fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);


    const char* items[] = 
    { 
        "Resume", "Reset", 
        "Quick Save State", "Quick Load State", 
        "Save and Quit" 
    };
    constexpr int num_items = sizeof(items) / sizeof(items[0]);
    constexpr int item_height = 12;
    constexpr int text_height = 8;
    constexpr int text_padding = (item_height - text_height) / 2;

    // Draw section borders
    constexpr int section_count[] = { 2, 2, 1 };
    int section_y = window_y + 8;
    for (int s = 0; s < 3; s++)
    {
        int w = window_w - 16;
        int h = (section_count[s] * item_height) + 8;
        screen.drawRect(window_x + 8, section_y, w, h, TFT_BLACK);
        screen.drawRect(window_x + 9, section_y, w, h, TFT_BLACK);

        section_y += (h - 1);
    }

    // Draw items
    constexpr int items_y[] = { 28, 40, 60, 72, 90 };
    screen.fillRect(window_x + 10, items_y[0], window_w - 19, item_height, SELECTED_BG_COLOR);
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + text_padding;
        screen.setTextColor(TFT_BLACK);
        screen.setCursor(window_x + 12, y);
        screen.print(items[i]);
        screen.setCursor(window_x + 12, y);
        screen.setTextColor(TEXT2_COLOR);
        screen.print(items[i][0]);
    }

    constexpr int initial_delay = 500;
    int last_input_time = millis() + initial_delay;
    while (true)
    {
        constexpr int delay = 250; 
        int now = millis();
        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up)) 
            {
                select--;
                if (select < 0) select = (num_items - 1);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down)) 
            {
                select++; 
                if (select > (num_items - 1)) select = 0;
                last_input_time = now;
            }

            // Resume
            if (isDownPressed(CONTROLLER::A)) 
            {
                switch (select)
                {
                case 0: // Resume
                    screen.fillScreen(TFT_BLACK);
                    screen.startWrite();
                    return;
            
                case 1: // Reset
                    nes->reset();
                    screen.startWrite();
                    return;

                case 2: // Quick Save State
                    nes->saveState();
                    screen.fillScreen(TFT_BLACK);
                    screen.startWrite();
                    return;

                case 3: // Quick Load State
                    nes->loadState();
                    screen.fillScreen(TFT_BLACK);
                    screen.startWrite();
                    return;

                case 4: // Save and Quit
                    ESP.restart();
                    return;
                }
            }
        }

        // Update Selection
        if (prev_select != select)
        {
            int y;
            // Redraw old selection
            screen.fillRect(window_x + 10, items_y[prev_select], window_w - 19, item_height, BAR_COLOR);
            y = items_y[prev_select] + text_padding;
            screen.setTextColor(TFT_BLACK);
            screen.setCursor(window_x + 12, y);
            screen.print(items[prev_select]);
            screen.setCursor(window_x + 12, y);
            screen.setTextColor(TEXT2_COLOR);
            screen.print(items[prev_select][0]);

            // Draw new selection
            screen.fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
            y = items_y[select] + text_padding;
            screen.setTextColor(TFT_BLACK);
            screen.setCursor(window_x + 12, y);
            screen.print(items[select]);
            screen.setCursor(window_x + 12, y);
            screen.setTextColor(TEXT2_COLOR);
            screen.print(items[select][0]);
        }

        prev_select = select;
    }
}