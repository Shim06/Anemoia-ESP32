#ifndef UI_H
#define UI_H

#include <SD.h>
#include <TFT_eSPI.h>

#include "controller.h"
#include "core/bus.h"

#define BG_COLOR 0x0015
#define BAR_COLOR 0xAD55
#define TEXT_COLOR 0xFFFF
#define TEXT2_COLOR 0xA800
#define SELECTED_TEXT_COLOR 0x57CA
#define SELECTED_BG_COLOR 0x0560

class UI
{
public:
    UI(TFT_eSPI* screen);
    ~UI();
    Cartridge* selectGame();
    void getNesFiles();
    void drawFileList();
    void drawWindowBox(int x, int y, int w, int h);
    void drawBars();
    void pauseMenu(Bus* nes);

    bool paused = false;

private:
    void drawText(const char* text, const int x, const int y);
    TFT_eSPI* screen = nullptr;
    int selected = 0;
    int prev_selected = 0;
    int scroll_offset = 0;
    int max_items = 0;
    static constexpr int ITEM_HEIGHT = 12;
    std::vector<std::string> files;
};

#endif