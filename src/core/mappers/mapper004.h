#ifndef MAPPER004_H
#define MAPPER004_H

#include "../mapper.h"

#define MAPPER004_NUM_PRG_BANKS_8K 18
#define MAPPER004_NUM_CHR_BANKS_1K 26

Mapper createMapper004(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart);

bool mapper004_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper004_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
bool mapper004_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper004_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
uint8_t* mapper004_ppuReadPtr(Mapper* mapper, uint16_t addr);
void mapper004_scanline(Mapper* mapper);
void mapper004_reset(Mapper* mapper);
void mapper004_dumpState(Mapper* mapper, File& state);
void mapper004_loadState(Mapper* mapper, File& state);
#endif