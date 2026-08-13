#ifndef MAPPER003_H
#define MAPPER003_H

#include "../mapper.h"

#define MAPPER003_NUM_CHR_BANKS_8K 16
struct Mapper003_state
{
    Cartridge* cart = nullptr;
    MappedROM* mROM = nullptr;
    ROMBackend backend;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;

    uint8_t* ptr_CHR_bank_8K = nullptr;
    Bank CHR_banks_8K[MAPPER003_NUM_CHR_BANKS_8K];
    BankCache CHR_cache_8K;
    uint8_t* PRG_bank = nullptr;
};

Mapper createMapper003(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart);

void mapper003_mapPages(Mapper* mapper, Bus* bus);
void mapper003_mapPPUPages(Mapper* mapper, Ppu2C02* ppu);
void mapper003_reset(Mapper* mapper);
void mapper003_dumpState(Mapper* mapper, File& state);
void mapper003_loadState(Mapper* mapper, File& state);
#endif
