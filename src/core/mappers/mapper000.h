#ifndef MAPPER000_H
#define MAPPER000_H

#include "../mapper.h"

struct Mapper000_state
{
    Cartridge* cart = nullptr;
    MappedROM* mROM = nullptr;
    ROMBackend backend;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* PRG_ROM = nullptr;
    uint8_t* CHR_ROM = nullptr;
    uint8_t* CHR_bank = nullptr;
    uint8_t* PRG_banks[2];
};

Mapper createMapper000(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart);

void mapper000_mapPages(Mapper* mapper, Bus* bus);
void mapper000_mapPPUPages(Mapper* mapper, Ppu2C02* ppu);
void mapper000_reset(Mapper* mapper);
void mapper000_dumpState(Mapper* mapper, File& state);
void mapper000_loadState(Mapper* mapper, File& state);
#endif
