#include "mapper069.h"
#include "../bus.h"
#include "../cartridge.h"
#include "../ppu2C02.h"

struct Mapper069_state
{
    Cartridge* cart = nullptr;
    MappedROM* mROM = nullptr;
    ROMBackend backend;
    uint8_t number_PRG_banks;
    uint8_t number_CHR_banks;
    uint8_t* RAM = nullptr;

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

    static constexpr MIRROR mirror[4] = { MIRROR::VERTICAL, MIRROR::HORIZONTAL,
                                          MIRROR::ONESCREEN_LOW, MIRROR::ONESCREEN_HIGH };
};
constexpr MIRROR Mapper069_state::mirror[4];
static inline uint8_t* getPRGBank(Mapper069_state* state, uint8_t index);
static inline uint8_t* getCHRBank(Mapper069_state* state, uint8_t index);

static void mapper069_remapWindows(Mapper069_state* state, Bus* bus)
{
    uint8_t* windows[4] = { state->ptr_PRG_bank_8K[1], state->ptr_PRG_bank_8K[2],
                            state->ptr_PRG_bank_8K[3], state->ptr_PRG_bank_8K[4] };
    for (int i = 0; i < 4; i++)
    {
        int base = 0x80 + (i * 0x20);
        for (int p = 0; p <= 0x1F; p++) bus->read_pages[base + p] = windows[i] + (p * 256);
    }
}

static void mapper069_remapCHRPages(Mapper069_state* state, Ppu2C02* ppu)
{
    uint8_t* windows[8] = { state->ptr_CHR_bank_1K[0], state->ptr_CHR_bank_1K[1],
                            state->ptr_CHR_bank_1K[2], state->ptr_CHR_bank_1K[3],
                            state->ptr_CHR_bank_1K[4], state->ptr_CHR_bank_1K[5],
                            state->ptr_CHR_bank_1K[6], state->ptr_CHR_bank_1K[7] };
    for (int i = 0; i < 8; i++)
    {
        int base = 0x00 + (i * 0x04);
        for (int p = 0; p <= 0x03; p++) ppu->ppu_read_pages[base + p] = windows[i] + (p * 256);
    }
}

static void mapper069_remapRAMWindow(Mapper069_state* state, Bus* bus)
{
    // Read side: safe to point at ROM unconditionally when RAM is deselected
    uint8_t* read_ptr = state->PRG_RAM_select ? state->RAM : state->ptr_PRG_bank_8K[0];
    for (int p = 0x60; p <= 0x7F; p++) bus->read_pages[p] = read_ptr + ((p - 0x60) * 256);

    // Write side: null falls through to mapper069_PRGWrite (no-op) when RAM disabled
    for (int p = 0x60; p <= 0x7F; p++)
    {
        if (state->PRG_RAM_select && state->PRG_RAM_enable)
            bus->write_pages[p] = state->RAM + ((p - 0x60) * 256);
        else bus->write_pages[p] = nullptr;
    }
}

static void mapper069_commandWrite(Bus* bus, uint16_t addr, uint8_t data)
{
    Mapper069_state* state = (Mapper069_state*)bus->cart->mapper.state;
    state->command_register = data & 0x0F;
}

static void mapper069_parameterWrite(Bus* bus, uint16_t addr, uint8_t data)
{
    Mapper069_state* state = (Mapper069_state*)bus->cart->mapper.state;
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
        state->ptr_CHR_bank_1K[command] = getCHRBank(state, data & state->CHR_mask);
        mapper069_remapCHRPages(state, &bus->ppu);
        break;

    case 0x08:
        state->ptr_PRG_bank_8K[command & 0x03] = getPRGBank(state, (data & 0x3F) & state->PRG_mask);
        state->PRG_RAM_select = (data & 0x40) != 0;
        state->PRG_RAM_enable = (data & 0x80) != 0;
        mapper069_remapRAMWindow(state, bus);
        break;

    case 0x09:
    case 0x0A:
    case 0x0B:
        state->ptr_PRG_bank_8K[command & 0x03] = getPRGBank(state, (data & 0x3F) & state->PRG_mask);
        mapper069_remapWindows(state, bus);
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

void mapper069_cycle(Mapper* mapper, int cycles)
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
    if (state->RAM) memset(state->RAM, 0, 16U * 1024U);
    switch (state->backend)
    {
    case ROMBackend::LRU:
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
        break;

    case ROMBackend::FLASH:
        state->ptr_PRG_bank_8K[0] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_8K[1] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_8K[2] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_8K[3] = (uint8_t*)state->mROM->prg_base;
        state->ptr_PRG_bank_8K[4] =
            (uint8_t*)(state->mROM->prg_base + (state->mROM->prg_size - (8U * 1024U)));

        state->ptr_CHR_bank_1K[0] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[1] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[2] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[3] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[4] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[5] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[6] = (uint8_t*)state->mROM->chr_base;
        state->ptr_CHR_bank_1K[7] = (uint8_t*)state->mROM->chr_base;
        break;
    }

    state->command_register = 0x00;
    state->parameter_register = 0x00;
    state->IRQ_counter = 0x0000;
    state->IRQ_counter_enable = false;
    state->IRQ_enable = false;
    state->PRG_RAM_select = false;
    state->PRG_RAM_enable = false;
    state->PRG_mask = (state->number_PRG_banks * 2) - 1;
    state->CHR_mask = (state->number_CHR_banks * 8) - 1;
    state->cart->setMirrorMode(MIRROR::HORIZONTAL);
}

void mapper069_mapPages(Mapper* mapper, Bus* bus)
{
    Mapper069_state* state = (Mapper069_state*)mapper->state;

    // $6000-$7FFF: PRG-ROM/PRG-RAM r/w
    mapper069_remapRAMWindow(state, bus);

    // Command Register ($8000-$9FFF)
    for (int p = 0x80; p <= 0x9F; p++) bus->write_handlers[p] = mapper069_commandWrite;

    // Parameter Register ($A000-$BFFF)
    for (int p = 0xA0; p <= 0xBF; p++) bus->write_handlers[p] = mapper069_parameterWrite;

    // Map bank reads
    mapper069_remapWindows(state, bus);
}

void mapper069_mapPPUPages(Mapper* mapper, Ppu2C02* ppu)
{
    Mapper069_state* state = (Mapper069_state*)mapper->state;
    mapper069_remapCHRPages(state, ppu);
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

    MIRROR mirror = s->cart->getMirrorMode();
    state.write((uint8_t*)&mirror, sizeof(mirror));

    uint8_t PRG_bank_8K[4];
    uint8_t CHR_bank_1K[8];
    switch (s->backend)
    {
    case ROMBackend::LRU:
        for (int i = 0; i < 4; i++)
            PRG_bank_8K[i] = getBankIndex(&s->PRG_cache_8K, s->ptr_PRG_bank_8K[i]);
        for (int i = 0; i < 8; i++)
            CHR_bank_1K[i] = getBankIndex(&s->CHR_cache_1K, s->ptr_CHR_bank_1K[i]);
        state.write(PRG_bank_8K, sizeof(PRG_bank_8K));
        state.write(CHR_bank_1K, sizeof(CHR_bank_1K));
        state.write(s->RAM, 8U * 1024U);
        return;

    case ROMBackend::FLASH:
        for (int i = 0; i < 4; i++)
            PRG_bank_8K[i] = (s->ptr_PRG_bank_8K[i] - (uint8_t*)s->mROM->prg_base) / (8U * 1024U);
        for (int i = 0; i < 8; i++)
            CHR_bank_1K[i] = (s->ptr_CHR_bank_1K[i] - (uint8_t*)s->mROM->chr_base) / (1U * 1024U);
        state.write(PRG_bank_8K, sizeof(PRG_bank_8K));
        state.write(CHR_bank_1K, sizeof(CHR_bank_1K));
        state.write(s->RAM, 8U * 1024U);
        return;
    }
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

    MIRROR mirror;
    state.read((uint8_t*)&mirror, sizeof(mirror));
    s->cart->setMirrorMode(mirror);

    uint8_t PRG_bank_8K[4];
    uint8_t CHR_bank_1K[8];
    switch (s->backend)
    {
    case ROMBackend::LRU:
        state.read(PRG_bank_8K, sizeof(PRG_bank_8K));
        state.read(CHR_bank_1K, sizeof(CHR_bank_1K));

        invalidateCache(&s->PRG_cache_8K);
        invalidateCache(&s->CHR_cache_1K);
        for (int i = 0; i < 4; i++)
            s->ptr_PRG_bank_8K[i] = getBank(&s->PRG_cache_8K, PRG_bank_8K[i], RomType::PRG);
        for (int i = 0; i < 8; i++)
            s->ptr_CHR_bank_1K[i] = getBank(&s->CHR_cache_1K, CHR_bank_1K[i], RomType::CHR);

        state.read(s->RAM, 8U * 1024U);
        return;

    case ROMBackend::FLASH:
        state.read(PRG_bank_8K, sizeof(PRG_bank_8K));
        state.read(CHR_bank_1K, sizeof(CHR_bank_1K));

        for (int i = 0; i < 4; i++)
            s->ptr_PRG_bank_8K[i] = (uint8_t*)(s->mROM->prg_base + (PRG_bank_8K[i] * (8U * 1024U)));
        for (int i = 0; i < 8; i++)
            s->ptr_CHR_bank_1K[i] = (uint8_t*)(s->mROM->chr_base + (CHR_bank_1K[i] * (1U * 1024U)));

        state.read(s->RAM, 8U * 1024U);
        return;
    }
}

Mapper createMapper069(uint8_t PRG_banks, uint8_t CHR_banks, ROMBackend backend, Cartridge* cart)
{
    Mapper mapper;
    Mapper069_state* state = new Mapper069_state;

    state->RAM = (uint8_t*)malloc(8U * 1024U);
    state->ptr_PRG_bank_8K[4] = (uint8_t*)malloc(8U * 1024U);

    switch (backend)
    {
    case ROMBackend::LRU:
        bankInit(&state->PRG_cache_8K, state->PRG_banks_8K, MAPPER069_NUM_PRG_BANKS_8K, 8U * 1024U,
                 cart);
        bankInit(&state->CHR_cache_1K, state->CHR_banks_1K, MAPPER069_NUM_CHR_BANKS_1K, 1U * 1024,
                 cart);
        break;

    case ROMBackend::FLASH: state->mROM = &cart->mROM; break;
    }

    state->number_PRG_banks = PRG_banks;
    state->number_CHR_banks = CHR_banks;
    state->backend = backend;
    state->cart = cart;
    mapper.state = state;
    return mapper;
}

// Helper functions
static inline uint8_t* getPRGBank(Mapper069_state* state, uint8_t index)
{
    if (state->backend == ROMBackend::LRU)
        return getBank(&state->PRG_cache_8K, index, RomType::PRG);
    return (uint8_t*)(state->mROM->prg_base + (uint32_t)index * 8U * 1024U);
}

static inline uint8_t* getCHRBank(Mapper069_state* state, uint8_t index)
{
    if (state->backend == ROMBackend::LRU)
        return getBank(&state->CHR_cache_1K, index, RomType::CHR);
    return (uint8_t*)(state->mROM->chr_base + (uint32_t)index * 1U * 1024U);
}
