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
    if (state->backend == ROMBackend::LRU)
        state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, bank, RomType::CHR);
    else state->ptr_CHR_bank_8K = (uint8_t*)(state->mROM->chr_base + bank * 8U * 1024U);
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
    switch (state->backend)
    {
    case ROMBackend::LRU:
        state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, 0, RomType::CHR);
        state->cart->loadPRGBank(state->PRG_bank, 32 * 1024, 0);
        return;

    case ROMBackend::FLASH: return;
    }
}

void mapper003_dumpState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;
    switch (s->backend)
    {
    case ROMBackend::LRU:
    {
        uint8_t CHR_bank = getBankIndex(&s->CHR_cache_8K, s->ptr_CHR_bank_8K);
        state.write((uint8_t*)&CHR_bank, sizeof(CHR_bank));
        return;
    }
    case ROMBackend::FLASH: return;
    }
}

void mapper003_loadState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;
    switch (s->backend)
    {
    case ROMBackend::LRU:
    {
        uint8_t CHR_bank;
        state.read((uint8_t*)&CHR_bank, sizeof(CHR_bank));
        invalidateCache(&s->CHR_cache_8K);
        s->ptr_CHR_bank_8K = getBank(&s->CHR_cache_8K, CHR_bank, RomType::CHR);
    }

    case ROMBackend::FLASH: return;
    }
}

Mapper createMapper003(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart)
{
    Mapper mapper;
    Mapper003_state* state = new Mapper003_state;
    switch (backend)
    {
    case ROMBackend::LRU:
        state->PRG_bank = (uint8_t*)malloc(32U * 1024U);
        bankInit(&state->CHR_cache_8K, state->CHR_banks_8K, MAPPER003_NUM_CHR_BANKS_8K, 16U * 1024U,
                 cart);
        break;

    case ROMBackend::FLASH: state->mROM = &cart->mROM; break;
    }

    state->backend = backend;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;
    mapper.state = state;
    return mapper;
}