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
    if (cart->cpuWrite(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {
        RAM[addr & 0x07FF] = data;
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        ppu.cpuWrite(addr & 0x0007, data);
    }
    else if ((addr & 0xF000) == 0x4000 && (addr <= 0x4013 || addr == 0x4015 || addr == 0x4017))
    {
        cpu.apuWrite(addr, data);
    }
    else if (addr == 0x4014)
    {
        cpu.OAM_DMA(data);
    }
    else if (addr == 0x4016)
    {
        controller_strobe = data & 1;
        if (controller_strobe)
        {
            controller_state = controller;
        }
    }
}

IRAM_ATTR uint8_t Bus::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;

    if (cart->cpuRead(addr, data)) {}
    else if ((addr & 0xE000) == 0x0000)
    {   
        data = RAM[addr & 0x07FF];
    }
    else if ((addr & 0xE000) == 0x2000)
    {
        data = ppu.cpuRead(addr & 0x0007);
    }
    else if (addr == 0x4016)
    {
        uint8_t value = controller_state & 1;
        if (!controller_strobe)
            controller_state >>= 1;
        data = value | 0x40;
    }
    return data;
}

void Bus::reset()
{
    ptr_screen->fillScreen(TFT_BLACK);
	for (auto& i : RAM) i = 0x00;
    cart->reset();
	cpu.reset();
    cpu.apu.reset();
	ppu.reset();
}

IRAM_ATTR void Bus::clock()
{
    // 1 frame == 341 dots * 261 scanlines
    // Visible scanlines 0-239
    
    // Rendering 3 scanlines at a time because 1 CPU clock == 3 PPU clocks
    // and there's only 341 ppu clocks (dots) in a scanline, which is not divisible by 3.
    // Using a counter/for loop with += 341 & -= 3 is too big of a performance hit.
    // 1 scanline == ~113.67 CPU clocks, so for every 3 scanlines, two scanlines will have an extra CPU clock
    if (!frame_latch)
    {
        for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
        {
            cpu.clock(113);
            ppu.renderScanline(ppu_scanline);

            cpu.clock(114);
            ppu.renderScanline(ppu_scanline + 1);

            cpu.clock(114);
            ppu.renderScanline(ppu_scanline + 2);
        }
    }
    else
    {
        for (ppu_scanline = 0; ppu_scanline < 240; ppu_scanline += 3)
        {
            cpu.clock(113);
            ppu.fakeSpriteHit(ppu_scanline);

            cpu.clock(114);
            ppu.fakeSpriteHit(ppu_scanline + 1);

            cpu.clock(114);
            ppu.fakeSpriteHit(ppu_scanline + 2);
        }
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

#ifdef FRAMESKIP
    frame_latch = !frame_latch;
#endif
}

IRAM_ATTR void Bus::setPPUMirrorMode(Cartridge::MIRROR mirror)
{
    ppu.setMirror(mirror);
}

Cartridge::MIRROR Bus::getPPUMirrorMode()
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

IRAM_ATTR void Bus::renderImage(uint16_t scanline)
{
    #ifndef TFT_PARALLEL
        ptr_screen->pushPixelsDMA(ppu.ptr_display, 256 * SCANLINES_PER_BUFFER);
    #else
        ptr_screen->pushPixels(ppu.ptr_display, 256 * SCANLINES_PER_BUFFER);
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

void Bus::saveState()
{
    if (!SD.exists("/states")) SD.mkdir("/states");
    uint32_t CRC32 = cart->CRC32;

    char CRC32_str[9];
    sprintf(CRC32_str, "%08X", CRC32);

    char filename[32];
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

    char CRC32_str[9];
    sprintf(CRC32_str, "%08X", CRC32);

    char filename[32];
    sprintf(filename, "/states/%s.state", CRC32_str);
    if (!SD.exists(filename)) return;

    File state = SD.open(filename, FILE_READ);
    if (!state) return;

    // Verify header
    char header[8];
    char CRC[9];
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