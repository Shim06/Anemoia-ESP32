#ifndef MAPPER069_H
#define MAPPER069_H

#include "../mapper.h"

#define MAPPER069_NUM_PRG_BANKS_8K 17
#define MAPPER069_NUM_CHR_BANKS_1K 26

Mapper createMapper069(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

bool mapper069_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper069_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
bool mapper069_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper069_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
uint8_t* mapper069_ppuReadPtr(Mapper* mapper, uint16_t addr);
void mapper069_cycle(Mapper* mapper, int cycles);
void mapper069_reset(Mapper* mapper);
void mapper069_dumpState(Mapper* mapper, File& state);
void mapper069_loadState(Mapper* mapper, File& state);
#endif