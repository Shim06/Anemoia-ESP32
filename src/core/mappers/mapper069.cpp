#include "mapper069.h"
#include "../cartridge.h"

struct Mapper069_state
{
    Cartridge* cart;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* RAM;

    uint8_t* ptr_PRG_bank_8K[5];
    uint8_t* ptr_CHR_bank_1K[8];

    Bank PRG_banks_8K[MAPPER069_NUM_PRG_BANKS_8K];
    Bank CHR_banks_1K[MAPPER069_NUM_CHR_BANKS_1K];
    BankCache PRG_cache_8K;
    BankCache CHR_cache_1K;

    uint8_t command_register = 0x00;
    uint8_t parameter_register = 0x00;

    uint16_t IRQ_counter = 0x0000;
    bool IRQ_counter_enable = false;
    bool IRQ_enable = false;
    bool PRG_RAM_select = false;
    bool PRG_RAM_enable = false;
    uint16_t PRG_mask = 0;
    uint16_t CHR_mask = 0;

    static constexpr Cartridge::MIRROR mirror[4] = { Cartridge::MIRROR::VERTICAL,
                                                     Cartridge::MIRROR::HORIZONTAL,
                                                     Cartridge::MIRROR::ONESCREEN_LOW,
                                                     Cartridge::MIRROR::ONESCREEN_HIGH };
};
constexpr Cartridge::MIRROR Mapper069_state::mirror[4];

IRAM_ATTR bool mapper069_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x6000) return false;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    if (addr < 0x8000)
    {
        // PRG RAM
        if (state->PRG_RAM_select)
        {
            if (state->PRG_RAM_enable) data = state->RAM[addr & 0x1FFF];
            return true;
        }

        // PRG ROM
        data = state->ptr_PRG_bank_8K[0][addr & 0x1FFF];
        return true;
    }

    uint8_t bank = (addr >> 13) & 0x03;
    data = state->ptr_PRG_bank_8K[bank + 1][addr & 0x1FFF];
    return true;
}

IRAM_ATTR bool mapper069_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x6000) return false;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    if (addr < 0x8000)
    {
        if (state->PRG_RAM_select && state->PRG_RAM_enable) state->RAM[addr & 0x1FFF] = data;
        return true;
    }

    // Command Register ($8000-$9FFF) | Parameter Register ($A000-$BFFF)
    uint16_t masked_addr = addr & 0xE000;
    if (masked_addr == 0x8000) state->command_register = data & 0x0F;
    else if (masked_addr == 0xA000)
    {
        uint8_t command = state->command_register;
        switch (command)
        {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            state->ptr_CHR_bank_1K[command] =
                getBank(&state->CHR_cache_1K, data & state->CHR_mask, RomType::CHR);
            break;

        case 0x08:
            state->ptr_PRG_bank_8K[command & 0x03] =
                getBank(&state->PRG_cache_8K, (data & 0x3F) & state->PRG_mask, RomType::PRG);
            state->PRG_RAM_select = (data & 0x40) != 0;
            state->PRG_RAM_enable = (data & 0x80) != 0;
            break;

        case 0x09:
        case 0x0A:
        case 0x0B:
            state->ptr_PRG_bank_8K[command & 0x03] =
                getBank(&state->PRG_cache_8K, (data & 0x3F) & state->PRG_mask, RomType::PRG);
            break;

        case 0x0C: state->cart->setMirrorMode(state->mirror[data & 0x03]); break;

        case 0x0D:
            state->IRQ_enable = (data & 0x01) != 0;
            state->IRQ_counter_enable = (data & 0x80) != 0;
            break;
        case 0x0E: state->IRQ_counter = (state->IRQ_counter & 0xFF00) | data; break;
        case 0x0F: state->IRQ_counter = (state->IRQ_counter & 0x00FF) | (data << 8); break;
        }
    }
    return false;
}

IRAM_ATTR bool mapper069_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
    data = state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
    return true;
}

IRAM_ATTR bool mapper069_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    return false;
}

IRAM_ATTR uint8_t* mapper069_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper069_state* state = (Mapper069_state*)mapper->state;
    uint8_t bank = (addr >> 10) & 0x07;
    return &state->ptr_CHR_bank_1K[bank][addr & 0x03FF];
}

IRAM_ATTR void mapper069_cycle(Mapper* mapper, int cycles)
{
    Mapper069_state* state = (Mapper069_state*)mapper->state;
    if (!state->IRQ_counter_enable) return;

    // IRQ if IRQ counter underflows from 0x0000 -> 0xFFFF;
    uint16_t before = state->IRQ_counter;
    state->IRQ_counter -= cycles;
    if ((before < cycles) && state->IRQ_enable) state->cart->IRQ();
}

void mapper069_reset(Mapper* mapper)
{
    Mapper069_state* state = (Mapper069_state*)mapper->state;
    memset(state->RAM, 0, 8U * 1024U);
    state->ptr_PRG_bank_8K[0] = getBank(&state->PRG_cache_8K, 0, RomType::PRG);
    state->ptr_PRG_bank_8K[1] = getBank(&state->PRG_cache_8K, 0, RomType::PRG);
    state->ptr_PRG_bank_8K[2] = getBank(&state->PRG_cache_8K, 0, RomType::PRG);
    state->ptr_PRG_bank_8K[3] = getBank(&state->PRG_cache_8K, 0, RomType::PRG);
    state->cart->loadPRGBank(state->ptr_PRG_bank_8K[4], 8U * 1024U,
                             ((state->number_PRG_banks * 2) - 1) * 8U * 1024U);

    state->ptr_CHR_bank_1K[0] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[1] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[2] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[3] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[4] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[5] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[6] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);
    state->ptr_CHR_bank_1K[7] = getBank(&state->CHR_cache_1K, 0, RomType::CHR);

    state->command_register = 0x00;
    state->parameter_register = 0x00;
    state->IRQ_counter = 0x0000;
    state->IRQ_counter_enable = false;
    state->IRQ_enable = false;
    state->PRG_RAM_select = false;
    state->PRG_RAM_enable = false;
    state->PRG_mask = (state->number_PRG_banks * 2) - 1;
    state->CHR_mask = (state->number_CHR_banks * 8) - 1;
    state->cart->setMirrorMode(Cartridge::MIRROR::HORIZONTAL);
}

void mapper069_dumpState(Mapper* mapper, File& state)
{
    Mapper069_state* s = (Mapper069_state*)mapper->state;
    state.write((uint8_t*)&s->command_register, sizeof(s->command_register));
    state.write((uint8_t*)&s->parameter_register, sizeof(s->parameter_register));
    state.write((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
    state.write((uint8_t*)&s->IRQ_counter_enable, sizeof(s->IRQ_counter_enable));
    state.write((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
    state.write((uint8_t*)&s->PRG_RAM_select, sizeof(s->PRG_RAM_select));
    state.write((uint8_t*)&s->PRG_RAM_enable, sizeof(s->PRG_RAM_enable));

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
    state.write(s->RAM, 8U * 1024U);
}

void mapper069_loadState(Mapper* mapper, File& state)
{
    Mapper069_state* s = (Mapper069_state*)mapper->state;
    state.read((uint8_t*)&s->command_register, sizeof(s->command_register));
    state.read((uint8_t*)&s->parameter_register, sizeof(s->parameter_register));
    state.read((uint8_t*)&s->IRQ_counter, sizeof(s->IRQ_counter));
    state.read((uint8_t*)&s->IRQ_counter_enable, sizeof(s->IRQ_counter_enable));
    state.read((uint8_t*)&s->IRQ_enable, sizeof(s->IRQ_enable));
    state.read((uint8_t*)&s->PRG_RAM_select, sizeof(s->PRG_RAM_select));
    state.read((uint8_t*)&s->PRG_RAM_enable, sizeof(s->PRG_RAM_enable));

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
        s->ptr_PRG_bank_8K[i] = getBank(&s->PRG_cache_8K, PRG_bank_8K[i], RomType::PRG);
    for (int i = 0; i < 8; i++)
        s->ptr_CHR_bank_1K[i] = getBank(&s->CHR_cache_1K, CHR_bank_1K[i], RomType::CHR);

    state.read(s->RAM, 8U * 1024U);
}

Mapper createMapper069(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    Mapper069_state* state = new Mapper069_state;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    bankInit(&state->PRG_cache_8K, state->PRG_banks_8K, MAPPER069_NUM_PRG_BANKS_8K, 8U * 1024U,
             cart);
    bankInit(&state->CHR_cache_1K, state->CHR_banks_1K, MAPPER069_NUM_CHR_BANKS_1K, 1 * 1024, cart);
    state->RAM = (uint8_t*)malloc(8U * 1024U);
    state->ptr_PRG_bank_8K[4] = (uint8_t*)malloc(8U * 1024U);

    mapper.state = state;
    return mapper;
}