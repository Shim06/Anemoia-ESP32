#include "ppu2C02.h"
#include "bus.h"


#define READ_PALETTE(x) palette_table[((x) & 0x1F) ^ (((x) & 0x13) == 0x10 ? 0x10 : 0x00)]
DMA_ATTR uint16_t Ppu2C02::display_buffer[SCANLINE_SIZE * SCANLINES_PER_BUFFER];
constexpr uint8_t Ppu2C02::palette_mirror[32];

Ppu2C02::Ppu2C02()
{
    memset(nametable, 0, sizeof(nametable));
    memset(palette_table, 0, sizeof(palette_table));
    memset(scanline_buffer, 0, sizeof(scanline_buffer));
    memset(scanline_metadata, 0, sizeof(scanline_metadata));
    memset(display_buffer, 0, sizeof(display_buffer));
    memset(sprite, 0, sizeof(sprite));
}

Ppu2C02::~Ppu2C02()
{

}

inline void Ppu2C02::ppuWrite(uint16_t addr, uint8_t data)
{
    addr &= 0x3FFF;
    
    if (cart->ppuWrite(addr, data)) return;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        ptr_nametable[(addr >> 10) & 3][addr & 0x03FF] = data;
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr = palette_mirror[addr & 0x001F];
        palette_table[addr] = data;
    }
}

inline uint8_t Ppu2C02::ppuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    addr &= 0x3FFF;
    
    if (cart->ppuRead(addr, data)) return data;
    else if (addr >= 0x2000 && addr <= 0x3EFF)
    {
        data = ptr_nametable[(addr >> 10) & 3][addr & 0x03FF];
    }
    else if (addr >= 0x3F00 && addr <= 0x3FFF)
    {
        addr &= 0x001F;
        switch (addr)
        {
        case 0x0010: addr = 0x0000; break;
        case 0x0014: addr = 0x0004; break;
        case 0x0018: addr = 0x0008; break;
        case 0x001C: addr = 0x000C; break;
        }
        data = palette_table[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }

    return data;
}

IRAM_ATTR void Ppu2C02::cpuWrite(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
    case 0x2000: // PPUCTRL
        control.reg = data;
        t.nametable_x = control.nametable_x;
        t.nametable_y = control.nametable_y;
        break;
    case 0x2001: // PPUMASK
        mask.reg = data;
        break;
    case 0x2003: // OAMADDR
        OAMADDR = data;
        break;
    case 0x2004: // OAMDATA
        ptr_sprite[OAMADDR++] = data;
        break;
    case 0x2005: // PPUSCROLL
        if (w == 0)
        {
            x = data & 0x07;
            t.coarse_x = data >> 3;
        }
        else
        {
            t.fine_y = data & 0x07;
            t.coarse_y = data >> 3;
        }
        w = ~w;
        break;
    case 0x2006: // PPUADDR
        if (w == 0)
        {
            t.reg = (t.reg & 0x00FF) | (uint16_t)((data & 0x3F) << 8);
        }
        else
        {
            t.reg = (t.reg & 0xFF00) | data;
            v.reg = t.reg;
        }
        w = ~w;
        break;
    case 0x2007: // PPUDATA
        ppuWrite(v.reg, data);
        v.reg += (control.VRAM_addr_increment ? 32 : 1);
        break;
    }
}

IRAM_ATTR uint8_t Ppu2C02::cpuRead(uint16_t addr)
{
    uint8_t data = 0x00;
    switch (addr)
    {
    case 0x2002: // PPUSTATUS
        data = status.reg & 0xE0;
        status.VBlank = 0;
        w = 0;
        break;
    case 0x2004: // OAMDATA
        data = ptr_sprite[OAMADDR];
        break;
    case 0x2007: // PPUDATA
        data = PPUDATA_buffer;
        PPUDATA_buffer = ppuRead(v.reg);

        if (v.reg >= 0x3F00 && v.reg <= 0x3FFF) data = PPUDATA_buffer;
        v.reg += (control.VRAM_addr_increment ? 32 : 1);
        break;
    }

    return data;
}

IRAM_ATTR void Ppu2C02::setVBlank()
{
    status.VBlank = 1;
    if (control.Vblank_NMI) bus->NMI();
}

IRAM_ATTR void Ppu2C02::clearVBlank()
{
    status.VBlank = 0;
    status.sprite_zero_hit = 0;
    status.sprite_overflow = 0;
}

IRAM_ATTR void Ppu2C02::renderScanline(uint16_t scanline)
{
    transferScroll(scanline);
    renderBackground();
    renderSprites(scanline);
    incrementY();
    finishScanline(scanline);
}

inline void Ppu2C02::transferScroll(uint16_t scanline)
{
    if (!(mask.reg & (1 << 3) || mask.reg & (1 << 4))) return;
    v.reg = (scanline == 0) ? t.reg : v.reg = (v.reg & ~0x041F) | (t.reg & 0x041F);
}

inline void Ppu2C02::incrementY()
{
    if (!(mask.render_background || mask.render_sprite)) return;

    if (v.fine_y < 7)
    {
        v.fine_y++;
        return;
    }
    
    v.fine_y = 0;
    if (v.coarse_y == 29)
    {
        v.coarse_y = 0;
        v.nametable_y = ~v.nametable_y;
    }
    else if (v.coarse_y == 31) v.coarse_y = 0;
    else v.coarse_y++;
}

inline void Ppu2C02::renderBackground()
{   
    // Show transparency pixel if not rendering background
    if (!mask.render_background)
    {
        uint16_t bg_color = nes_palette[mask.emphasize][palette_table[0]];
        uint32_t color32 = ((uint32_t)bg_color << 16) | bg_color;
        uint32_t* buffer = (uint32_t*)scanline_buffer;
        for (int i = 0, size = (BUFFER_SIZE >> 1); i < size; i++) 
            buffer[i] = color32;

        memset(scanline_metadata, 0x80, BUFFER_SIZE);
        ptr_buffer = scanline_buffer + x;
        ptr_scanline_meta = scanline_metadata + x;
        return;
    }

    uint16_t bg_color = nes_palette[mask.emphasize][palette_table[0]];
    ptr_buffer = scanline_buffer;
    ptr_scanline_meta = scanline_metadata;
    x_tile = v.coarse_x;
    y_tile = v.coarse_y;
    offset = (control.background_table_addr ? 0x1000 : 0x0000) + v.fine_y;
    nametable_index = (v.reg >> 10) & 3;

    nametable_byte_base = v.reg & 0x03E0;
    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base + x_tile];

    attribute_byte_base = 0x03C0 + ((y_tile & 0x1C) << 1);
    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base + (x_tile >> 2)];
    attribute_byte = *ptr_attribute++;
    attribute_shift = ((y_tile & 2) << 1) + (x_tile & 2);
    attribute = ((attribute_byte >> attribute_shift) & 3) << 2;

    static constexpr DRAM_ATTR uint8_t pixel_shift[8] = { 14, 6, 12, 4, 10, 2, 8, 0 }; // Shifts to get the bits of a pixel
    static constexpr DRAM_ATTR uint8_t pixel_metadata[4] = { 0x80, 0x00, 0x00, 0x00 };
    for (int tile = 0; tile < 33; tile++)
    {
        tile_index = *ptr_tile++;
        ptr_pattern_tile = cart->ppuReadPtr(offset + (tile_index << 4)); 

        // draw to framebuffer
        uint16_t pattern = ((ptr_pattern_tile[8] & 0xAA) << 8) | ((ptr_pattern_tile[8] & 0x55) << 1)
                    | ((ptr_pattern_tile[0] & 0xAA) << 7) | (ptr_pattern_tile[0] & 0x55);
        uint16_t tile_palette[4];
        tile_palette[0] = bg_color;
        for (int t = 1; t < 4; t++) tile_palette[t] = nes_palette[mask.emphasize][READ_PALETTE(attribute + t)];
        for (int i = 0; i < 8; i++)
        {   
            uint8_t pixel = (pattern >> pixel_shift[i]) & 3;
            *ptr_buffer++ = tile_palette[pixel];
            *ptr_scanline_meta++ = pixel_metadata[pixel]; // Store if pixel is transparent for sprite rendering
        }

        x_tile++;
        if ((x_tile & 1) == 0)
        {
            if ((x_tile & 3) == 0)
            {
                if (x_tile == 32)
                {
                    // switch nametable
                    x_tile = 0;
                    nametable_index ^= 1;

                    // recalculate pointer to tile and attribute data
                    ptr_tile = &ptr_nametable[nametable_index][nametable_byte_base];
                    ptr_attribute = &ptr_nametable[nametable_index][attribute_byte_base];
                }

                attribute_byte = *ptr_attribute++;
            }

            attribute_shift ^= 2;
            attribute = ((attribute_byte >> attribute_shift) & 0x03) << 2;
        }
    }
    ptr_buffer = scanline_buffer + x;
}

inline void Ppu2C02::renderSprites(uint16_t scanline)
{
    if (!mask.render_sprite) 
    {
        return;
    }

    OAM* ptr_sprite_OAM;
    uint8_t sprite_size;
    uint8_t sprite_count = 0;

    uint16_t bg_color = nes_palette[mask.emphasize][palette_table[0]];
    uint16_t tile_palette[4];
    tile_palette[0] = bg_color;

    ptr_sprite_OAM = sprite;
    offset = (control.sprite_table_addr ? 0x1000 : 0);

    sprite_size = (control.sprite_size ? 16 : 8);

    uint16_t* buffer_offset = scanline_buffer + x;
    uint8_t* metadata_offset = scanline_metadata + x;
    for (int i = 0; i < 64; i++, ptr_sprite_OAM++)
    {
        uint8_t sprite_x, sprite_y;
        sprite_y = ptr_sprite_OAM->y + 1;
        // Check if sprite is in scanline
        if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_size)) 
            || (sprite_y == 0) || (sprite_y >= 240))
            continue;

        int16_t y_offset;
        uint16_t tile_addr;

        sprite_x = ptr_sprite_OAM->x;
        tile_index = ptr_sprite_OAM->index;
        attribute_byte = ptr_sprite_OAM->attribute;
        attribute = ((attribute_byte & 0x03) << 2);
    
        ptr_buffer = buffer_offset + sprite_x;
        ptr_scanline_meta = metadata_offset + sprite_x;

        // If 8x16 sprite mode
        tile_addr = (control.sprite_size) ? ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4) : offset + (tile_index << 4);
        ptr_tile = cart->ppuReadPtr(tile_addr);

        y_offset = scanline - sprite_y;
        if (y_offset > 7) y_offset += 8;
        if (attribute_byte & 0x80) // If flip sprite vertically
        {
            y_offset -= (control.sprite_size) ? 23 : 7;
            ptr_tile -= y_offset;
        }
        else ptr_tile += y_offset;

        // Draw to buffer
        uint16_t pattern = ((ptr_tile[8] & 0xAA) << 8) | ((ptr_tile[8] & 0x55) << 1)
                    | ((ptr_tile[0] & 0xAA) << 7) | (ptr_tile[0] & 0x55);
        if (pattern)
        {
            uint8_t pixel[8];
            uint8_t palette_offset = 16 + attribute;
            for (int t = 1; t < 4; t++) tile_palette[t] = nes_palette[mask.emphasize][READ_PALETTE(palette_offset + t)];

            if (attribute_byte & 0x40) // If flip sprite horizontally
            {
                pixel[7] = (pattern >> 14) & 3;
                pixel[6] = (pattern >> 6) & 3;
                pixel[5] = (pattern >> 12) & 3;
                pixel[4] = (pattern >> 4) & 3;
                pixel[3] = (pattern >> 10) & 3;
                pixel[2] = (pattern >> 2) & 3;
                pixel[1] = (pattern >> 8) & 3;
                pixel[0] = pattern & 3;
            }
            else
            {
                pixel[0] = (pattern >> 14) & 3;
                pixel[1] = (pattern >> 6) & 3;
                pixel[2] = (pattern >> 12) & 3;
                pixel[3] = (pattern >> 4) & 3;
                pixel[4] = (pattern >> 10) & 3;
                pixel[5] = (pattern >> 2) & 3;
                pixel[6] = (pattern >> 8) & 3;
                pixel[7] = pattern & 3;
            }

            // Check for sprite 0 hit
            if (i == 0 && status.sprite_zero_hit == 0)
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j] && ((ptr_scanline_meta[j] & 0x80) == 0))
                    {
                        status.sprite_zero_hit = true;
                        break;
                    }
                }
            }

            // Render sprite pixels on scanline buffer
            if (attribute_byte & 0x20) // Sprite Priorty : 1 - behind background | 0 - in front of background
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j])
                    {
                        if (ptr_scanline_meta[j] & 0x80) ptr_buffer[j] = tile_palette[pixel[j]];
                        ptr_scanline_meta[j] |= 0x40;
                    }
                }
            }
            else
            {
                for (int j = 0; j < 8; j++)
                {
                    if (pixel[j] && ((ptr_scanline_meta[j] & 0x40) == 0))
                    {
                        ptr_buffer[j] = tile_palette[pixel[j]];;
                        ptr_scanline_meta[j] |= 0x40;
                    }
                }
            }
        }

        // If sprite overflow, break
        if (++sprite_count == 8) { status.sprite_overflow = true; break; }
    }

    ptr_buffer = buffer_offset;
}

void Ppu2C02::fakeSpriteHit(uint16_t scanline)
{
    if (mask.render_background || mask.render_sprite) cart->ppuScanline();
    if (!mask.render_sprite || status.sprite_zero_hit) return;

    uint8_t sprite_size;
    offset = (control.sprite_table_addr ? 0x1000 : 0);
    sprite_size = (control.sprite_size ? 16 : 8);

    uint8_t sprite_x, sprite_y;
    sprite_y = sprite[0].y + 1;
    // Check if sprite is in scanline
    if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_size)) 
        || (sprite_y == 0) || (sprite_y >= 240))
        return;

    int16_t y_offset;
    uint16_t tile_addr;

    sprite_x = sprite[0].x;
    tile_index = sprite[0].index;
    attribute_byte = sprite[0].attribute;

    tile_addr = (control.sprite_size) ? ((tile_index & 0x01) << 12) | ((tile_index & 0xFE) << 4) : offset + (tile_index << 4);
    ptr_tile = cart->ppuReadPtr(tile_addr);

    y_offset = scanline - sprite_y;
    if (y_offset > 7) y_offset += 8;

    if (attribute_byte & 0x80) // If flip sprite vertically
    {
        y_offset -= (control.sprite_size) ? 23 : 7;
        ptr_tile -= y_offset;
    }
    else ptr_tile += y_offset;

    // Draw to buffer
    uint16_t pattern = ((ptr_tile[8] & 0xAA) << 8) | ((ptr_tile[8] & 0x55) << 1)
                    | ((ptr_tile[0] & 0xAA) << 7) | (ptr_tile[0] & 0x55);
    if (pattern)
    {
        status.sprite_zero_hit = true;
        return;
        // uint8_t pixel[8];
        // if (attribute_byte & 0x40) // If flip sprite horizontally
        // {
        //     pixel[7] = (pattern >> 14) & 3;
        //     pixel[6] = (pattern >> 6) & 3;
        //     pixel[5] = (pattern >> 12) & 3;
        //     pixel[4] = (pattern >> 4) & 3;
        //     pixel[3] = (pattern >> 10) & 3;
        //     pixel[2] = (pattern >> 2) & 3;
        //     pixel[1] = (pattern >> 8) & 3;
        //     pixel[0] = pattern & 3;
        // }
        // else
        // {
        //     pixel[0] = (pattern >> 14) & 3;
        //     pixel[1] = (pattern >> 6) & 3;
        //     pixel[2] = (pattern >> 12) & 3;
        //     pixel[3] = (pattern >> 4) & 3;
        //     pixel[4] = (pattern >> 10) & 3;
        //     pixel[5] = (pattern >> 2) & 3;
        //     pixel[6] = (pattern >> 8) & 3;
        //     pixel[7] = pattern & 3;
        // }

        // for (int i = 0; i < 8; i++)
        // {
        //     if (pixel[i]) 
        //     {
        //         status.sprite_zero_hit = true;
        //         return;
        //     }
        // }
    }
}

inline void Ppu2C02::finishScanline(uint16_t scanline)
{
    if (mask.render_background || mask.render_sprite) 
        cart->ppuScanline();
    uint32_t* display = (uint32_t*)(ptr_display + (scanline_counter * SCANLINE_SIZE));
    uint32_t* buffer = (uint32_t*)ptr_buffer;
    for (int i = 0, size = (SCANLINE_SIZE >> 1); i < size; i++) 
        display[i] = buffer[i];
    scanline_counter++;
    if (scanline_counter >= SCANLINES_PER_BUFFER) 
    { 
        bus->renderImage(scanline - (SCANLINES_PER_BUFFER - 1));
        scanline_counter = 0;
    }
}

void Ppu2C02::reset()
{
    status.reg = 0x00;
	mask.reg = 0x00;
	control.reg = 0x00;
	t.reg = 0x00;
	v.reg = 0x00;
	x = 0x00;
    w = 0x00;
    OAMADDR = 0x00;
    OAMDATA = 0x00;
	PPUDATA_buffer = 0x00;
    nametable_byte = 0x00;
    attribute_byte = 0x00;
}

void Ppu2C02::connectCartridge(Cartridge* cartridge)
{
    cart = cartridge;
    setMirror((Cartridge::MIRROR)cart->hardware_mirror);
}

void Ppu2C02::setMirror(Cartridge::MIRROR mirror)
{
    switch (mirror)
    {
        case Cartridge::MIRROR::VERTICAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0400];
            ptr_nametable[2] = &nametable[0x0000];
            ptr_nametable[3] = &nametable[0x0400];
            break;

        case Cartridge::MIRROR::HORIZONTAL:
            ptr_nametable[0] = &nametable[0x0000];
            ptr_nametable[1] = &nametable[0x0000];
            ptr_nametable[2] = &nametable[0x0400];
            ptr_nametable[3] = &nametable[0x0400];
            break;

        case Cartridge::MIRROR::ONESCREEN_LOW:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0000];
            break;

        case Cartridge::MIRROR::ONESCREEN_HIGH:
            ptr_nametable[0] = ptr_nametable[1] = ptr_nametable[2] = ptr_nametable[3] = &nametable[0x0400];
            break;
    }
}

Cartridge::MIRROR Ppu2C02::getMirror()
{
    if (ptr_nametable[0] == &nametable[0x0000] && 
        ptr_nametable[1] == &nametable[0x0400] &&
        ptr_nametable[2] == &nametable[0x0000] &&
        ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::VERTICAL;

    if (ptr_nametable[0] == &nametable[0x0000] && 
        ptr_nametable[1] == &nametable[0x0000] &&
        ptr_nametable[2] == &nametable[0x0400] &&
        ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::HORIZONTAL;

    if (ptr_nametable[0] == &nametable[0x0000] && 
        ptr_nametable[1] == &nametable[0x0000] &&
        ptr_nametable[2] == &nametable[0x0000] &&
        ptr_nametable[3] == &nametable[0x0000])
        return Cartridge::MIRROR::ONESCREEN_LOW;

    if (ptr_nametable[0] == &nametable[0x0400] && 
        ptr_nametable[1] == &nametable[0x0400] &&
        ptr_nametable[2] == &nametable[0x0400] &&
        ptr_nametable[3] == &nametable[0x0400])
        return Cartridge::MIRROR::ONESCREEN_HIGH;

    return Cartridge::MIRROR::HORIZONTAL;
}

void Ppu2C02::setPalette(uint8_t palette)
{
    switch (palette)
    {
    case NTSC565:
        nes_palette = palette_NTSC565;
        break;

    case PAL565:
        nes_palette = palette_PAL565;
        break;

    case NTSC222:
        nes_palette = palette_NTSC222;
        break;
    
    case PAL222:
        nes_palette = palette_PAL222;
        break;
    }
}

void Ppu2C02::dumpState(File& state)
{
    state.write((uint8_t*)scanline_buffer, sizeof(scanline_buffer));
    state.write((uint8_t*)scanline_metadata, sizeof(scanline_metadata));
    state.write(nametable, sizeof(nametable));
    for (int i = 0; i < 4; i++)
    {
        uint8_t map = 0;
        if (ptr_nametable[i] == &nametable[0x0400]) 
            map = 1;

        state.write((uint8_t*)&map, sizeof(map));
    }
    state.write(palette_table, sizeof(palette_table));
    state.write((uint8_t*)&scanline_counter, sizeof(scanline_counter));

    state.write((uint8_t*)&control.reg, sizeof(control.reg));
    state.write((uint8_t*)&mask.reg, sizeof(mask.reg));
    state.write((uint8_t*)&status.reg, sizeof(status.reg));
    state.write((uint8_t*)&OAMADDR, sizeof(OAMADDR));
    state.write((uint8_t*)&OAMDATA, sizeof(OAMDATA));

    state.write((uint8_t*)sprite, sizeof(sprite));
    state.write((uint8_t*)&v.reg, sizeof(v.reg));
    state.write((uint8_t*)&t.reg, sizeof(t.reg));
    state.write((uint8_t*)&x, sizeof(x));
    state.write((uint8_t*)&w, sizeof(w));
    state.write((uint8_t*)&PPUDATA_buffer, sizeof(PPUDATA_buffer));

    state.write((uint8_t*)&offset, sizeof(offset));
    state.write((uint8_t*)&nametable_index, sizeof(nametable_index));
    state.write((uint8_t*)&nametable_byte_base, sizeof(nametable_byte_base));
    state.write((uint8_t*)&attribute_byte_base, sizeof(attribute_byte_base));
    state.write((uint8_t*)&nametable_byte, sizeof(nametable_byte));
    state.write((uint8_t*)&attribute_byte, sizeof(attribute_byte));
    state.write((uint8_t*)&x_tile, sizeof(x_tile));
    state.write((uint8_t*)&y_tile, sizeof(y_tile));
    state.write((uint8_t*)&attribute_shift, sizeof(attribute_shift));
    state.write((uint8_t*)&attribute, sizeof(attribute));
    state.write((uint8_t*)&tile_index, sizeof(tile_index));
    state.write((uint8_t*)&sprite_count, sizeof(sprite_count));
}

void Ppu2C02::loadState(File& state)
{
    state.read((uint8_t*)scanline_buffer, sizeof(scanline_buffer));
    state.read((uint8_t*)scanline_metadata, sizeof(scanline_metadata));
    state.read(nametable, sizeof(nametable));
    for (int i = 0; i < 4; i++)
    {
        uint8_t map = 0;
        state.read(&map, sizeof(map));
        ptr_nametable[i] = &nametable[(map == 0) ? 0x0000 : 0x0400];
    }
    state.read(palette_table, sizeof(palette_table));
    state.read((uint8_t*)&scanline_counter, sizeof(scanline_counter));

    state.read((uint8_t*)&control.reg, sizeof(control.reg));
    state.read((uint8_t*)&mask.reg, sizeof(mask.reg));
    state.read((uint8_t*)&status.reg, sizeof(status.reg));
    state.read((uint8_t*)&OAMADDR, sizeof(OAMADDR));
    state.read((uint8_t*)&OAMDATA, sizeof(OAMDATA));

    state.read((uint8_t*)sprite, sizeof(sprite));
    state.read((uint8_t*)&v.reg, sizeof(v.reg));
    state.read((uint8_t*)&t.reg, sizeof(t.reg));
    state.read((uint8_t*)&x, sizeof(x));
    state.read((uint8_t*)&w, sizeof(w));
    state.read((uint8_t*)&PPUDATA_buffer, sizeof(PPUDATA_buffer));

    state.read((uint8_t*)&offset, sizeof(offset));
    state.read((uint8_t*)&nametable_index, sizeof(nametable_index));
    state.read((uint8_t*)&nametable_byte_base, sizeof(nametable_byte_base));
    state.read((uint8_t*)&attribute_byte_base, sizeof(attribute_byte_base));
    state.read((uint8_t*)&nametable_byte, sizeof(nametable_byte));
    state.read((uint8_t*)&attribute_byte, sizeof(attribute_byte));
    state.read((uint8_t*)&x_tile, sizeof(x_tile));
    state.read((uint8_t*)&y_tile, sizeof(y_tile));
    state.read((uint8_t*)&attribute_shift, sizeof(attribute_shift));
    state.read((uint8_t*)&attribute, sizeof(attribute));
    state.read((uint8_t*)&tile_index, sizeof(tile_index));
    state.read((uint8_t*)&sprite_count, sizeof(sprite_count));
}