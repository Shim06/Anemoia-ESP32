#ifndef MAPPER069_H
#define MAPPER069_H

#include "../mapper.h"

#define MAPPER069_NUM_PRG_BANKS_8K 17
#define MAPPER069_NUM_CHR_BANKS_1K 20

Mapper createMapper069(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart);

void mapper069_mapPages(Mapper* mapper, Bus* bus);
void mapper069_mapPPUPages(Mapper* mapper, Ppu2C02* ppu);
void mapper069_cycle(Mapper* mapper, int cycles);
void mapper069_reset(Mapper* mapper);
void mapper069_dumpState(Mapper* mapper, File& state);
void mapper069_loadState(Mapper* mapper, File& state);
#endif
