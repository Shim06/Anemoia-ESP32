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
    max_items = (screen->width() - 56) / ITEM_HEIGHT;

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
    if (prev_selected != selected) screen->fillRect(10, 32, screen->width() - 20, screen->height() - 64, BG_COLOR);
    const int size = files.size();
    for (int i = 0; i < max_items; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;
        const char* filename = files[item].c_str();
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

    int prev_select = 0;
    int select = 0;

    screen->endWrite();

    // Draw bars with text
    drawBars();
    const char* text2 = "Pause";
    int text2_x = screen->width() - screen->textWidth(text2) - 12;
    screen->fillRect(text2_x - 4, 0, screen->textWidth(text2) + 8, 16, SELECTED_BG_COLOR);
    screen->setTextColor(TFT_BLACK, SELECTED_BG_COLOR);
    screen->setCursor(text2_x, 4);
    screen->print(text2);
    screen->setCursor(text2_x, 4);
    screen->setTextColor(TEXT2_COLOR, SELECTED_BG_COLOR);
    screen->print(text2[0]);

    // Draw pause window 
    constexpr int window_w = 124;
    constexpr int window_h = 104;
    int window_x = screen->width() - window_w;
    constexpr int window_y = 16;
    screen->fillRect(window_x, window_y, window_w, window_h, BAR_COLOR);


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
        screen->drawRect(window_x + 8, section_y, w, h, TFT_BLACK);
        screen->drawRect(window_x + 9, section_y, w, h, TFT_BLACK);

        section_y += (h - 1);
    }

    // Draw items
    constexpr int items_y[] = { 28, 40, 60, 72, 90 };
    screen->fillRect(window_x + 10, items_y[0], window_w - 19, item_height, SELECTED_BG_COLOR);
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + text_padding;
        screen->setTextColor(TFT_BLACK);
        screen->setCursor(window_x + 12, y);
        screen->print(items[i]);
        screen->setCursor(window_x + 12, y);
        screen->setTextColor(TEXT2_COLOR);
        screen->print(items[i][0]);
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
                case 0: // Resume
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    return;
            
                case 1: // Reset
                    nes->reset();
                    screen->startWrite();
                    return;

                case 2: // Quick Save State
                    nes->saveState();
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
                    return;

                case 3: // Quick Load State
                    nes->loadState();
                    screen->fillScreen(TFT_BLACK);
                    screen->startWrite();
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
            screen->fillRect(window_x + 10, items_y[prev_select], window_w - 19, item_height, BAR_COLOR);
            y = items_y[prev_select] + text_padding;
            screen->setTextColor(TFT_BLACK);
            screen->setCursor(window_x + 12, y);
            screen->print(items[prev_select]);
            screen->setCursor(window_x + 12, y);
            screen->setTextColor(TEXT2_COLOR);
            screen->print(items[prev_select][0]);

            // Draw new selection
            screen->fillRect(window_x + 10, items_y[select], window_w - 19, item_height, SELECTED_BG_COLOR);
            y = items_y[select] + text_padding;
            screen->setTextColor(TFT_BLACK);
            screen->setCursor(window_x + 12, y);
            screen->print(items[select]);
            screen->setCursor(window_x + 12, y);
            screen->setTextColor(TEXT2_COLOR);
            screen->print(items[select][0]);
        }

        prev_select = select;
    }
}
