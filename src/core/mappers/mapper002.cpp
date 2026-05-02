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
    state->ptr_16K_PRG_banks[0] = getBank(&state->prg_cache, bank, Mapper::ROM_TYPE::PRG_ROM);
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
    state->ptr_16K_PRG_banks[0] = getBank(&state->prg_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[1] = state->PRG_bank;

    state->cart->loadPRGBank(state->ptr_16K_PRG_banks[1], 16U * 1024U,
                             0x4000 * (state->number_PRG_banks - 1));
    state->cart->loadCHRBank(state->CHR_bank, 8U * 1024U, 0);
}

void mapper002_dumpState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;

    uint8_t PRG_16K;
    PRG_16K = getBankIndex(&s->prg_cache, s->ptr_16K_PRG_banks[0]);
    state.write((uint8_t*)&PRG_16K, sizeof(PRG_16K));
    if (s->number_CHR_banks == 0) { state.write(s->CHR_bank, sizeof(s->CHR_bank)); }
}

void mapper002_loadState(Mapper* mapper, File& state)
{
    Mapper002_state* s = (Mapper002_state*)mapper->state;

    uint8_t PRG_16K;
    state.read((uint8_t*)&PRG_16K, sizeof(PRG_16K));
    invalidateCache(&s->prg_cache);
    s->ptr_16K_PRG_banks[0] = getBank(&s->prg_cache, PRG_16K, Mapper::ROM_TYPE::PRG_ROM);
    if (s->number_CHR_banks == 0) { state.read(s->CHR_bank, sizeof(s->CHR_bank)); }
}

Mapper createMapper002(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    Mapper002_state* state = new Mapper002_state;
    bankInit(&state->prg_cache, state->prg_banks, MAPPER002_NUM_PRG_BANKS_16K, 16U * 1024U, cart);

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    mapper.state = state;
    return mapper;
}