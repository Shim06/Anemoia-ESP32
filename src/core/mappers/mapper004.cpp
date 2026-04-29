#include "mapper004.h"
#include "../cartridge.h"

struct Mapper004_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* RAM;

    uint8_t bank_register[8];
    uint8_t* ptr_PRG_bank_8K[4];
    uint8_t* ptr_CHR_bank_1K[8];

    Bank PRG_banks_8K[MAPPER004_NUM_PRG_BANKS_8K];
    Bank CHR_banks_1K[MAPPER004_NUM_CHR_BANKS_1K];
    BankCache PRG_cache_8K;
    BankCache CHR_cache_1K;

    uint8_t bank_select = 0x00; // Bank select register
    uint8_t IRQ_latch = 0x00;   // IRQ latch register
    uint8_t IRQ_counter = 0x00;
    bool IRQ_enable = false; // IRQ enable/disable register

    uint8_t PRG_ROM_bank_mode = 0;
    uint8_t CHR_ROM_bank_mode = 0;
    uint16_t PRG_mask = 0;
    uint16_t CHR_mask = 0;

    static constexpr Cartridge::MIRROR mirror[2] = { Cartridge::MIRROR::VERTICAL,
                                                     Cartridge::MIRROR::HORIZONTAL };
};
constexpr Cartridge::MIRROR Mapper004_state::mirror[2];

IRAM_ATTR bool mapper004_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x6000) return false;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    if (addr < 0x8000)
    {
        data = state->RAM[addr & 0x1FFF];
        return true;
    }

    uint8_t bank = (addr >> 13) & 0x03;
    data = state->ptr_PRG_bank_8K[bank][addr & 0x1FFF];
    return true;
}

IRAM_ATTR bool mapper004_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x6000) return false;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    if (addr < 0x8000)
    {
        state->RAM[addr & 0x1FFF] = data;
        return true;
    }

    // Bank select (even address) | Bank data (odd address)
    uint8_t bank_register[10];
    switch (addr & 0xE001)
    {
    case 0x8000:
        state->bank_select = data & 0x07;
        state->PRG_ROM_bank_mode = (data >> 6) & 0x01;
        state->CHR_ROM_bank_mode = (data >> 7) & 0x01;
        break;

    case 0x8001:
        state->bank_register[state->bank_select] = data;

        bank_register[0] = (state->bank_register[0] & 0xFE) & state->CHR_mask;
        bank_register[1] = (state->bank_register[1] & 0xFE) & state->CHR_mask;
        bank_register[2] = state->bank_register[2] & state->CHR_mask;
        bank_register[3] = state->bank_register[3] & state->CHR_mask;
        bank_register[4] = state->bank_register[4] & state->CHR_mask;
        bank_register[5] = state->bank_register[5] & state->CHR_mask;
        bank_register[6] = state->bank_register[6] & state->PRG_mask;
        bank_register[7] = state->bank_register[7] & state->PRG_mask;
        bank_register[8] = ((state->bank_register[0] & 0xFE) + 1) & state->CHR_mask;
        bank_register[9] = ((state->bank_register[1] & 0xFE) + 1) & state->CHR_mask;
        if (state->CHR_ROM_bank_mode)
        {
            state->ptr_CHR_bank_1K[0] =
                getBank(&state->CHR_cache_1K, bank_register[2], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[1] =
                getBank(&state->CHR_cache_1K, bank_register[3], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[2] =
                getBank(&state->CHR_cache_1K, bank_register[4], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[3] =
                getBank(&state->CHR_cache_1K, bank_register[5], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[4] =
                getBank(&state->CHR_cache_1K, bank_register[0], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[5] =
                getBank(&state->CHR_cache_1K, bank_register[8], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[6] =
                getBank(&state->CHR_cache_1K, bank_register[1], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[7] =
                getBank(&state->CHR_cache_1K, bank_register[9], Mapper::ROM_TYPE::CHR_ROM);
        }
        else
        {
            state->ptr_CHR_bank_1K[0] =
                getBank(&state->CHR_cache_1K, bank_register[0], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[1] =
                getBank(&state->CHR_cache_1K, bank_register[8], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[2] =
                getBank(&state->CHR_cache_1K, bank_register[1], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[3] =
                getBank(&state->CHR_cache_1K, bank_register[9], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[4] =
                getBank(&state->CHR_cache_1K, bank_register[2], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[5] =
                getBank(&state->CHR_cache_1K, bank_register[3], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[6] =
                getBank(&state->CHR_cache_1K, bank_register[4], Mapper::ROM_TYPE::CHR_ROM);
            state->ptr_CHR_bank_1K[7] =
                getBank(&state->CHR_cache_1K, bank_register[5], Mapper::ROM_TYPE::CHR_ROM);
        }

        if (state->PRG_ROM_bank_mode)
        {
            state->ptr_PRG_bank_8K[0] = getBank(
                &state->PRG_cache_8K, (state->number_PRG_banks * 2) - 2, Mapper::ROM_TYPE::PRG_ROM);
            state->ptr_PRG_bank_8K[2] =
                getBank(&state->PRG_cache_8K, bank_register[6], Mapper::ROM_TYPE::PRG_ROM);
        }
        else
        {
            state->ptr_PRG_bank_8K[0] =
                getBank(&state->PRG_cache_8K, bank_register[6], Mapper::ROM_TYPE::PRG_ROM);
            state->ptr_PRG_bank_8K[2] = getBank(
                &state->PRG_cache_8K, (state->number_PRG_banks * 2) - 2, Mapper::ROM_TYPE::PRG_ROM);
        }
        state->ptr_PRG_bank_8K[1] =
            getBank(&state->PRG_cache_8K, bank_register[7], Mapper::ROM_TYPE::PRG_ROM);
        state->ptr_PRG_bank_8K[3] = getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 1,
                                            Mapper::ROM_TYPE::PRG_ROM);
        break;

    // Mirroring (even address)
    case 0xA000: state->cart->setMirrorMode(state->mirror[data & 0x01]); break;

    // IRQ latch (even address) | IRQ reload (odd address)
    case 0xC000: state->IRQ_latch = data; break;

    case 0xC001: state->IRQ_counter = 0; break;

    // IRQ disable (even address) | IRQ enable (odd address)
    case 0xE000: state->IRQ_enable = false; break;

    case 0xE001: state->IRQ_enable = true; break;
    }

    return false;
}

IRAM_ATTR bool mapper004_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
    data = state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
    return true;
}

IRAM_ATTR bool mapper004_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    return false;
}

IRAM_ATTR uint8_t* mapper004_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper004_state* state = (Mapper004_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
    return &state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
}

IRAM_ATTR void mapper004_scanline(Mapper* mapper)
{
    Mapper004_state* state = (Mapper004_state*)mapper->state;
    if (state->IRQ_counter == 0) state->IRQ_counter = state->IRQ_latch;
    else
    {
        state->IRQ_counter--;
        if ((state->IRQ_counter == 0) && state->IRQ_enable) state->cart->IRQ();
    }
}

void mapper004_reset(Mapper* mapper)
{
    Mapper004_state* state = (Mapper004_state*)mapper->state;
    memset(state->RAM, 0, 8 * 1024);
    state->ptr_PRG_bank_8K[0] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_PRG_bank_8K[1] = getBank(&state->PRG_cache_8K, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_PRG_bank_8K[2] =
        getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 2, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_PRG_bank_8K[3] =
        getBank(&state->PRG_cache_8K, (state->number_PRG_banks * 2) - 1, Mapper::ROM_TYPE::PRG_ROM);

    state->ptr_CHR_bank_1K[0] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[1] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[2] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[3] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[4] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[5] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[6] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);
    state->ptr_CHR_bank_1K[7] = getBank(&state->CHR_cache_1K, 0, Mapper::ROM_TYPE::CHR_ROM);

    state->bank_select = 0x00;
    state->IRQ_latch = 0x00;
    state->IRQ_counter = 0x00;
    state->IRQ_enable = false;
    state->PRG_ROM_bank_mode = 0;
    state->CHR_ROM_bank_mode = 0;
    state->PRG_mask = (state->number_PRG_banks * 2) - 1;
    state->CHR_mask = (state->number_CHR_banks * 8) - 1;
    state->cart->setMirrorMode(Cartridge::MIRROR::HORIZONTAL);
}

void mapper004_dumpState(Mapper* mapper, File& state)
{
    Mapper004_state* s = (Mapper004_state*)mapper->state;
    state.write(s->bank_register, sizeof(s->bank_register));
    state.write((uint8_t*)&s->bank_select, sizeof(s->bank_select));
    state.write((uint8_t*)&s->IRQ_latch, sizeof(s->IRQ_latch));
    state.write((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
    state.write((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
    state.write((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
    state.write((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));

    Cartridge::MIRROR mirror = s->cart->getMirrorMode();
    state.write((uint8_t*)&mirror, sizeof(mirror));

    uint8_t PRG_bank_8K[4];
    uint8_t CHR_bank_1K[8];
    for (int i = 0; i < 4; i++)
        PRG_bank_8K[i] = getBankIndex(&s->PRG_cache_8K, s->ptr_PRG_bank_8K[i]);
    for (int i = 0; i < 8; i++)
        CHR_bank_1K[i] = getBankIndex(&s->CHR_cache_1K, s->ptr_CHR_bank_1K[i]);
    state.write(PRG_bank_8K, sizeof(PRG_bank_8K));
    state.write(CHR_bank_1K, sizeof(CHR_bank_1K));
    state.write(s->RAM, 8 * 1024);
}

void mapper004_loadState(Mapper* mapper, File& state)
{
    Mapper004_state* s = (Mapper004_state*)mapper->state;
    state.read(s->bank_register, sizeof(s->bank_register));
    state.read((uint8_t*)&s->bank_select, sizeof(s->bank_select));
    state.read((uint8_t*)&s->IRQ_latch, sizeof(s->IRQ_latch));
    state.read((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
    state.read((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
    state.read((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
    state.read((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));

    Cartridge::MIRROR mirror;
    state.read((uint8_t*)&mirror, sizeof(mirror));
    s->cart->setMirrorMode(mirror);

    uint8_t PRG_bank_8K[4];
    uint8_t CHR_bank_1K[8];
    state.read(PRG_bank_8K, sizeof(PRG_bank_8K));
    state.read(CHR_bank_1K, sizeof(CHR_bank_1K));

    invalidateCache(&s->PRG_cache_8K);
    invalidateCache(&s->CHR_cache_1K);
    for (int i = 0; i < 4; i++)
        s->ptr_PRG_bank_8K[i] =
            getBank(&s->PRG_cache_8K, PRG_bank_8K[i], Mapper::ROM_TYPE::PRG_ROM);
    for (int i = 0; i < 8; i++)
        s->ptr_CHR_bank_1K[i] =
            getBank(&s->CHR_cache_1K, CHR_bank_1K[i], Mapper::ROM_TYPE::CHR_ROM);

    state.read(s->RAM, 8 * 1024);
}

Mapper createMapper004(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    Mapper004_state* state = new Mapper004_state;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    bankInit(&state->PRG_cache_8K, state->PRG_banks_8K, MAPPER004_NUM_PRG_BANKS_8K, 8 * 1024, cart);
    bankInit(&state->CHR_cache_1K, state->CHR_banks_1K, MAPPER004_NUM_CHR_BANKS_1K, 1 * 1024, cart);
    state->RAM = (uint8_t*)malloc(8 * 1024);

    mapper.state = state;
    return mapper;
}