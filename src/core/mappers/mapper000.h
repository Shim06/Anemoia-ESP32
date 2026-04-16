#ifndef MAPPER000_H
#define MAPPER000_H

#include "../mapper.h"

struct Mapper000_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t PRG_ROM[32*1024];
    uint8_t CHR_ROM[8*1024];
    uint8_t* CHR_bank;
    uint8_t* PRG_banks[2];
};

Mapper createMapper000(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

bool mapper000_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper000_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
bool mapper000_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper000_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
uint8_t* mapper000_ppuReadPtr(Mapper* mapper, uint16_t addr);
void mapper000_reset(Mapper* mapper);
void mapper000_dumpState(Mapper* mapper, File& state);
void mapper000_loadState(Mapper* mapper, File& state);
#endif