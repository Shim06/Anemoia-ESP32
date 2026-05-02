#ifndef MAPPER_H
#define MAPPER_H

#include <SD.h>
#include <cstring>
#include <stdint.h>
#include <stdlib.h>

#include "../debug.h"
#include "Arduino.h"
#include "config.h"

class Cartridge;
class Mapper
{
public:
    enum ROM_TYPE : uint8_t
    {
        PRG_ROM,
        CHR_ROM
    };

    void* state = nullptr;
};

inline void mapperNoScanline(Mapper*)
{
}
inline void mapperNoCycle(Mapper*, int)
{
}

struct Bank
{
    uint8_t bank_id;
    uint8_t* bank_ptr;
    uint32_t last_used;
    uint32_t size;
};

struct BankCache
{
    Bank* banks;
    uint8_t num_banks;
    uint32_t tick;
    Cartridge* cart;
};

void bankInit(BankCache* cache, Bank* banks, uint8_t num_banks, uint32_t bank_size,
              Cartridge* cart);
uint8_t* getBank(BankCache* cache, uint8_t bank_id, Mapper::ROM_TYPE rom);
uint8_t getBankIndex(BankCache* cache, uint8_t* ptr);
void invalidateCache(BankCache* cache);

#endif