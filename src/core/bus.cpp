#include "bus.h"

Bus::Bus()
{
    memset(RAM, 0, sizeof(RAM));
    cpu.connectBus(this);
    cpu.apu.connectBus(this);
    ppu.connectBus(this);
}

Bus::~Bus()
{
}

IRAM_ATTR void Bus::cpuWrite(uint16_t addr, uint8_t data)
{
    if (uint8_t* p = write_pages[addr >> 8])
    {
        p[addr & 0xFF] = data;
        return;
    }
    write_handlers[addr >> 8](this, addr, data);
}

IRAM_ATTR uint8_t Bus::cpuRead(uint16_t addr)
{
    if (uint8_t* p = read_pages[addr >> 8]) return p[addr & 0xFF];
    return read_handlers[addr >> 8](this, addr);
}

void Bus::reset()
{
    for (auto& i : RAM) i = 0x00;

    buildPageTables();
    cart->reset();
    cart->mapPages(this);
    ppu.buildPPUPageTables();
    cart->mapPPUPages(&ppu);

    cpu.reset();
    ppu.reset();
}

IRAM_ATTR void Bus::clock()
{
    // 1 frame == 341 dots * 261 scanlines
    // Visible scanlines 0-239

    // Rendering 3 scanlines at a time because 1 CPU clock == 3 PPU clocks
    // and there's only 341 ppu clocks (dots) in a scanline, which is not divisible by 3.
    // Using a counter/for loop with += 341 & -= 3 is too big of a performance hit.
    // 1 scanline == ~113.67 CPU clocks, so for every 3 scanlines, two scanlines will have an extra
    // CPU clock
    for (int ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
    {
        cpu.clock(113);
        ppu.renderScanline(ppu_scanline);

        cpu.clock(114);
        ppu.renderScanline(ppu_scanline + 1);

        cpu.clock(114);
        ppu.renderScanline(ppu_scanline + 2);
    }

    // Setup for the next frame
    // Same reason as scanlines 0-239, 2/3 of scanlines will have an extra CPU clock.
    // Scanline 240
    cpu.clock(113);

    // Scanline 241-261
    ppu.setVBlank();
    cpu.clock(2501);

    ppu.clearVBlank();
    cpu.clock(114);
}

IRAM_ATTR void Bus::setPPUMirrorMode(MIRROR mirror)
{
    ppu.setMirror(mirror);
}

MIRROR Bus::getPPUMirrorMode()
{
    return ppu.getMirror();
}

IRAM_ATTR void Bus::OAM_Write(uint8_t addr, uint8_t data)
{
    ppu.ptr_sprite[addr] = data;
}

void Bus::insertCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    cpu.connectCartridge(cartridge);
    ppu.connectCartridge(cartridge);
    cart->connectBus(this);
}

void Bus::connectScreen(TFT_eSPI* screen)
{
    ptr_screen = screen;
}

void Bus::connectFramebuffer(uint8_t* framebuffer)
{
    ppu.connectFramebuffer(framebuffer);
}

IRAM_ATTR void Bus::renderImage(uint16_t scanline)
{
#ifndef COMPOSITE_VIDEO
    #ifndef DISABLE_DMA
    ptr_screen->pushPixelsDMA(ppu.ptr_display, 256 * SCANLINES_PER_BUFFER);
    #else
    ptr_screen->pushPixels(ppu.ptr_display, 256 * SCANLINES_PER_BUFFER);
    #endif
#endif
}

IRAM_ATTR void Bus::IRQ()
{
    cpu.IRQ();
}

IRAM_ATTR void Bus::NMI()
{
    cpu.NMI();
}

static void defaultWriteHandler(Bus* b, uint16_t a, uint8_t d)
{
    return;
}

static uint8_t defaultReadHandler(Bus* b, uint16_t a)
{
    return 0x00;
}

// Builds the memory map for bus read and writes
void Bus::buildPageTables()
{
    for (int p = 0; p < NUM_PAGES; p++)
    {
        read_pages[p] = nullptr;
        write_pages[p] = nullptr;
        read_handlers[p] = defaultReadHandler;
        write_handlers[p] = defaultWriteHandler;
    }

    // $0000 - $1FFF: 2KB RAM mirrored x4
    for (int p = 0x00; p <= 0x1F; p++)
    {
        uint8_t* base = &RAM[(p % 8) * PAGE_SIZE];
        read_pages[p] = base;
        write_pages[p] = base;
    }

    // $2000 - $3FFF: PPU registers, mirrored every 8 bytes
    for (int p = 0x20; p <= 0x3F; p++)
    {
        read_handlers[p] = [](Bus* b, uint16_t a) -> uint8_t { return b->ppu.cpuRead(a & 0x0007); };
        write_handlers[p] = [](Bus* b, uint16_t a, uint8_t d) { b->ppu.cpuWrite(a & 0x0007, d); };
    }

    // $4000 - $401F: APU/IO
    read_handlers[0x40] = [](Bus* b, uint16_t a) -> uint8_t
    {
        if (a == 0x4016)
        {
            uint8_t value = b->controller_state & 1;
            if (!b->controller_strobe) b->controller_state >>= 1;
            return value | 0x40;
        }
        return 0x00;
    };
    write_handlers[0x40] = [](Bus* b, uint16_t a, uint8_t d)
    {
        if (a == 0x4014) b->cpu.OAM_DMA(d);
        else if (a <= 0x4013 || a == 0x4015 || a == 0x4017) b->cpu.apuWrite(a, d);
        else if (a == 0x4016)
        {
            b->controller_strobe = d & 1;
            if (b->controller_strobe) b->controller_state = b->controller;
        }
    };
}

void Bus::saveState()
{
    if (!SD.exists("/states")) SD.mkdir("/states");
    uint32_t CRC32 = cart->CRC32;

    static char CRC32_str[9];
    sprintf(CRC32_str, "%08lX", (unsigned long)CRC32);

    static char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);

    File state = SD.open(filename, FILE_WRITE);
    if (!state) return;

    // Header for verification - ANEMOIA + CRC32
    state.print("ANEMOIA");
    state.write((const uint8_t*)CRC32_str, 8);

    // Dump state
    state.write(RAM, sizeof(RAM));
    cpu.dumpState(state);
    ppu.dumpState(state);
    cart->dumpState(state);

    state.close();
}

void Bus::loadState()
{
    uint32_t CRC32 = cart->CRC32;

    static char CRC32_str[9];
    sprintf(CRC32_str, "%08lX", (unsigned long)CRC32);

    static char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);
    if (!SD.exists(filename)) return;

    File state = SD.open(filename, FILE_READ);
    if (!state) return;

    // Verify header
    static char header[8];
    static char CRC[9];
    state.read((uint8_t*)&header, 7);
    header[7] = '\0';
    state.read((uint8_t*)&CRC, 8);
    CRC[8] = '\0';

    if (strcmp(header, "ANEMOIA") != 0)
    {
        state.close();
        return;
    }
    if (strcmp(CRC, CRC32_str) != 0)
    {
        state.close();
        return;
    }

    // Load state
    state.read(RAM, sizeof(RAM));
    cpu.loadState(state);
    ppu.loadState(state);
    cart->loadState(state);

    state.close();
}
