#include "mapper003.h"
#include "../cartridge.h"

IRAM_ATTR bool mapper003_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    data = state->PRG_bank[addr & 0x7FFF];
    return true;
}

IRAM_ATTR bool mapper003_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x8000) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    uint8_t bank = data & 0x03;
    state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, bank, Mapper::ROM_TYPE::CHR_ROM);
    return true;
}

IRAM_ATTR bool mapper003_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    data = state->ptr_CHR_bank_8K[addr];
    return true;
}

IRAM_ATTR bool mapper003_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    return false;
}

IRAM_ATTR uint8_t* mapper003_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper003_state* state = (Mapper003_state*)mapper->state;
    return &state->ptr_CHR_bank_8K[addr];
}

void mapper003_reset(Mapper* mapper)
{
    Mapper003_state* state = (Mapper003_state*)mapper->state;

    state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->cart->loadPRGBank(state->PRG_bank, 32 * 1024, 0);
}

void mapper003_dumpState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;

    uint8_t CHR_bank = getBankIndex(&s->CHR_cache_8K, s->ptr_CHR_bank_8K);
    state.write((uint8_t*)&CHR_bank, sizeof(CHR_bank));
}

void mapper003_loadState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;

    uint8_t CHR_bank;
    state.read((uint8_t*)&CHR_bank, sizeof(CHR_bank));
    invalidateCache(&s->CHR_cache_8K);
    s->ptr_CHR_bank_8K = getBank(&s->CHR_cache_8K, CHR_bank, Mapper::ROM_TYPE::PRG_ROM);
}

Mapper createMapper003(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    Mapper003_state* state = new Mapper003_state;
    bankInit(&state->CHR_cache_8K, state->CHR_banks_8K, MAPPER003_NUM_CHR_BANKS_8K, 8U * 1024U, cart);

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    mapper.state = state;
    return mapper;
}