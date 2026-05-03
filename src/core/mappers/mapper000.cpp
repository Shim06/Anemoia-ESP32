#include "mapper000.h"
#include "../cartridge.h"

IRAM_ATTR bool mapper000_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x8000) return false;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    uint16_t offset = addr & 0x3FFF;
    uint8_t bank_id = (addr >> 14) & 1;

    data = state->PRG_banks[bank_id][offset];
    return true;
}

IRAM_ATTR bool mapper000_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    return false;
}

IRAM_ATTR bool mapper000_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    data = state->CHR_bank[addr];
    return true;
}

IRAM_ATTR bool mapper000_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_bank[addr] = data;
        return true;
    }

    return false;
}

IRAM_ATTR uint8_t* mapper000_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper000_state* state = (Mapper000_state*)mapper->state;
    return &state->CHR_bank[addr];
}

void mapper000_reset(Mapper* mapper)
{
    Mapper000_state* state = (Mapper000_state*)mapper->state;
    switch (state->backend)
    {
    case ROMBackend::LRU:
        state->PRG_banks[0] = &state->PRG_ROM[0];
        state->PRG_banks[1] =
            (state->number_PRG_banks > 1) ? &state->PRG_ROM[16U * 1024U] : &state->PRG_ROM[0];
        state->CHR_bank = &state->CHR_ROM[0];

        state->cart->loadPRGBank(state->PRG_banks[0], 16U * 1024U, 0);
        if (state->number_PRG_banks > 1)
            state->cart->loadPRGBank(state->PRG_banks[1], 16U * 1024U, 16U * 1024U);
        state->cart->loadCHRBank(state->CHR_bank, 16U * 1024U, 0);
        break;
    case ROMBackend::FLASH:
        state->PRG_banks[0] = (uint8_t*)state->mROM->prg_base;
        state->PRG_banks[1] = (state->number_PRG_banks > 1)
                                  ? (uint8_t*)(state->mROM->prg_base + (16U * 1024U))
                                  : (uint8_t*)state->mROM->prg_base;
        state->CHR_bank =
            (state->number_CHR_banks == 0) ? state->CHR_ROM : (uint8_t*)state->mROM->chr_base;
    }
}

void mapper000_dumpState(Mapper* mapper, File& state)
{
    Mapper000_state* s = (Mapper000_state*)mapper->state;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        if (s->number_CHR_banks == 0 && s->CHR_bank) { state.write(s->CHR_bank, 8U * 1024U); }
        return;

    case ROMBackend::FLASH: return;
    }
}

void mapper000_loadState(Mapper* mapper, File& state)
{
    Mapper000_state* s = (Mapper000_state*)mapper->state;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        if (s->number_CHR_banks == 0 && s->CHR_bank) { state.read(s->CHR_bank, 8U * 1024U); }
        return;

    case ROMBackend::FLASH: return;
    }
}

Mapper createMapper000(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart)
{
    Mapper mapper;
    Mapper000_state* state = new Mapper000_state;
    switch (backend)
    {
    case ROMBackend::LRU:
        state->PRG_ROM = (uint8_t*)malloc(32 * 1024);
        if (state->PRG_ROM)
            LOGF("Allocated 32 KB for PRG ROM, free heap: %u bytes\n",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
        else LOG("32 KB for PRG ROM Allocation failed.");

        state->CHR_ROM = (uint8_t*)malloc(16U * 1024U);
        if (state->CHR_ROM)
            LOGF("Allocated 8 KB for CHR ROM, free heap: %u bytes\n",
                 heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
        else LOG("8 KB for CHR ROM Allocation failed.");
        break;
    case ROMBackend::FLASH:
        if (CHR_banks == 0) state->CHR_ROM = (uint8_t*)malloc(8U * 1024U);
        state->mROM = &cart->mROM;
        break;
    }

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;
    state->backend = backend;
    mapper.state = state;
    return mapper;
}