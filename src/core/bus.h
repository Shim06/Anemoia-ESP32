#ifndef BUS_H
#define BUS_H

#include "../debug.h"
#include "cartridge.h"
#include "config.h"
#include "cpu6502.h"
#include "ppu2C02.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdint.h>
#include <stdio.h>

class Bus
{
public:
    Bus();
    ~Bus();

public:
    Cpu6502 cpu;
    Ppu2C02 ppu;
    Cartridge* cart;
    uint8_t RAM[2048];
    uint8_t controller = 0x00;

    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t cpuRead(uint16_t addr);
    void setPPUMirrorMode(Cartridge::MIRROR mirror);
    Cartridge::MIRROR getPPUMirrorMode();

    void insertCartridge(Cartridge* cartridge);
    void connectScreen(TFT_eSPI* screen);
    void reset();
    void clock();
    void IRQ();
    void NMI();
    void OAM_Write(uint8_t addr, uint8_t data);
    uint16_t ppu_scanline = 0;
    void renderImage(uint16_t scanline);

    void saveState();
    void loadState();

private:
    void cpuClock();
    TFT_eSPI* ptr_screen;
    uint8_t controller_state;
    uint8_t controller_strobe = 0x00;
    bool frame_latch = false;
};

#endif