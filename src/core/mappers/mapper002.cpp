#include "mapper002.h"
#include "../cartridge.h"

IRAM_ATTR bool mapper002_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    uint16_t offset = addr & 0x3FFF;
    uint8_t bank_id = (addr >> 14) & 1;
    data = state->ptr_16K_PRG_banks[bank_id][offset];
    return true;
}

IRAM_ATTR bool mapper002_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x8000) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    uint8_t bank = data & 0x0F;
    if (state->backend == ROMBackend::LRU)
        state->ptr_16K_PRG_banks[0] = getBank(&state->prg_cache, bank, RomType::PRG);
    else state->ptr_16K_PRG_banks[0] = (uint8_t*)(state->mROM->prg_base + (bank * 16U * 1024U));
    return true;
}

IRAM_ATTR bool mapper002_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    data = state->CHR_bank[addr];
    return true;
}

IRAM_ATTR bool mapper002_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_bank[addr] = data;
        return true;
    }

    return false;
}

IRAM_ATTR uint8_t* mapper002_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper002_state* state = (Mapper002_state*)mapper->state;
    return &state->CHR_bank[addr];
}

void mapper002_reset(Mapper* mapper)
{
    Mapper002_state* state = (Mapper002_state*)mapper->state;
    switch (state->backend)
    {
    case ROMBackend::LRU:
        state->ptr_16K_PRG_banks[0] = getBank(&state->prg_cache, 0, RomType::PRG);
        state->ptr_16K_PRG_banks[1] = state->PRG_bank;

        state->cart->loadPRGBank(state->ptr_16K_PRG_banks[1], 16U * 1024U,
                                 0x4000 * (state->number_PRG_banks - 1));
        state->cart->loadCHRBank(state->CHR_bank, 8U * 1024U, 0);
        return;

    case ROMBackend::FLASH:
        state->ptr_16K_PRG_banks[0] = (uint8_t*)state->mROM->prg_base;
        state->ptr_16K_PRG_banks[1] =
            (uint8_t*)(state->mROM->prg_base + (state->mROM->prg_size - (16U * 1024U)));

        if (state->number_CHR_banks != 0) state->CHR_bank = (uint8_t*)state->mROM->chr_base;
        return;
    }
}

void mapper002_dumpState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;
    uint8_t PRG_16K;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        PRG_16K = getBankIndex(&s->prg_cache, s->ptr_16K_PRG_banks[0]);
        state.write((uint8_t*)&PRG_16K, sizeof(PRG_16K));
        if (s->number_CHR_banks == 0) { state.write(s->CHR_bank, sizeof(s->CHR_bank)); }
        return;

    case ROMBackend::FLASH: return;
    }
}

void mapper002_loadState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;
    uint8_t PRG_16K;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        state.read((uint8_t*)&PRG_16K, sizeof(PRG_16K));
        invalidateCache(&s->prg_cache);
        s->ptr_16K_PRG_banks[0] = getBank(&s->prg_cache, PRG_16K, RomType::PRG);
        if (s->number_CHR_banks == 0) { state.read(s->CHR_bank, sizeof(s->CHR_bank)); }
        return;

    case ROMBackend::FLASH: return;
    }
}

Mapper createMapper002(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart)
{
    Mapper mapper;
    Mapper002_state* state = new Mapper002_state;
    switch (backend)
    {
    case ROMBackend::LRU:
        state->PRG_bank = (uint8_t*)malloc(16U * 1024U);
        state->CHR_bank = (uint8_t*)malloc(8U * 1024U);
        bankInit(&state->prg_cache, state->prg_banks, MAPPER002_NUM_PRG_BANKS_16K, 16U * 1024U,
                 cart);
        break;

    case ROMBackend::FLASH:
        if (CHR_banks == 0) state->CHR_bank = (uint8_t*)malloc(8U * 1024U);
        state->mROM = &cart->mROM;
        break;
    }

    state->backend = backend;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;
    mapper.state = state;
    return mapper;
}