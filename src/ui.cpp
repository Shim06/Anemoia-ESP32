#include "ui.h"

UI::UI(TFT_eSPI* screen)
{    
    this->screen = screen;
}

UI::~UI()
{
}

Cartridge* UI::selectGame()
{
    unsigned int last_input_time = 0;
    constexpr unsigned int delay = 250; 
    max_items = (screen->height() - 56) / ITEM_HEIGHT;

    drawWindowBox(2, 20, screen->width() - 4, screen->height() - 40);
    drawBars();
    getNesFiles();
    drawFileList();

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
                    scroll_offset = selected - max_items + 1;
                }
                else if (selected < scroll_offset) scroll_offset = selected; 
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
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
                else if (selected >= scroll_offset + max_items) scroll_offset = selected - max_items + 1;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                drawFileList();
                last_input_time = now;
            }
            
        }
        
        if (isDownPressed(CONTROLLER::A) && (selected >= 0 && selected < size))
        {
            Cartridge* cart;
            const char* game = ("/" + files[selected]).c_str();
            std::vector<std::string>().swap(files);
            cart = new Cartridge(game);
            return cart;
        }
    }
}

void UI::getNesFiles()
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

void UI::drawFileList()
{
    if (prev_selected != selected) 
        screen->fillRect(10, 32, screen->width() - 20, screen->height() - 64, BG_COLOR);

    const int size = files.size();
    for (int i = 0; i < max_items; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;

        std::string file = files[item];
        int maxWidth = screen->width() - 28;
        while (screen->textWidth(file.c_str()) > maxWidth)
        {
            file.pop_back();
        }
        if (file.size() < files[item].size())
        {
            file.replace(file.size()-3, 3, "...");
        }

        const char* filename = file.c_str();
        int y = i * ITEM_HEIGHT + 32;
        if (item == selected)
        {
            screen->setTextColor(SELECTED_TEXT_COLOR);
            screen->drawString(filename, 14, y, 1);
        }
        else
        {
            screen->setTextColor(TEXT_COLOR); 
            screen->drawString(filename, 14, y, 1);
        }
    }

    prev_selected = selected;
}

void UI::drawWindowBox(int x, int y, int w, int h) 
{
    screen->drawRect(x, y, w, h, TFT_WHITE);
    screen->drawRect(x+1, y, w-2, h, TFT_WHITE);

    screen->drawRect(x+4, y+3, w-8, h-7, TFT_WHITE);
    screen->drawRect(x+5, y+3, w-10, h-7, TFT_WHITE);

    const char* text1 = " ANEMOIA.CPP ";
    screen->setTextColor(TEXT_COLOR, BG_COLOR);
    screen->setCursor((screen->width() - screen->textWidth(text1)) / 2, 20);
    screen->print(text1);
}

void UI::drawBars() 
{
    // Top bar
    screen->fillRect(0, 0, screen->width(), 16, BAR_COLOR);
    screen->setTextColor(TFT_BLACK, BAR_COLOR);

    const char* text1 = "ANEMOIA-ESP32";
    screen->setCursor((screen->width() - screen->textWidth(text1)) / 2, 4);
    screen->print(text1);

    // Bottom bar
    screen->fillRect(0, screen->height() - 16, screen->width(), 16, BAR_COLOR);
    screen->setTextColor(TFT_BLACK, BAR_COLOR);

    int y = screen->height() - 12;
    int x = 4;

    screen->setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen->setCursor(x, y);
    screen->print("Up/Down");

    screen->setTextColor(TFT_BLACK, BAR_COLOR);
    screen->print(" Move   ");

    screen->setTextColor(TEXT2_COLOR, BAR_COLOR);
    screen->print("A");

    screen->setTextColor(TFT_BLACK, BAR_COLOR);
    screen->print(" Select");
}

void UI::pauseMenu(Bus* nes)
{
    // Black magic stuff
    // Padding bytes for code alignment for better performance
    __attribute__((used, section(".text"), aligned(64)))
    static const uint8_t padding[128] = {0};

    paused = true;
    int prev_select = 0;
    int select = 0;

    screen->endWrite();

    // Draw bars with text
    drawBars();
    const char* text2 = "Pause";
    int text2_x = screen->width() - screen->textWidth(text2) - 12;
    screen->fillRect(text2_x - 4, 0, screen->textWidth(text2) + 8, 16, SELECTED_BG_COLOR);
    drawText(text2, text2_x, 4);

    constexpr int section_count[] = { 3, 2, 1 };
    constexpr const char* items[] = 
    { 
        "Resume", "Settings", "Reset", 
        "Quick Save State", "Quick Load State", 
        "Save and Quit" 
    };
    enum ItemSelect
    {
        Resume,
        Settings,
        Reset,
        QuickSaveState,
        QuickLoadState,
        SaveAndQuit
    };
    constexpr int items_y[] = { 28, 40, 52, 72, 84, 102 };
    constexpr int num_items = sizeof(items) / sizeof(items[0]);
    constexpr int num_sections = sizeof(section_count) / sizeof(section_count[0]);
    constexpr int item_height = 12;
    constexpr int text_height = 8;
    constexpr int text_padding = (item_height - text_height) / 2;

    // Draw pause window 
    constexpr int window_w = 124;
    constexpr int window_h = 104;
    int window_x = screen->width() - window_w;
    constexpr int window_y = 16;
    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);

    // Draw section borders
    int section_y = window_y + 8;
    for (int s = 0; s < num_sections; s++)
    {
        int w = window_w - 16;
        int h = (section_count[s] * item_height) + 8;
        screen->drawRect(window_x + 8, section_y, w, h, TFT_BLACK);
        screen->drawRect(window_x + 9, section_y, w, h, TFT_BLACK);

        section_y += (h - 1);
    }

    // Draw items
    screen->fillRect(window_x + 10, items_y[0], window_w - 19, item_height, SELECTED_BG_COLOR);
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + text_padding;
        drawText(items[i], window_x + 12, y);
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

            if (isDownPressed(CONTROLLER::A)) 
            {
                switch (select)
                {
                case Resume:
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    paused = false;
                    return;

                case Settings:
                    settingsMenu(nes);

                    // Redraw pause menu
                    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);
                    section_y = window_y + 8;
                    for (int s = 0; s < num_sections; s++)
                    {
                        int w = window_w - 16;
                        int h = (section_count[s] * item_height) + 8;
                        screen->drawRect(window_x + 8, section_y, w, h, TFT_BLACK);
                        screen->drawRect(window_x + 9, section_y, w, h, TFT_BLACK);

                        section_y += (h - 1);
                    }
                    screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
                    for (int i = 0; i < num_items; i++)
                    {
                        int y = items_y[i] + text_padding;
                        drawText(items[i], window_x + 12, y);
                    }
                    last_input_time = millis() + 500;
                    break;
            
                case Reset:
                    nes->reset();
                    screen->startWrite();
                    paused = false;
                    return;

                case QuickSaveState:
                    nes->saveState();
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    paused = false;
                    return;

                case QuickLoadState:
                    nes->loadState();
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    paused = false;
                    return;

                case SaveAndQuit:
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
            screen->fillRect(window_x + 10, items_y[prev_select], window_w - 19, item_height, BAR_COLOR);
            y = items_y[prev_select] + text_padding;
            drawText(items[prev_select], window_x + 12, y);

            // Draw new selection
            screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
            y = items_y[select] + text_padding;
            drawText(items[select], window_x + 12, y);
        }

        prev_select = select;
    }
}

void UI::settingsMenu(Bus* nes)
{
    // Draw settings window 

    int prev_select = 0;
    int select = 0;

    constexpr int window_w = 124;
    constexpr int window_h = 104;
    int window_x = screen->width() - window_w;
    constexpr int window_y = 16;
    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);

    const char* text2 = "Settings";
    int text2_x = screen->width() - screen->textWidth(text2) - 12;
    screen->fillRect(text2_x - 4, 0, screen->textWidth(text2) + 8, 16, SELECTED_BG_COLOR);
    drawText(text2, text2_x, 4);

    static char volume_text[15];
    static char palette_text[20];
    static char brightness_text[20];
    const char* palette_names[] =
    {
        "NTSC 565",
        "PAL 565",
        "NTSC 222",
        "PAL 222"
    };

    snprintf(volume_text, sizeof(volume_text), "Volume: %d%%", settings.volume);
    snprintf(palette_text, sizeof(palette_text), "Palette: %s", palette_names[settings.palette]);
    snprintf(brightness_text, sizeof(brightness_text), "Brightness: %d%%", settings.brightness);
    char* items[] = 
    { 
        volume_text,
        brightness_text,
        palette_text,
        "Save & Return"
    };
    enum ItemSelect
    {
        Volume,
        Brightness,
        Palette,
        Back
    };
    constexpr int items_y[] = { 30, 42, 54, 66 };
    constexpr int num_items = sizeof(items) / sizeof(items[0]);
    constexpr int item_height = 12;
    constexpr int text_height = 8;
    constexpr int text_padding = (item_height - text_height) / 2;

    screen->drawRect(window_x + 8, window_y + 8, window_w - 16, window_h - 16, TFT_BLACK);
    screen->drawRect(window_x + 9, window_y + 8, window_w - 16, window_h - 16, TFT_BLACK);
    
    screen->fillRect(window_x + 10, items_y[0], window_w - 19, item_height, SELECTED_BG_COLOR);
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + text_padding;
        if (i == Brightness)
        {
            #ifdef TFT_BACKLIGHT_ENABLE
                drawText(items[i], window_x + 12, y);
            #else
                screen->setCursor(window_x + 12, y);
                screen->setTextColor(TFT_DARKGREY);
                screen->print(items[i]);
            #endif
        }
        else
            drawText(items[i], window_x + 12, y);
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
                #ifndef TFT_BACKLIGHT_ENABLE
                    if (select == Brightness)
                    {
                        select--;
                        if (select < 0) select = (num_items - 1);
                    }
                #endif
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down)) 
            {
                select++; 
                if (select > (num_items - 1)) select = 0;
                #ifndef TFT_BACKLIGHT_ENABLE
                    if (select == Brightness)
                    {
                        select++;
                        if (select > (num_items - 1)) select = 0;
                    }
                #endif
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Left)) 
            {
                switch (select)
                {
                case Volume:
                    if (settings.volume >= 5) settings.volume -= 5;
                    snprintf(volume_text, sizeof(volume_text), "Volume: %d%%", settings.volume);
                    screen->fillRect(window_x + 10, items_y[Volume], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Volume], window_x + 12, items_y[Volume] + text_padding);
                    break;
                case Brightness:
                    if (settings.brightness >= 5) settings.brightness -= 5;
                    snprintf(brightness_text, sizeof(brightness_text), "Brightness: %d%%", settings.brightness);
                    screen->fillRect(window_x + 10, items_y[Brightness], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Brightness], window_x + 12, items_y[Brightness] + text_padding);
                    setBrightness(settings.brightness);
                    break;
                case Palette:
                    if (settings.palette == 0)
                        settings.palette = Ppu2C02::Palette::PaletteCount - 1;
                    else settings.palette--;
                    snprintf(palette_text, sizeof(palette_text), "Palette: %s", palette_names[settings.palette]);
                    screen->fillRect(window_x + 10, items_y[Palette], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Palette], window_x + 12, items_y[Palette] + text_padding);
                    break;
                }
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Right)) 
            {
                switch (select)
                {
                case Volume:
                    if (settings.volume <= 95) settings.volume += 5;
                    snprintf(volume_text, sizeof(volume_text), "Volume: %d%%", settings.volume);
                    screen->fillRect(window_x + 10, items_y[Volume], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Volume], window_x + 12, items_y[Volume] + text_padding);
                    break;
                case Brightness:
                    if (settings.brightness <= 95) settings.brightness += 5;
                    snprintf(brightness_text, sizeof(brightness_text), "Brightness: %d%%", settings.brightness);
                    screen->fillRect(window_x + 10, items_y[Brightness], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Brightness], window_x + 12, items_y[Brightness] + text_padding);
                    setBrightness(settings.brightness);
                    break;
                case Palette:
                    settings.palette = (settings.palette + 1) % Ppu2C02::Palette::PaletteCount;
                    snprintf(palette_text, sizeof(palette_text), "Palette: %s", palette_names[settings.palette]);
                    screen->fillRect(window_x + 10, items_y[Palette], window_w - 19, item_height, SELECTED_BG_COLOR);
                    drawText(items[Palette], window_x + 12, items_y[Palette] + text_padding);
                    break;
                }
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::A)) 
            {
                switch (select)
                {
                case Back:
                    loadEmulatorSettings(nes);
                    saveSettings(&settings);
                    return;
                }
            }
        }

        // Update Selection
        if (prev_select != select)
        {
            int y;
            // Redraw old selection
            screen->fillRect(window_x + 10, items_y[prev_select], window_w - 19, item_height, BAR_COLOR);
            y = items_y[prev_select] + text_padding;
            drawText(items[prev_select], window_x + 12, y);
    

            // Draw new selection
            screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
            y = items_y[select] + text_padding;
            drawText(items[select], window_x + 12, y);
        }

        prev_select = select;
    }
}

void UI::initializeSettings()
{
    if (!SD.exists("/settings.bin"))
    {
        Settings temp;
        saveSettings(&temp);
    }
    loadSettings(&settings);

    #ifdef TFT_BACKLIGHT_ENABLE
        setBrightness(settings.brightness)
    #endif
}

void UI::loadEmulatorSettings(Bus* nes)
{
    nes->ppu.setPalette(settings.palette);
    nes->cpu.apu.setVolume(settings.volume);
}

void UI::setBrightness(int value)
{
    uint8_t pwm = ((value * 255) + 50) / 100;
    ledcWrite(BL_CHANNEL, pwm);
}

void UI::saveSettings(const Settings* s)
{
    File f = SD.open("/settings.bin", FILE_WRITE);
    if (!f) return;
        
    f.seek(0);
    f.write((uint8_t*)s, sizeof(*s));
    f.close();
}

void UI::loadSettings(Settings* s)
{
    File f = SD.open("/settings.bin", FILE_READ);
    if (!f) return;
    if (f.size() != sizeof(Settings)) 
    {
        f.close();
        Settings temp;
        saveSettings(&temp);
        *s = temp;
    }

    f.read((uint8_t*)s, sizeof(*s));
    f.close();
}

void UI::drawText(const char* text, const int x, const int y)
{
    screen->setTextColor(TFT_BLACK);
    screen->setCursor(x, y);
    screen->print(text);
    screen->setCursor(x, y);
    screen->setTextColor(TEXT2_COLOR);
    screen->print(text[0]);
}