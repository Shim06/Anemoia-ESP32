#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <SD.h>
#include <SPI.h>
#include <vector>

#include "mapper.h"
#include "mappers/mapper000.h"
#include "mappers/mapper001.h"
#include "mappers/mapper002.h"
#include "mappers/mapper003.h"
#include "mappers/mapper004.h"
#include "mappers/mapper069.h"

class Bus;
class Cartridge
{
public:
    Cartridge(const char* filename);
    ~Cartridge();

    enum MIRROR
    {
        HORIZONTAL,
        VERTICAL,
        ONESCREEN_LOW,
        ONESCREEN_HIGH,
        HARDWARE
    };

    bool cpuRead(uint16_t addr, uint8_t& data);
	bool cpuWrite(uint16_t addr, uint8_t data);
	bool ppuRead(uint16_t addr, uint8_t& data);
    uint8_t* ppuReadPtr(uint16_t addr);
	bool ppuWrite(uint16_t addr, uint8_t data);
    void ppuScanline();
    void cpuCycle(int cycles);
    void reset();

    void loadPRGBank(uint8_t* bank, uint16_t size, uint32_t offset);
    void loadCHRBank(uint8_t* bank, uint16_t size, uint32_t offset);
    void setMirrorMode(MIRROR mirror);
    Cartridge::MIRROR getMirrorMode();
    void connectBus(Bus* n) { bus = n; }
    void IRQ();

    void dumpState(File& state);
    void loadState(File& state);
    bool isValid();

    uint8_t hardware_mirror;
    uint8_t mirror = HORIZONTAL;
    uint32_t CRC32 = ~0U;

private:
	Bus* bus = nullptr;
    bool is_valid = true;
    uint32_t prg_base;
    uint32_t chr_base;

    File rom;
    Mapper mapper;
    uint8_t mapper_ID = 0;
	uint8_t number_PRG_banks = 0;
	uint8_t number_CHR_banks = 0;

    uint32_t crc32(const void* buf, size_t size, uint32_t seed = ~0U);
};

#endif