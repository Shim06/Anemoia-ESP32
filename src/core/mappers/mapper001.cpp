#include "mapper001.h"
#include "../cartridge.h"

struct Mapper001_state
{
    Cartridge* cart;
    uint8_t* RAM;
    uint8_t* CHR_RAM;

    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* ptr_16K_PRG_banks[4];
    uint8_t* ptr_8K_CHR_bank;
    uint8_t* ptr_4K_CHR_banks[2];

    Bank PRG_banks_16K[MAPPER001_NUM_PRG_BANKS_16K];
    Bank CHR_banks_8K[MAPPER001_NUM_CHR_BANKS_8K];
    Bank CHR_banks_4K[MAPPER001_NUM_CHR_BANKS_4K];
    BankCache PRG_16K_cache;
    BankCache CHR_8K_cache;
    BankCache CHR_4K_cache;

    uint8_t load = 0x00;     // Load Register
    uint8_t control = 0x1C;  // Control register
    uint8_t load_writes = 0; // Keeps track of number of writes to load register
                             // 5 writes == move data to register

    uint8_t PRG_ROM_bank_mode = 0x03;
    uint8_t CHR_ROM_bank_mode = 0;
    uint8_t CHR_bank_0 = 0x00; // CHR Bank 0 Register
    uint8_t CHR_bank_1 = 0x00; // CHR Bank 1 Register
    uint8_t PRG_bank = 0x00;   // PRG Bank Register

    static constexpr Cartridge::MIRROR mirror[4] = { Cartridge::MIRROR::ONESCREEN_LOW,
                                                     Cartridge::MIRROR::ONESCREEN_HIGH,
                                                     Cartridge::MIRROR::VERTICAL,
                                                     Cartridge::MIRROR::HORIZONTAL };
};
constexpr Cartridge::MIRROR Mapper001_state::mirror[4];

IRAM_ATTR bool mapper001_cpuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr < 0x6000) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (addr < 0x8000)
    {
        data = state->RAM[addr & 0x1FFF];
        return true;
    }

    if (state->PRG_ROM_bank_mode < 2)
    {
        uint8_t bank = ((addr >> 14) & 0x01) + 2;
        data = state->ptr_16K_PRG_banks[bank][addr & 0x3FFF];
        return true;
    }

    data = state->ptr_16K_PRG_banks[(addr >> 14) & 1][addr & 0x3FFF];
    return true;
}

IRAM_ATTR bool mapper001_cpuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr < 0x6000) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (addr < 0x8000)
    {
        state->RAM[addr & 0x1FFF] = data;
        return true;
    }

    // If bit 7 is set clear load shift register
    if (!(data & 0x80))
    {
        state->load = (state->load >> 1) | ((data & 0x01) << 4);
        state->load_writes++;

        if (state->load_writes == 5)
        {
            // Write data to register
            switch ((addr >> 13) & 0x03)
            {
            // Control Register
            case 0:
                state->control = state->load & 0x1F;
                state->CHR_ROM_bank_mode = (state->control >> 4) & 0x01;
                state->PRG_ROM_bank_mode = (state->control >> 2) & 0x03;

                // Set mirror mode
                state->cart->setMirrorMode(state->mirror[state->control & 0x03]);
                break;

            // CHR bank 0 Register
            case 1:
                state->CHR_bank_0 = state->load & 0x1F;

                if (state->CHR_ROM_bank_mode == 0)
                {
                    if (state->number_CHR_banks == 0)
                        state->cart->loadCHRBank(state->ptr_8K_CHR_bank, 8U * 1024U,
                                                 (state->CHR_bank_0 & 0x1E) * 8U * 1024U);
                    else
                        state->ptr_8K_CHR_bank =
                            getBank(&state->CHR_8K_cache, state->CHR_bank_0 & 0x1E,
                                    Mapper::ROM_TYPE::CHR_ROM);
                }
                else
                {
                    if (state->number_CHR_banks == 0)
                        state->cart->loadCHRBank(state->ptr_4K_CHR_banks[0], 4 * 1024,
                                                 state->CHR_bank_0 * 4 * 1024);
                    else
                        state->ptr_4K_CHR_banks[0] = getBank(
                            &state->CHR_4K_cache, state->CHR_bank_0, Mapper::ROM_TYPE::CHR_ROM);
                }
                break;

            // CHR bank 1 Register
            case 2:
                state->CHR_bank_1 = state->load & 0x1F;

                if (state->CHR_ROM_bank_mode == 1)
                {
                    if (state->number_CHR_banks == 0)
                        state->cart->loadCHRBank(state->ptr_4K_CHR_banks[1], 4 * 1024,
                                                 state->CHR_bank_1 * 4 * 1024);
                    else
                        state->ptr_4K_CHR_banks[1] = getBank(
                            &state->CHR_4K_cache, state->CHR_bank_1, Mapper::ROM_TYPE::CHR_ROM);
                }
                break;

            // PRG bank Register
            case 3:
                state->PRG_bank = state->load & 0x1F;

                switch (state->PRG_ROM_bank_mode)
                {
                case 0:
                case 1:
                    state->ptr_16K_PRG_banks[2] = getBank(
                        &state->PRG_16K_cache, state->PRG_bank & 0x0E, Mapper::ROM_TYPE::PRG_ROM);
                    state->ptr_16K_PRG_banks[3] =
                        getBank(&state->PRG_16K_cache, (state->PRG_bank & 0x0E) + 1,
                                Mapper::ROM_TYPE::PRG_ROM);
                    break;
                case 2:
                    state->ptr_16K_PRG_banks[0] =
                        getBank(&state->PRG_16K_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
                    state->ptr_16K_PRG_banks[1] = getBank(
                        &state->PRG_16K_cache, state->PRG_bank & 0x0F, Mapper::ROM_TYPE::PRG_ROM);
                    break;
                case 3:
                    state->ptr_16K_PRG_banks[0] = getBank(
                        &state->PRG_16K_cache, state->PRG_bank & 0x0F, Mapper::ROM_TYPE::PRG_ROM);
                    state->ptr_16K_PRG_banks[1] =
                        getBank(&state->PRG_16K_cache, state->number_PRG_banks - 1,
                                Mapper::ROM_TYPE::PRG_ROM);
                    break;
                default: break;
                }
                break;
            default: break;
            }

            // Reset Load Register and counter
            state->load = 0x00;
            state->load_writes = 0;
        }
    }
    else
    {
        state->load = 0x00;
        state->load_writes = 0;
        state->control |= 0x0C;
    }
    return true;
}

IRAM_ATTR bool mapper001_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->CHR_ROM_bank_mode == 0) { data = state->ptr_8K_CHR_bank[addr & 0x1FFF]; }
    else { data = state->ptr_4K_CHR_banks[(addr >> 12) & 1][addr & 0x0FFF]; }
    return true;
}

IRAM_ATTR bool mapper001_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
{
    if (addr > 0x1FFF) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->number_CHR_banks == 0)
    {
        // Treat as RAM
        state->CHR_RAM[addr & 0x1FFF] = data;
        return true;
    }

    return false;
}

IRAM_ATTR uint8_t* mapper001_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->CHR_ROM_bank_mode == 0) { return &state->ptr_8K_CHR_bank[addr & 0x1FFF]; }
    else { return &state->ptr_4K_CHR_banks[(addr >> 12) & 1][addr & 0x0FFF]; }
}

void mapper001_reset(Mapper* mapper)
{
    Mapper001_state* state = (Mapper001_state*)mapper->state;
    memset(state->RAM, 0, 8U * 1024U);

    if (state->number_CHR_banks == 0)
    {
        // Point 4K banks into the same memory
        state->ptr_8K_CHR_bank = state->CHR_RAM;
        state->ptr_4K_CHR_banks[0] = state->CHR_RAM;
        state->ptr_4K_CHR_banks[1] = state->CHR_RAM + 0x1000;
    }
    else
    {
        state->ptr_8K_CHR_bank = getBank(&state->CHR_8K_cache, 0, Mapper::ROM_TYPE::CHR_ROM);
        state->ptr_4K_CHR_banks[0] = getBank(&state->CHR_4K_cache, 0, Mapper::ROM_TYPE::CHR_ROM);
        state->ptr_4K_CHR_banks[1] = getBank(&state->CHR_4K_cache, 0, Mapper::ROM_TYPE::CHR_ROM);
    }

    state->ptr_16K_PRG_banks[0] = getBank(&state->PRG_16K_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[1] =
        getBank(&state->PRG_16K_cache, state->number_PRG_banks - 1, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[2] = getBank(&state->PRG_16K_cache, 0, Mapper::ROM_TYPE::PRG_ROM);
    state->ptr_16K_PRG_banks[3] = getBank(&state->PRG_16K_cache, 1, Mapper::ROM_TYPE::PRG_ROM);

    state->load = 0x00;
    state->control = 0x1C;
    state->load_writes = 0;
    state->PRG_ROM_bank_mode = 0x03;
    state->CHR_ROM_bank_mode = 0;
    state->CHR_bank_0 = 0x00;
    state->CHR_bank_1 = 0x00;
    state->PRG_bank = 0x00;
    state->cart->setMirrorMode(Cartridge::MIRROR::HORIZONTAL);
}

void mapper001_dumpState(Mapper* mapper, File& state)
{
    Mapper001_state* s = (Mapper001_state*)mapper->state;
    Cartridge::MIRROR mirror = s->cart->getMirrorMode();
    state.write((uint8_t*)&s->load, sizeof(s->load));
    state.write((uint8_t*)&s->control, sizeof(s->control));
    state.write((uint8_t*)&s->load_writes, sizeof(s->load_writes));
    state.write((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
    state.write((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));
    state.write((uint8_t*)&s->CHR_bank_0, sizeof(s->CHR_bank_0));
    state.write((uint8_t*)&s->CHR_bank_1, sizeof(s->CHR_bank_1));
    state.write((uint8_t*)&s->PRG_bank, sizeof(s->PRG_bank));
    state.write((uint8_t*)&mirror, sizeof(mirror));
    state.write(s->RAM, 8U * 1024U);

    uint8_t PRG_16K[4];
    uint8_t CHR_8K;
    uint8_t CHR_4K[2];
    for (int i = 0; i < 4; i++)
        PRG_16K[i] = getBankIndex(&s->PRG_16K_cache, s->ptr_16K_PRG_banks[i]);
    state.write(PRG_16K, sizeof(PRG_16K));
    if (s->number_CHR_banks == 0) { state.write(s->CHR_RAM, 8U * 1024U); }
    else
    {
        CHR_8K = getBankIndex(&s->CHR_8K_cache, s->ptr_8K_CHR_bank);
        for (int i = 0; i < 2; i++)
            CHR_4K[i] = getBankIndex(&s->CHR_4K_cache, s->ptr_4K_CHR_banks[i]);

        state.write((uint8_t*)&CHR_8K, sizeof(CHR_8K));
        state.write(CHR_4K, sizeof(CHR_4K));
    }
}

void mapper001_loadState(Mapper* mapper, File& state)
{
    Mapper001_state* s = (Mapper001_state*)mapper->state;
    Cartridge::MIRROR mirror;
    state.read((uint8_t*)&s->load, sizeof(s->load));
    state.read((uint8_t*)&s->control, sizeof(s->control));
    state.read((uint8_t*)&s->load_writes, sizeof(s->load_writes));
    state.read((uint8_t*)&s->PRG_ROM_bank_mode, sizeof(s->PRG_ROM_bank_mode));
    state.read((uint8_t*)&s->CHR_ROM_bank_mode, sizeof(s->CHR_ROM_bank_mode));
    state.read((uint8_t*)&s->CHR_bank_0, sizeof(s->CHR_bank_0));
    state.read((uint8_t*)&s->CHR_bank_1, sizeof(s->CHR_bank_1));
    state.read((uint8_t*)&s->PRG_bank, sizeof(s->PRG_bank));
    state.read((uint8_t*)&mirror, sizeof(mirror));
    state.read(s->RAM, 8U * 1024U);
    s->cart->setMirrorMode(mirror);

    uint8_t PRG_16K[4];
    uint8_t CHR_8K;
    uint8_t CHR_4K[2];
    state.read(PRG_16K, sizeof(PRG_16K));
    invalidateCache(&s->PRG_16K_cache);
    for (int i = 0; i < 4; i++)
        s->ptr_16K_PRG_banks[i] = getBank(&s->PRG_16K_cache, PRG_16K[i], Mapper::ROM_TYPE::PRG_ROM);
    if (s->number_CHR_banks == 0) { state.read(s->CHR_RAM, 8U * 1024U); }
    else
    {
        state.read((uint8_t*)&CHR_8K, sizeof(CHR_8K));
        state.read(CHR_4K, sizeof(CHR_4K));

        invalidateCache(&s->CHR_8K_cache);
        invalidateCache(&s->CHR_4K_cache);
        s->ptr_8K_CHR_bank = getBank(&s->CHR_8K_cache, CHR_8K, Mapper::ROM_TYPE::CHR_ROM);
        for (int i = 0; i < 2; i++)
            s->ptr_4K_CHR_banks[i] =
                getBank(&s->CHR_4K_cache, CHR_4K[i], Mapper::ROM_TYPE::CHR_ROM);
    }
}

Mapper createMapper001(uint8_t PRG_banks, uint8_t CHR_banks, Cartridge* cart)
{
    Mapper mapper;
    Mapper001_state* state = new Mapper001_state;

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;

    bankInit(&state->PRG_16K_cache, state->PRG_banks_16K, MAPPER001_NUM_PRG_BANKS_16K, 16U * 1024U,
             cart);
    state->RAM = (uint8_t*)malloc(8U * 1024U);

    if (CHR_banks == 0)
    {
        // Allocate one shared 8 KB RAM
        state->CHR_RAM = (uint8_t*)malloc(8U * 1024U);
        memset(state->CHR_RAM, 0, 8U * 1024U);
    }
    else
    {
        bankInit(&state->CHR_8K_cache, state->CHR_banks_8K, MAPPER001_NUM_CHR_BANKS_8K, 8U * 1024U,
                 cart);
        bankInit(&state->CHR_4K_cache, state->CHR_banks_4K, MAPPER001_NUM_CHR_BANKS_4K, 4 * 1024,
                 cart);
    }

    mapper.state = state;
    return mapper;
}