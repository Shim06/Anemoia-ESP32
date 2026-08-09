#ifndef MAPPER002_H
#define MAPPER002_H

#include "../mapper.h"

#define MAPPER002_NUM_PRG_BANKS_16K 7
struct Mapper002_state
{
    Cartridge* cart = nullptr;
    MappedROM* mROM = nullptr;
    ROMBackend backend;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* ptr_PRG_bank_16K[2];
    Bank PRG_banks_16K[MAPPER002_NUM_PRG_BANKS_16K];
    BankCache PRG_cache_16K;
    uint8_t* PRG_bank = nullptr;
    uint8_t* CHR_bank = nullptr;
};

class Bus;
Mapper createMapper002(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart);

void mapper002_mapPages(Mapper* mapper, Bus* bus);
bool mapper002_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper002_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
uint8_t* mapper002_ppuReadPtr(Mapper* mapper, uint16_t addr);
void mapper002_reset(Mapper* mapper);
void mapper002_dumpState(Mapper* mapper, File& state);
void mapper002_loadState(Mapper* mapper, File& state);
#endif
