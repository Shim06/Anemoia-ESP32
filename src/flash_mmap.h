#ifndef FLASH_MMAP_H
#define FLASH_MMAP_H

#include <Arduino.h>
#include "debug.h"
#include "esp_partition.h"
#include "core/mapper.h"
#include "core/rom_types.h"

class Cartridge;
struct MappedROM
{
    const uint8_t* prg_base;
    const uint8_t* chr_base;
    uint32_t prg_size;
    uint32_t chr_size;
    esp_partition_mmap_handle_t mmap_handle;
};

bool mappedROM_init(MappedROM* rom, Cartridge* cart, uint32_t crc32,
                   uint8_t num_prg_banks_16k, uint8_t num_chr_banks_8k);

IRAM_ATTR uint8_t ROM_read(const MappedROM* rom, const RomType type, const uint32_t offset);

#endif