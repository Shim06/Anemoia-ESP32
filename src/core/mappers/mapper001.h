#ifndef MAPPER001_H
#define MAPPER001_H

#include "../mapper.h"

#define MAPPER001_NUM_PRG_BANKS_16K 8
#define MAPPER001_NUM_CHR_BANKS_8K  1
#define MAPPER001_NUM_CHR_BANKS_4K  5

class Bus;
Mapper createMapper001(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart);

void mapper001_mapPages(Mapper* mapper, Bus* bus);
bool mapper001_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data);
bool mapper001_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data);
uint8_t* mapper001_ppuReadPtr(Mapper* mapper, uint16_t addr);
void mapper001_reset(Mapper* mapper);
void mapper001_dumpState(Mapper* mapper, File& state);
void mapper001_loadState(Mapper* mapper, File& state);
#endif
