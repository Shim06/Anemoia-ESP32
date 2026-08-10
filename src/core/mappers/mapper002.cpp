#include "mapper002.h"
#include "../bus.h"
#include "../cartridge.h"

static void mapper002_remapWindows(Mapper002_state* state, Bus* bus)
{
    for (int p = 0x80; p <= 0xBF; p++)
        bus->read_pages[p] = state->ptr_PRG_bank_16K[0] + ((p - 0x80) * 256);
}

static void mapper002_bankWrite(Bus* bus, uint16_t addr, uint8_t data)
{
    Mapper002_state* state = (Mapper002_state*)bus->cart->mapper.state;

    uint8_t bank = data & 0x0F;
    if (state->backend == ROMBackend::LRU)
        state->ptr_PRG_bank_16K[0] = getBank(&state->PRG_cache_16K, bank, RomType::PRG);
    else state->ptr_PRG_bank_16K[0] = (uint8_t*)(state->mROM->prg_base + (bank * 16U * 1024U));
    mapper002_remapWindows(state, bus);
}

void mapper002_reset(Mapper* mapper)
{
    Mapper002_state* state = (Mapper002_state*)mapper->state;
    switch (state->backend)
    {
    case ROMBackend::LRU:
        state->ptr_PRG_bank_16K[0] = getBank(&state->PRG_cache_16K, 0, RomType::PRG);
        state->ptr_PRG_bank_16K[1] = state->PRG_bank;

        state->cart->loadPRGBank(state->ptr_PRG_bank_16K[1], 16U * 1024U,
                                 0x4000 * (state->number_PRG_banks - 1));
        state->cart->loadCHRBank(state->CHR_bank, 8U * 1024U, 0);
        return;

    case ROMBackend::FLASH:
        state->ptr_PRG_bank_16K[0] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_16K[1] =
            (uint8_t*)(state->mROM->prg_base + (state->mROM->prg_size - (16U * 1024U)));

        if (state->number_CHR_banks != 0) state->CHR_bank = (uint8_t*)state->mROM->chr_base;
        return;
    }
}

void mapper002_mapPages(Mapper* mapper, Bus* bus)
{
    Mapper002_state* state = (Mapper002_state*)mapper->state;

    // $8000-$FFFF: Bank switch
    for (int p = 0x80; p <= 0xFF; p++)
    {
        bus->write_pages[p] = nullptr;
        bus->write_handlers[p] = mapper002_bankWrite;
    }

    // $C000-$FFFF: fixed to last bank, never remapped by writes
    for (int p = 0xC0; p <= 0xFF; p++)
        bus->read_pages[p] = state->ptr_PRG_bank_16K[1] + ((p - 0xC0) * 256);

    // Map bank reads ($8000-$BFFF, switchable)
    mapper002_remapWindows(state, bus);
}

void mapper002_mapPPUPages(Mapper* mapper, Ppu2C02* ppu)
{
    Mapper002_state* state = (Mapper002_state*)mapper->state;
    for (int p = 0x00; p <= 0x1F; p++)
    {
        ppu->ppu_read_pages[p] = state->CHR_bank + (p * 256);
        if (state->number_CHR_banks == 0) ppu->ppu_write_pages[p] = state->CHR_bank + (p * 256);
    }
}

void mapper002_dumpState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;
    uint8_t PRG_16K;
    switch (s->backend)
    {
    case ROMBackend::LRU:
        PRG_16K = getBankIndex(&s->PRG_cache_16K, s->ptr_PRG_bank_16K[0]);
        state.write((uint8_t*)&PRG_16K, sizeof(PRG_16K));
        if (s->number_CHR_banks == 0) { state.write(s->CHR_bank, 8U * 1024U); }
        return;

    case ROMBackend::FLASH:
        PRG_16K = (s->ptr_PRG_bank_16K[0] - (uint8_t*)s->mROM->prg_base) / (16U * 1024U);
        state.write((uint8_t*)&PRG_16K, sizeof(PRG_16K));
        if (s->number_CHR_banks == 0) { state.write(s->CHR_bank, 8U * 1024U); }
        return;
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
        invalidateCache(&s->PRG_cache_16K);
        s->ptr_PRG_bank_16K[0] = getBank(&s->PRG_cache_16K, PRG_16K, RomType::PRG);
        if (s->number_CHR_banks == 0) { state.read(s->CHR_bank, 8U * 1024U); }
        return;

    case ROMBackend::FLASH:
        state.read((uint8_t*)&PRG_16K, sizeof(PRG_16K));
        s->ptr_PRG_bank_16K[0] = (uint8_t*)(s->mROM->prg_base + (PRG_16K * (16U * 1024U)));
        if (s->number_CHR_banks == 0) { state.read(s->CHR_bank, 8U * 1024U); }
        return;
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
        bankInit(&state->PRG_cache_16K, state->PRG_banks_16K, MAPPER002_NUM_PRG_BANKS_16K,
                 16U * 1024U, cart);
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
