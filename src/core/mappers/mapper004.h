#ifndef MAPPER004_H
#define MAPPER004_H

#include "../mapper.h"

#define MAPPER004_NUM_PRG_BANKS_8K 17
#define MAPPER004_NUM_CHR_BANKS_1K 16

Mapper createMapper004(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart);

void mapper004_mapPages(Mapper* mapper, Bus* bus);
void mapper004_mapPPUPages(Mapper* mapper, Ppu2C02* ppu);
void mapper004_scanline(Mapper* mapper);
void mapper004_reset(Mapper* mapper);
void mapper004_dumpState(Mapper* mapper, File& state);
void mapper004_loadState(Mapper* mapper, File& state);
#endif
