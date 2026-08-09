#include "mapper001.h"
#include "../bus.h"
#include "../cartridge.h"

struct Mapper001_state
{
    Cartridge* cart = nullptr;
    MappedROM* mROM = nullptr;
    ROMBackend backend;
    uint8_t* RAM = nullptr;
    uint8_t* CHR_RAM = nullptr;

    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* ptr_PRG_bank_16K[4];
    uint8_t* ptr_CHR_bank_8K;
    uint8_t* ptr_CHR_bank_4K[2];

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
static inline uint8_t* getPRGBank(Mapper001_state* state, uint8_t index);
static inline uint8_t* getCHRBank8K(Mapper001_state* state, uint8_t index);
static inline uint8_t* getCHRBank4K(Mapper001_state* state, uint8_t index);
static inline void loadCHRRAM(Mapper001_state* state, uint8_t* bank, uint16_t size,
                              uint32_t offset);

static void mapper001_remapWindows(Mapper001_state* state, Bus* bus)
{
    uint8_t* windows[2];
    if (state->PRG_ROM_bank_mode < 2)
    {
        windows[0] = state->ptr_PRG_bank_16K[2];
        windows[1] = state->ptr_PRG_bank_16K[3];
    }
    else
    {
        windows[0] = state->ptr_PRG_bank_16K[0];
        windows[1] = state->ptr_PRG_bank_16K[1];
    }

    for (int p = 0x80; p <= 0xBF; p++) bus->read_pages[p] = windows[0] + ((p - 0x80) * 256);
    for (int p = 0xC0; p <= 0xFF; p++) bus->read_pages[p] = windows[1] + ((p - 0xC0) * 256);
}

static void mapper001_shiftWrite(Bus* bus, uint16_t addr, uint8_t data)
{
    Mapper001_state* state = (Mapper001_state*)bus->cart->mapper.state;
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
                        loadCHRRAM(state, state->ptr_CHR_bank_8K, 8U * 1024U,
                                   (state->CHR_bank_0 & 0x1E) * 8U * 1024U);
                    else state->ptr_CHR_bank_8K = getCHRBank8K(state, state->CHR_bank_0 & 0x1E);
                }
                else
                {
                    if (state->number_CHR_banks == 0)
                        loadCHRRAM(state, state->ptr_CHR_bank_4K[0], 4U * 1024,
                                   state->CHR_bank_0 * 4U * 1024);
                    else state->ptr_CHR_bank_4K[0] = getCHRBank4K(state, state->CHR_bank_0);
                }
                break;

            // CHR bank 1 Register
            case 2:
                state->CHR_bank_1 = state->load & 0x1F;

                if (state->CHR_ROM_bank_mode == 1)
                {
                    if (state->number_CHR_banks == 0)
                        loadCHRRAM(state, state->ptr_CHR_bank_4K[1], 4U * 1024,
                                   state->CHR_bank_1 * 4U * 1024);
                    else state->ptr_CHR_bank_4K[1] = getCHRBank4K(state, state->CHR_bank_1);
                }
                break;

            // PRG bank Register
            case 3:
                state->PRG_bank = state->load & 0x1F;

                switch (state->PRG_ROM_bank_mode)
                {
                case 0:
                case 1:
                    state->ptr_PRG_bank_16K[2] = getPRGBank(state, state->PRG_bank & 0x0E);
                    state->ptr_PRG_bank_16K[3] = getPRGBank(state, (state->PRG_bank & 0x0E) + 1);
                    break;
                case 2:
                    state->ptr_PRG_bank_16K[0] = getPRGBank(state, 0);
                    state->ptr_PRG_bank_16K[1] = getPRGBank(state, state->PRG_bank & 0x0F);
                    break;
                case 3:
                    state->ptr_PRG_bank_16K[0] = getPRGBank(state, state->PRG_bank & 0x0F);
                    state->ptr_PRG_bank_16K[1] = getPRGBank(state, state->number_PRG_banks - 1);
                    break;
                default: break;
                }
                break;
            default: break;
            }

            // Reset Load Register and counter
            state->load = 0x00;
            state->load_writes = 0;

            mapper001_remapWindows(state, bus);
        }
    }
    else
    {
        state->load = 0x00;
        state->load_writes = 0;
        state->control |= 0x0C;
    }
}

bool mapper001_ppuRead(Mapper* mapper, uint16_t addr, uint8_t& data)
{
    if (addr > 0x1FFF) return false;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->CHR_ROM_bank_mode == 0) { data = state->ptr_CHR_bank_8K[addr & 0x1FFF]; }
    else { data = state->ptr_CHR_bank_4K[(addr >> 12) & 1][addr & 0x0FFF]; }
    return true;
}

bool mapper001_ppuWrite(Mapper* mapper, uint16_t addr, uint8_t data)
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

uint8_t* mapper001_ppuReadPtr(Mapper* mapper, uint16_t addr)
{
    if (addr > 0x1FFF) return nullptr;

    Mapper001_state* state = (Mapper001_state*)mapper->state;
    if (state->CHR_ROM_bank_mode == 0) return &state->ptr_CHR_bank_8K[addr & 0x1FFF];
    else return &state->ptr_CHR_bank_4K[(addr >> 12) & 1][addr & 0x0FFF];
}

void mapper001_reset(Mapper* mapper)
{
    Mapper001_state* state = (Mapper001_state*)mapper->state;
    memset(state->RAM, 0, 8U * 1024U);
    if (state->CHR_RAM) memset(state->CHR_RAM, 0, 8U * 1024U);

    switch (state->backend)
    {
    case ROMBackend::LRU:
        if (state->number_CHR_banks == 0)
        {
            // Point 4K banks into the same memory
            state->ptr_CHR_bank_8K = state->CHR_RAM;
            state->ptr_CHR_bank_4K[0] = state->CHR_RAM;
            state->ptr_CHR_bank_4K[1] = state->CHR_RAM + 0x1000;
        }
        else
        {
            state->ptr_CHR_bank_8K = getBank(&state->CHR_8K_cache, 0, RomType::CHR);
            state->ptr_CHR_bank_4K[0] = getBank(&state->CHR_4K_cache, 0, RomType::CHR);
            state->ptr_CHR_bank_4K[1] = getBank(&state->CHR_4K_cache, 1, RomType::CHR);
        }

        state->ptr_PRG_bank_16K[0] = getBank(&state->PRG_16K_cache, 0, RomType::PRG);
        state->ptr_PRG_bank_16K[1] =
            getBank(&state->PRG_16K_cache, state->number_PRG_banks - 1, RomType::PRG);
        state->ptr_PRG_bank_16K[2] = getBank(&state->PRG_16K_cache, 0, RomType::PRG);
        state->ptr_PRG_bank_16K[3] = getBank(&state->PRG_16K_cache, 1, RomType::PRG);
        break;

    case ROMBackend::FLASH:
        if (state->number_CHR_banks == 0)
        {
            // Point 4K banks into the same memory
            state->ptr_CHR_bank_8K = state->CHR_RAM;
            state->ptr_CHR_bank_4K[0] = state->CHR_RAM;
            state->ptr_CHR_bank_4K[1] = state->CHR_RAM + 0x1000;
        }
        else
        {
            state->ptr_CHR_bank_8K = (uint8_t*)state->mROM->chr_base;
            state->ptr_CHR_bank_4K[0] = (uint8_t*)state->mROM->chr_base;
            state->ptr_CHR_bank_4K[1] = (uint8_t*)state->mROM->chr_base;
        }

        state->ptr_PRG_bank_16K[0] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_16K[1] =
            (uint8_t*)(state->mROM->prg_base + (state->mROM->prg_size - (16U * 1024U)));
        state->ptr_PRG_bank_16K[2] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_16K[3] = (uint8_t*)(state->mROM->prg_base + (16U * 1024U));
        break;
    }

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

void mapper001_mapPages(Mapper* mapper, Bus* bus)
{
    Mapper001_state* state = (Mapper001_state*)mapper->state;

    // $6000-$7FFF: 8KB PRG-RAM
    for (int p = 0x60; p <= 0x7F; p++)
    {
        bus->read_pages[p] = state->RAM + ((p - 0x60) * 256);
        bus->write_pages[p] = state->RAM + ((p - 0x60) * 256);
    }

    // $8000-$FFFF: Shift register
    for (int p = 0x80; p <= 0xFF; p++) bus->write_handlers[p] = mapper001_shiftWrite;

    // Map bank reads
    mapper001_remapWindows(state, bus);
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
    switch (s->backend)
    {
    case ROMBackend::LRU:
        for (int i = 0; i < 4; i++)
            PRG_16K[i] = getBankIndex(&s->PRG_16K_cache, s->ptr_PRG_bank_16K[i]);
        state.write(PRG_16K, sizeof(PRG_16K));
        if (s->number_CHR_banks == 0) { state.write(s->CHR_RAM, 8U * 1024U); }
        else
        {
            CHR_8K = getBankIndex(&s->CHR_8K_cache, s->ptr_CHR_bank_8K);
            for (int i = 0; i < 2; i++)
                CHR_4K[i] = getBankIndex(&s->CHR_4K_cache, s->ptr_CHR_bank_4K[i]);

            state.write((uint8_t*)&CHR_8K, sizeof(CHR_8K));
            state.write(CHR_4K, sizeof(CHR_4K));
        }
        return;

    case ROMBackend::FLASH:
        for (int i = 0; i < 4; i++)
        {
            PRG_16K[i] = (s->ptr_PRG_bank_16K[i] - (uint8_t*)s->mROM->prg_base) / (16U * 1024U);
        }
        state.write(PRG_16K, sizeof(PRG_16K));

        if (s->number_CHR_banks == 0) { state.write(s->CHR_RAM, 8U * 1024U); }
        else
        {
            CHR_8K = (s->ptr_CHR_bank_8K - (uint8_t*)s->mROM->chr_base) / (8U * 1024U);
            for (int i = 0; i < 2; i++)
                CHR_4K[i] = (s->ptr_CHR_bank_4K[i] - (uint8_t*)s->mROM->chr_base) / (4U * 1024U);

            state.write((uint8_t*)&CHR_8K, sizeof(CHR_8K));
            state.write(CHR_4K, sizeof(CHR_4K));
        }
        return;
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
    switch (s->backend)
    {
    case ROMBackend::LRU:
        state.read(PRG_16K, sizeof(PRG_16K));
        invalidateCache(&s->PRG_16K_cache);
        for (int i = 0; i < 4; i++)
            s->ptr_PRG_bank_16K[i] = getBank(&s->PRG_16K_cache, PRG_16K[i], RomType::PRG);
        if (s->number_CHR_banks == 0) { state.read(s->CHR_RAM, 8U * 1024U); }
        else
        {
            state.read((uint8_t*)&CHR_8K, sizeof(CHR_8K));
            state.read(CHR_4K, sizeof(CHR_4K));

            invalidateCache(&s->CHR_8K_cache);
            invalidateCache(&s->CHR_4K_cache);
            s->ptr_CHR_bank_8K = getBank(&s->CHR_8K_cache, CHR_8K, RomType::CHR);
            for (int i = 0; i < 2; i++)
                s->ptr_CHR_bank_4K[i] = getBank(&s->CHR_4K_cache, CHR_4K[i], RomType::CHR);
        }
        return;

    case ROMBackend::FLASH:
        state.read(PRG_16K, sizeof(PRG_16K));
        for (int i = 0; i < 4; i++)
        {
            s->ptr_PRG_bank_16K[i] =
                (uint8_t*)(s->mROM->prg_base + (uint32_t)PRG_16K[i] * (16U * 1024U));
        }

        if (s->number_CHR_banks == 0) { state.read(s->CHR_RAM, (8U * 1024U)); }
        else
        {
            state.read((uint8_t*)&CHR_8K, sizeof(CHR_8K));
            state.read(CHR_4K, sizeof(CHR_4K));

            s->ptr_CHR_bank_8K = (uint8_t*)(s->mROM->chr_base + (uint32_t)CHR_8K * (8U * 1024U));
            for (int i = 0; i < 2; i++)
                s->ptr_CHR_bank_4K[i] =
                    (uint8_t*)(s->mROM->chr_base + (uint32_t)CHR_4K[i] * (4U * 1024U));
        }
        return;
    }
}

Mapper createMapper001(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart)
{
    Mapper mapper;
    Mapper001_state* state = new Mapper001_state;
    switch (backend)
    {
    case ROMBackend::LRU:
        bankInit(&state->PRG_16K_cache, state->PRG_banks_16K, MAPPER001_NUM_PRG_BANKS_16K,
                 16U * 1024U, cart);

        if (CHR_banks == 0)
        {
            // Allocate one shared 8 KB RAM
            state->CHR_RAM = (uint8_t*)malloc(8U * 1024U);
            memset(state->CHR_RAM, 0, 8U * 1024U);
        }
        else
        {
            bankInit(&state->CHR_8K_cache, state->CHR_banks_8K, MAPPER001_NUM_CHR_BANKS_8K,
                     8U * 1024U, cart);
            bankInit(&state->CHR_4K_cache, state->CHR_banks_4K, MAPPER001_NUM_CHR_BANKS_4K,
                     4U * 1024, cart);
        }
        break;
    case ROMBackend::FLASH:
        state->mROM = &cart->mROM;
        if (CHR_banks == 0)
        {
            // Allocate one shared 8 KB RAM
            state->CHR_RAM = (uint8_t*)malloc(8U * 1024U);
            memset(state->CHR_RAM, 0, 8U * 1024U);
        }
        break;
    }

    state->RAM = (uint8_t*)malloc(8U * 1024U);
    state->backend = backend;
    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->cart = cart;
    mapper.state = state;
    return mapper;
}

// Helper functions
static inline uint8_t* getPRGBank(Mapper001_state* state, uint8_t index)
{
    if (state->backend == ROMBackend::LRU)
        return getBank(&state->PRG_16K_cache, index, RomType::PRG);
    return (uint8_t*)(state->mROM->prg_base + (uint32_t)index * 16U * 1024U);
}

static inline uint8_t* getCHRBank8K(Mapper001_state* state, uint8_t index)
{
    if (state->backend == ROMBackend::LRU)
        return getBank(&state->CHR_8K_cache, index, RomType::CHR);
    return (uint8_t*)(state->mROM->chr_base + (uint32_t)index * 8U * 1024U);
}

static inline uint8_t* getCHRBank4K(Mapper001_state* state, uint8_t index)
{
    if (state->backend == ROMBackend::LRU)
        return getBank(&state->CHR_4K_cache, index, RomType::CHR);
    return (uint8_t*)(state->mROM->chr_base + (uint32_t)index * 4U * 1024U);
}

static inline void loadCHRRAM(Mapper001_state* state, uint8_t* bank, uint16_t size, uint32_t offset)
{
    if (state->backend == ROMBackend::FLASH)
        memcpy(bank, (uint8_t*)(state->mROM->chr_base + offset), size);
    else state->cart->loadCHRBank(bank, size, offset);
}
