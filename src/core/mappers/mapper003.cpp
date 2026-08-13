#include "mapper003.h"
#include "../bus.h"
#include "../cartridge.h"
#include "../ppu2C02.h"

static void mapper003_remapCHRPages(Mapper003_state* state, Ppu2C02* ppu)
{
    for (int p = 0; p <= 0x1F; p++) ppu->ppu_read_pages[p] = state->ptr_CHR_bank_8K + (p * 256);
}

static void mapper003_bankWrite(Bus* bus, uint16_t addr, uint8_t data)
{
    Mapper003_state* state = (Mapper003_state*)bus->cart->mapper.state;
    uint8_t bank = data & 0x03;
    if (state->backend == ROMBackend::LRU)
        state->ptr_CHR_bank_8K = getBank(&state->CHR_cache_8K, bank, RomType::CHR);
    else state->ptr_CHR_bank_8K = (uint8_t*)(state->mROM->chr_base + bank * 8U * 1024U);
    mapper003_remapCHRPages(state, &bus->ppu);
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

    case ROMBackend::FLASH:
        state->ptr_CHR_bank_8K = (uint8_t*)state->mROM->chr_base;
        state->PRG_bank = (uint8_t*)state->mROM->prg_base;
        return;
    }
}

void mapper003_mapPages(Mapper* mapper, Bus* bus)
{
    Mapper003_state* state = (Mapper003_state*)mapper->state;

    // $8000-$FFFF: Bank write to switch CHR banks
    for (int p = 0x80; p <= 0xFF; p++) bus->write_handlers[p] = mapper003_bankWrite;

    // $8000-$FFFF: 32KB unbanked PRG-ROM
    for (int p = 0x80; p <= 0xFF; p++) bus->read_pages[p] = state->PRG_bank + ((p - 0x80) * 256);
}

void mapper003_mapPPUPages(Mapper* mapper, Ppu2C02* ppu)
{
    Mapper003_state* state = (Mapper003_state*)mapper->state;
    mapper003_remapCHRPages(state, ppu);
}

void mapper003_dumpState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;
    uint8_t CHR_bank;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        CHR_bank = getBankIndex(&s->CHR_cache_8K, s->ptr_CHR_bank_8K);
        state.write((uint8_t*)&CHR_bank, sizeof(CHR_bank));
        return;
    case ROMBackend::FLASH:
        CHR_bank = (s->ptr_CHR_bank_8K - (uint8_t*)s->mROM->chr_base) / (8U * 1024U);
        state.write((uint8_t*)&CHR_bank, sizeof(CHR_bank));
        return;
    }
}

void mapper003_loadState(Mapper* mapper, File& state)
{
    Mapper003_state* s = (Mapper003_state*)mapper->state;
    uint8_t CHR_bank;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        state.read((uint8_t*)&CHR_bank, sizeof(CHR_bank));
        invalidateCache(&s->CHR_cache_8K);
        s->ptr_CHR_bank_8K = getBank(&s->CHR_cache_8K, CHR_bank, RomType::CHR);
        return;

    case ROMBackend::FLASH:
        state.read((uint8_t*)&CHR_bank, sizeof(CHR_bank));
        s->ptr_CHR_bank_8K = (uint8_t*)(s->mROM->chr_base + CHR_bank * (8U * 1024U));
        return;
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
        bankInit(&state->CHR_cache_8K, state->CHR_banks_8K, MAPPER003_NUM_CHR_BANKS_8K, 8U * 1024U,
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
