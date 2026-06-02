// Credit to Peter Barrett for his research and work on composite video generation using the ESP32.
// This code is based on his work and has been adapted for use in this project.
// All credit for the composite video generation goes to him.
// https://github.com/rossumur/esp_8_bit

/* Copyright (c) 2020, Peter Barrett
**
** Permission to use, copy, modify, and/or distribute this software for
** any purpose with or without fee is hereby granted, provided that the
** above copyright notice and this permission notice appear in all copies.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#ifndef COMPOSITE_VIDEO_H
#define COMPOSITE_VIDEO_H

#include "../config.h"
#include "controller.h"
#include "core/bus.h"
#include "core/cartridge.h"
#include "core/rom_backends.h"
#include "driver/dac.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/periph_ctrl.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_intr_alloc.h"
#include "esp_types.h"
#include "rom/gpio.h"
#include "rom/lldesc.h"
#include "soc/efuse_periph.h"
#include "soc/efuse_reg.h"
#include "soc/gpio_reg.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/ledc_struct.h"
#include "soc/rtc.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc.h"
#include <stdint.h>

static const int screen_width = 256;
static const int screen_height = 240;
uint8_t cv_framebuffer[screen_width * screen_height];

// https://wiki.nesdev.com/w/index.php/NTSC_video
// // NES/SMS have pixel rates of 5.3693175, or 2/3 color clock
// // in 3 phase mode each pixel gets 2 DAC values written, 2 color clocks = 3 nes pixels
static const DRAM_ATTR uint32_t nes_3_phase[64] = {
    0x2C2C2C00, 0x241D2400, 0x221D2600, 0x1F1F2700, 0x1D222600, 0x1D242400, 0x1D262200, 0x1F271F00,
    0x22261D00, 0x24241D00, 0x26221D00, 0x271F1F00, 0x261D2200, 0x14141400, 0x14141400, 0x14141400,
    0x38383800, 0x2C252C00, 0x2A252E00, 0x27272F00, 0x252A2E00, 0x252C2C00, 0x252E2A00, 0x272F2700,
    0x2A2E2500, 0x2C2C2500, 0x2E2A2500, 0x2F272700, 0x2E252A00, 0x1F1F1F00, 0x15151500, 0x15151500,
    0x45454500, 0x3A323A00, 0x37333C00, 0x35353C00, 0x33373C00, 0x323A3A00, 0x333C3700, 0x353C3500,
    0x373C3300, 0x3A3A3200, 0x3C373300, 0x3C353500, 0x3C333700, 0x2B2B2B00, 0x16161600, 0x16161600,
    0x45454500, 0x423B4200, 0x403B4400, 0x3D3D4500, 0x3B404400, 0x3B424200, 0x3B444000, 0x3D453D00,
    0x40443B00, 0x42423B00, 0x44403B00, 0x453D3D00, 0x443B4000, 0x39393900, 0x17171700, 0x17171700,
};
static const DRAM_ATTR uint32_t nes_4_phase[64] = {
    0x2C2C2C2C, 0x241D1F26, 0x221D2227, 0x1F1D2426, 0x1D1F2624, 0x1D222722, 0x1D24261F, 0x1F26241D,
    0x2227221D, 0x24261F1D, 0x26241D1F, 0x27221D22, 0x261F1D24, 0x14141414, 0x14141414, 0x14141414,
    0x38383838, 0x2C25272E, 0x2A252A2F, 0x27252C2E, 0x25272E2C, 0x252A2F2A, 0x252C2E27, 0x272E2C25,
    0x2A2F2A25, 0x2C2E2725, 0x2E2C2527, 0x2F2A252A, 0x2E27252C, 0x1F1F1F1F, 0x15151515, 0x15151515,
    0x45454545, 0x3A33353C, 0x3732373C, 0x35333A3C, 0x33353C3A, 0x32373C37, 0x333A3C35, 0x353C3A33,
    0x373C3732, 0x3A3C3533, 0x3C3A3335, 0x3C373237, 0x3C35333A, 0x2B2B2B2B, 0x16161616, 0x16161616,
    0x45454545, 0x423B3D44, 0x403B4045, 0x3D3B4244, 0x3B3D4442, 0x3B404540, 0x3B42443D, 0x3D44423B,
    0x4045403B, 0x42443D3B, 0x44423B3D, 0x45403B40, 0x443D3B42, 0x39393939, 0x17171717, 0x17171717,
};
static const DRAM_ATTR uint32_t nes_yuv_4_phase_pal[] = {
    0x31313131, 0x2D21202B, 0x2720252D, 0x21212B2C, 0x1D23302A, 0x1B263127, 0x1C293023, 0x202B2D22,
    0x262B2722, 0x2C2B2122, 0x2F2B1E23, 0x31291F27, 0x30251F2A, 0x18181818, 0x19191919, 0x19191919,
    0x3D3D3D3D, 0x34292833, 0x2F282D34, 0x29283334, 0x252B3732, 0x232E392E, 0x2431382B, 0x28333429,
    0x2D342F28, 0x33342928, 0x3732252A, 0x392E232E, 0x382B2431, 0x24242424, 0x1A1A1A1A, 0x1A1A1A1A,
    0x49494949, 0x42373540, 0x3C373B40, 0x36374040, 0x3337433F, 0x3139433B, 0x323D4338, 0x35414237,
    0x3B423D35, 0x41413736, 0x453F3238, 0x473C313B, 0x4639323F, 0x2F2F2F2F, 0x1A1A1A1A, 0x1A1A1A1A,
    0x49494949, 0x48413D45, 0x42404345, 0x3D3F4644, 0x3B3D4543, 0x3B3E4542, 0x3B42453F, 0x3E47463E,
    0x434A453E, 0x46483E3D, 0x4843393E, 0x4A403842, 0x4B403944, 0x3E3E3E3E, 0x1B1B1B1B, 0x1B1B1B1B,
    0x31313131, 0x20212D2B, 0x2520272D, 0x2B21212C, 0x30231D2A, 0x31261B27, 0x30291C23, 0x2D2B2022,
    0x272B2622, 0x212B2C22, 0x1E2B2F23, 0x1F293127, 0x1F25302A, 0x18181818, 0x19191919, 0x19191919,
    0x3D3D3D3D, 0x28293433, 0x2D282F34, 0x33282934, 0x372B2532, 0x392E232E, 0x3831242B, 0x34332829,
    0x2F342D28, 0x29343328, 0x2532372A, 0x232E392E, 0x242B3831, 0x24242424, 0x1A1A1A1A, 0x1A1A1A1A,
    0x49494949, 0x35374240, 0x3B373C40, 0x40373640, 0x4337333F, 0x4339313B, 0x433D3238, 0x42413537,
    0x3D423B35, 0x37414136, 0x323F4538, 0x313C473B, 0x3239463F, 0x2F2F2F2F, 0x1A1A1A1A, 0x1A1A1A1A,
    0x49494949, 0x3D414845, 0x43404245, 0x463F3D44, 0x453D3B43, 0x453E3B42, 0x45423B3F, 0x46473E3E,
    0x454A433E, 0x3E48463D, 0x3943483E, 0x38404A42, 0x39404B44, 0x3E3E3E3E, 0x1B1B1B1B, 0x1B1B1B1B,
};

static int _pal_ = 0;

static lldesc_t _dma_desc[2] = { 0 };
static intr_handle_t _isr_handle;
void IRAM_ATTR video_isr(volatile void* buf);
void IRAM_ATTR i2s_intr_handler_video(void* arg)
{
    if (I2S0.int_st.out_eof)
        video_isr(
            (volatile void*)((lldesc_t*)I2S0.out_eof_des_addr)->buf); // get the next line of video
    I2S0.int_clr.val = I2S0.int_st.val;                               // reset the interrupt
}

static esp_err_t composite_video_start_dma(int line_width, int samples_per_cc, int ch = 1)
{
    periph_module_enable(PERIPH_I2S0_MODULE);

    // setup interrupt
    if (esp_intr_alloc(ETS_I2S0_INTR_SOURCE, ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
                       i2s_intr_handler_video, 0, &_isr_handle) != ESP_OK)
        return -1;

    // reset conf
    I2S0.conf.val = 1;
    I2S0.conf.val = 0;
    I2S0.conf.tx_right_first = 1;
    I2S0.conf.tx_mono = (ch == 2 ? 0 : 1);

    I2S0.conf2.lcd_en = 1;
    I2S0.fifo_conf.tx_fifo_mod_force_en = 1;
    I2S0.sample_rate_conf.tx_bits_mod = 16;
    I2S0.conf_chan.tx_chan_mod = (ch == 2) ? 0 : 1;

    // Create TX DMA buffers
    for (int i = 0; i < 2; i++)
    {
        int n = line_width * 2 * ch;
        if (n >= 4092)
        {
            printf("DMA chunk too big:%s\n", n);
            return -1;
        }
        _dma_desc[i].buf = (uint8_t*)heap_caps_calloc(1, n, MALLOC_CAP_DMA);
        if (!_dma_desc[i].buf) return -1;

        _dma_desc[i].owner = 1;
        _dma_desc[i].eof = 1;
        _dma_desc[i].length = n;
        _dma_desc[i].size = n;
        _dma_desc[i].empty = (uint32_t)(i == 1 ? _dma_desc : _dma_desc + 1);
    }
    I2S0.out_link.addr = (uint32_t)_dma_desc;

    //  Setup up the apll: See ref 3.2.7 Audio PLL
    // Formula:
    // vco = 40000000 * (4 + sdm2 + sdm1/256 + sdm0/65536);
    // apll_freq = f_out / ((o_div + 2) * 2);
    // dac_freq = apll_freq / 4; // <- This is the output frequency of GPIO25
    // operating range of the f_out is 250 MHz ~ 500 MHz
    // operating range of the apll_freq is 16 ~ 128 MHz.
    // select sdm0,sdm1,sdm2 to produce nice multiples of colorburst frequencies

    // Calculations for the closest possible frequency
    // These coefficients should supposedly be more accurate than the original coefficients used
    // === NTSC_3x (target = 10.7386363636 MHz) ===
    //   o_div = 2
    //   sdm2  = 4
    //   sdm1  = 151
    //   sdm0  = 70
    //   vco   = 343.6364746094 MHz  (valid: 250-500 MHz)
    //   apll  = 42.9545593262 MHz
    //   dac   = 10738639.8315429688 Hz
    //   error = +3.467943 Hz
    //   rtc_clk_apll_coeff_set(2, 70, 151, 4);

    // === NTSC_4x (target = 14.3181818182 MHz) ===
    //   o_div = 2
    //   sdm2  = 7
    //   sdm1  = 116
    //   sdm0  = 93
    //   vco   = 458.1817626953 MHz  (valid: 250-500 MHz)
    //   apll  = 57.2727203369 MHz
    //   dac   = 14318180.0842285156 Hz
    //   error = -1.733971 Hz
    //   rtc_clk_apll_coeff_set(2, 93, 116, 7);

    // === PAL_4x (target = 17.7344760000 MHz) ===
    //   o_div = 1
    //   sdm2  = 6
    //   sdm1  = 164
    //   sdm0  = 4
    //   vco   = 425.6274414062 MHz  (valid: 250-500 MHz)
    //   apll  = 70.9379069010 MHz
    //   dac   = 17734476.7252604179 Hz
    //   error = +0.725260 Hz
    //   rtc_clk_apll_coeff_set(1, 4, 164, 6);

    rtc_clk_apll_enable(1);
    if (!_pal_)
    {
        switch (samples_per_cc)
        {
        case 3:
            rtc_clk_apll_coeff_set(2, 0x46, 0x97, 0x4);
            break; // 10.7386363636 3x NTSC (10.7386398315mhz)
        case 4:
            // rtc_clk_apll_coeff_set(1, 0x46, 0x97, 0x4);
            rtc_clk_apll_coeff_set(2, 93, 116, 7); // Supposedly more accurate version
            break;                                 // 14.3181818182 4x NTSC (14.3181864421mhz)
        }
    }
    else
    {
        rtc_clk_apll_coeff_set(1, 0x04, 0xA4, 0x6); // 17.734476mhz ~4x PAL
    }

    I2S0.clkm_conf.clkm_div_num = 1; // I2S clock divider’s integral value.
    I2S0.clkm_conf.clkm_div_b = 0;   // Fractional clock divider’s numerator value.
    I2S0.clkm_conf.clkm_div_a = 1;   // Fractional clock divider’s denominator value
    I2S0.sample_rate_conf.tx_bck_div_num = 1;
    I2S0.clkm_conf.clka_en = 1;                     // Set this bit to enable clk_apll.
    I2S0.fifo_conf.tx_fifo_mod = (ch == 2) ? 0 : 1; // 32-bit dual or 16-bit single channel data

    dac_output_enable(DAC_CHANNEL_1);
    dac_i2s_enable();

    I2S0.conf.tx_start = 1;
    I2S0.int_clr.val = 0xFFFFFFFF;
    I2S0.int_ena.out_eof = 1;
    I2S0.out_link.start = 1;
    return esp_intr_enable(_isr_handle); // start interruprs!
}

void video_init_hw(int line_width, int samples_per_cc)
{
    // setup apll 4x NTSC or PAL colorburst rate
    composite_video_start_dma(line_width, samples_per_cc, 1);

    // Now ideally we would like to use the decoupled left DAC channel to produce audio
    // But when using the APLL there appears to be some clock domain conflict that causes
    // nasty digitial spikes and dropouts. You are also limited to a single audio channel.
    // So it is back to PWM/PDM and a 1 bit DAC for us. Good news is that we can do stereo
    // if we want to and have lots of different ways of doing nice noise shaping etc.

    // PWM audio out of pin 18 -> can be anything
    // lots of other ways, PDM by hand over I2S1, spi circular buffer etc
    // but if you would like stereo the led pwm seems like a fine choice
    // needs a simple rc filter (1k->1.2k resistor & 10nf->15nf cap work fine)

    // 18 ----/\/\/\/----|------- a out
    //          1k       |
    //                  ---
    //                  --- 10nf
    //                   |
    //                   v gnd

    // ledcAttach(AUDIO_PIN, 2000000, 7); // 625000 khz is as fast as we go w 7 bits
    ledcAttachChannel(AUDIO_PIN, 625000, 7, 0);
    ledcWrite(0, 0);
}

// send an audio sample every scanline (15720hz for ntsc, 15600hz for PAL)
inline void IRAM_ATTR audio_sample(uint8_t s)
{
    auto& reg = LEDC.channel_group[0].channel[0];
    reg.duty.duty = s << 4;   // 25 bit (21.4)
    reg.conf0.sig_out_en = 1; // This is the output enable control bit for channel
    reg.conf1.duty_start =
        1; // When duty_num duty_cycle and duty_scale has been configured. these register won't take
           // effect until set duty_start. this bit is automatically cleared by hardware
    reg.conf0.clk_en = 1;
}

//====================================================================================================
//====================================================================================================

uint32_t cpu_ticks()
{
    return xthal_get_ccount();
}

uint32_t us()
{
    return cpu_ticks() / 240;
}

// Color clock frequency is 315/88 (3.57954545455)
// DAC_MHZ is 315/11 or 8x color clock
// 455/2 color clocks per line, round up to maintain phase
// HSYNCH period is 44/315*455 or 63.55555..us
// Field period is 262*44/315*455 or 16651.5555us

#define IRE(_x)        ((uint32_t)(((_x) + 40) * 255 / 3.3 / 147.5) << 8) // 3.3V DAC
#define SYNC_LEVEL     IRE(-40)
#define BLANKING_LEVEL IRE(0)
#define BLACK_LEVEL    IRE(7.5)
#define GRAY_LEVEL     IRE(50)
#define WHITE_LEVEL    IRE(100)

#define P0             (color >> 16)
#define P1             (color >> 8)
#define P2             (color)
#define P3             (color << 8)

static uint8_t* _framebuffer;
volatile int _line_counter = 0;
volatile int _frame_counter = 0;

static int _active_lines;
static int _line_count;

static int _line_width;
static int _samples_per_cc;
static const uint32_t* _palette;

static float _sample_rate;

static int _hsync;
static int _hsync_long;
static int _hsync_short;
static int _burst_start;
static int _burst_width;
static int _active_start;

static int16_t* _burst0 = 0; // pal bursts
static int16_t* _burst1 = 0;

static int usec(float us)
{
    uint32_t r = (uint32_t)(us * _sample_rate);
    return ((r + _samples_per_cc) / (_samples_per_cc << 1)) *
           (_samples_per_cc << 1); // multiple of color clock, word align
}

#define NTSC_COLOR_CLOCKS_PER_SCANLINE                                                             \
    228 // really 227.5 for NTSC but want to avoid half phase fiddling for now
#define NTSC_FREQUENCY                (315000000.0 / 88)
#define NTSC_LINES                    262

#define PAL_COLOR_CLOCKS_PER_SCANLINE 284 // really 283.75 ?
#define PAL_FREQUENCY                 4433618.75
#define PAL_LINES                     312

void pal_init();

void video_init(int ntsc)
{
    _framebuffer = cv_framebuffer;
    _samples_per_cc = 4;

    if (ntsc)
    {
        _palette = nes_4_phase;
        _sample_rate = 315.0 / 88 * _samples_per_cc; // DAC rate
        _line_width = NTSC_COLOR_CLOCKS_PER_SCANLINE * _samples_per_cc;
        _line_count = NTSC_LINES;
        _hsync_long = usec(63.555 - 4.7);
        _active_start = usec(_samples_per_cc == 4 ? 10 : 10.5);
        _hsync = usec(4.7);
        _pal_ = 0;
    }
    else
    {
        pal_init();
        _pal_ = 1;
    }

    _active_lines = 240;
    video_init_hw(_line_width, _samples_per_cc); // init the hardware
}

//===================================================================================================
//===================================================================================================
// PAL

void pal_init()
{
    _palette = nes_yuv_4_phase_pal;
    int cc_width = 4;
    _sample_rate = PAL_FREQUENCY * cc_width / 1000000.0; // DAC rate in mhz
    _line_width = PAL_COLOR_CLOCKS_PER_SCANLINE * cc_width;
    _line_count = PAL_LINES;
    _hsync_short = usec(2);
    _hsync_long = usec(30);
    _hsync = usec(4.7);
    _burst_start = usec(5.6);
    _burst_width = (int)(10 * cc_width + 4) & 0xFFFE;
    _active_start = usec(10.4);

    // make colorburst tables for even and odd lines
    _burst0 = new int16_t[_burst_width];
    _burst1 = new int16_t[_burst_width];
    float phase = 2 * M_PI / 2;
    for (int i = 0; i < _burst_width; i++)
    {
        _burst0[i] = BLANKING_LEVEL + sin(phase + 3 * M_PI / 4) * BLANKING_LEVEL / 1.5;
        _burst1[i] = BLANKING_LEVEL + sin(phase - 3 * M_PI / 4) * BLANKING_LEVEL / 1.5;
        phase += 2 * M_PI / cc_width;
    }
}

void IRAM_ATTR blit_pal(uint8_t* src, uint16_t* dst)
{
    uint32_t c, color;
    bool even = _line_counter & 1;
    const uint32_t* p = even ? _palette : _palette + 256;
    int left = 0;
    int right = 256;
    uint8_t mask = 0xFF;
    uint8_t c0, c1, c2, c3, c4;
    uint8_t y1, y2, y3;

    // case EMU_NES:
    // 192 of 288 color clocks wide: roughly correct aspect ratio
    mask = 0x3F;
    if (!even) p = _palette + 64;
    dst += 88;

    // 4 pixels over 3 color clocks, 12 samples
    // do the blitting
    for (int i = left; i < right; i += 4)
    {
        c = *((uint32_t*)(src + i));
        color = p[c & mask];
        dst[0 ^ 1] = P0;
        dst[1 ^ 1] = P1;
        dst[2 ^ 1] = P2;
        color = p[(c >> 8) & mask];
        dst[3 ^ 1] = P3;
        dst[4 ^ 1] = P0;
        dst[5 ^ 1] = P1;
        color = p[(c >> 16) & mask];
        dst[6 ^ 1] = P2;
        dst[7 ^ 1] = P3;
        dst[8 ^ 1] = P0;
        color = p[(c >> 24) & mask];
        dst[9 ^ 1] = P1;
        dst[10 ^ 1] = P2;
        dst[11 ^ 1] = P3;
        dst += 12;
    }
}

void IRAM_ATTR burst_pal(uint16_t* line)
{
    line += _burst_start;
    int16_t* b = (_line_counter & 1) ? _burst0 : _burst1;
    for (int i = 0; i < _burst_width; i += 2)
    {
        line[i ^ 1] = b[i];
        line[(i + 1) ^ 1] = b[i + 1];
    }
}

//===================================================================================================
//===================================================================================================
// ntsc tables
// AA AA                // 2 pixels, 1 color clock - atari
// AA AB BB             // 3 pixels, 2 color clocks - nes
// AAA ABB BBC CCC      // 4 pixels, 3 color clocks - sms

// cc == 3 gives 684 samples per line, 3 samples per cc, 3 pixels for 2 cc
// cc == 4 gives 912 samples per line, 4 samples per cc, 2 pixels per cc

// draw a line of game in NTSC
void IRAM_ATTR blit(uint8_t* src, uint16_t* dst)
{
    uint32_t* d = (uint32_t*)dst;
    const uint32_t* p = _palette;
    uint32_t color, c;
    uint32_t mask = 0xFF;
    int i;

    if (_pal_)
    {
        blit_pal(src, dst);
        return;
    }

    // case EMU_NES:
    mask = 0x3F;
    // AAA ABB BBC CCC
    // 4 pixels, 3 color clocks, 4 samples per cc
    // each pixel gets 3 samples, 192 color clocks wide
    for (i = 0; i < 256; i += 4)
    {
        c = *((uint32_t*)(src + i));
        color = p[c & mask];
        dst[0 ^ 1] = P0;
        dst[1 ^ 1] = P1;
        dst[2 ^ 1] = P2;
        color = p[(c >> 8) & mask];
        dst[3 ^ 1] = P3;
        dst[4 ^ 1] = P0;
        dst[5 ^ 1] = P1;
        color = p[(c >> 16) & mask];
        dst[6 ^ 1] = P2;
        dst[7 ^ 1] = P3;
        dst[8 ^ 1] = P0;
        color = p[(c >> 24) & mask];
        dst[9 ^ 1] = P1;
        dst[10 ^ 1] = P2;
        dst[11 ^ 1] = P3;
        dst += 12;
    }
}

void IRAM_ATTR burst(uint16_t* line)
{
    if (_pal_)
    {
        burst_pal(line);
        return;
    }

    int i, phase;
    switch (_samples_per_cc)
    {
    case 4:
        // 4 samples per color clock
        for (i = _hsync; i < _hsync + (4 * 10); i += 4)
        {
            line[i + 1] = BLANKING_LEVEL;
            line[i + 0] = BLANKING_LEVEL + BLANKING_LEVEL / 2;
            line[i + 3] = BLANKING_LEVEL;
            line[i + 2] = BLANKING_LEVEL - BLANKING_LEVEL / 2;
        }
        break;
    case 3:
        // 3 samples per color clock
        phase = 0.866025 * BLANKING_LEVEL / 2;
        for (i = _hsync; i < _hsync + (3 * 10); i += 6)
        {
            line[i + 1] = BLANKING_LEVEL;
            line[i + 0] = BLANKING_LEVEL + phase;
            line[i + 3] = BLANKING_LEVEL - phase;
            line[i + 2] = BLANKING_LEVEL;
            line[i + 5] = BLANKING_LEVEL + phase;
            line[i + 4] = BLANKING_LEVEL - phase;
        }
        break;
    }
}

void IRAM_ATTR sync(uint16_t* line, int syncwidth)
{
    for (int i = 0; i < syncwidth; i++) line[i] = SYNC_LEVEL;
}

void IRAM_ATTR blanking(uint16_t* line, bool vbl)
{
    int syncwidth = vbl ? _hsync_long : _hsync;
    sync(line, syncwidth);
    for (int i = syncwidth; i < _line_width; i++) line[i] = BLANKING_LEVEL;
    if (!vbl) burst(line); // no burst during vbl
}

// Fancy pal non-interlace
// http://martin.hinner.info/vga/pal.html
void IRAM_ATTR pal_sync2(uint16_t* line, int width, int swidth)
{
    swidth = swidth ? _hsync_long : _hsync_short;
    int i;
    for (i = 0; i < swidth; i++) line[i] = SYNC_LEVEL;
    for (; i < width; i++) line[i] = BLANKING_LEVEL;
}

uint8_t DRAM_ATTR _sync_type[8] = { 0, 0, 0, 3, 3, 2, 0, 0 };
void IRAM_ATTR pal_sync(uint16_t* line, int i)
{
    uint8_t t = _sync_type[i - 304];
    pal_sync2(line, _line_width / 2, t & 2);
    pal_sync2(line + _line_width / 2, _line_width / 2, t & 1);
}

// audio is buffered as 6 bit unsigned samples
uint8_t _audio_buffer[1024];
uint32_t _audio_r = 0;
uint32_t _audio_w = 0;
void cv_audio_write_16(const uint16_t* s, int len, int channels)
{
    int b;
    while (len--)
    {
        if (_audio_w == (_audio_r + sizeof(_audio_buffer))) break;
        if (channels == 2)
        {
            b = (s[0] + s[1]) >> 9;
            s += 2;
        }
        else b = *s++ >> 8;
        b >>= 1; // scale [0, 255] down to [0, 127]
        if (b > 127) b = 127;
        _audio_buffer[_audio_w++ & (sizeof(_audio_buffer) - 1)] = b;
    }
}

bool cv_audio_buffer_full(int buffer_size)
{
    return (_audio_w - _audio_r) + buffer_size > sizeof(_audio_buffer);
}

// Wait for blanking before starting drawing
// avoids tearing in our unsynchonized world
void video_sync()
{
    int n = 0;
    if (_pal_)
    {
        if (_line_counter < _active_lines) n = (_active_lines - _line_counter) * 1000 / 15600;
    }
    else
    {
        if (_line_counter < _active_lines) n = (_active_lines - _line_counter) * 1000 / 15720;
    }
    vTaskDelay(n + 1);
}

void IRAM_ATTR video_isr(volatile void* vbuf)
{
    uint8_t s =
        _audio_r < _audio_w ? _audio_buffer[_audio_r++ & (sizeof(_audio_buffer) - 1)] : 0x40;
    audio_sample(s);
    // audio_sample(_sin64[_x++ & 0x3F]);

    int i = _line_counter++;
    uint16_t* buf = (uint16_t*)vbuf;
    if (_pal_)
    {
        // pal
        if (i < 32)
        {
            blanking(buf, false); // pre render/black 0-32
        }
        else if (i < _active_lines + 32)
        { // active video 32-272
            sync(buf, _hsync);
            burst(buf);
            blit(_framebuffer + ((i - 32) * 256), buf + _active_start);
        }
        else if (i < 304)
        {                // post render/black 272-304
            if (i < 274) // slight optimization here, once you have 2 blanking buffers
                blanking(buf, false);
        }
        else
        {
            pal_sync(buf, i); // 8 lines of sync 304-312
        }
    }
    else
    {
        // ntsc
        if (i < _active_lines)
        { // active video
            sync(buf, _hsync);
            burst(buf);
            blit(_framebuffer + (i * 256), buf + _active_start);
        }
        else if (i < (_active_lines + 5))
        { // post render/black
            blanking(buf, false);
        }
        else if (i < (_active_lines + 8))
        { // vsync
            blanking(buf, true);
        }
        else
        { // pre render/black
            blanking(buf, false);
        }
    }

    if (_line_counter == _line_count)
    {
        _line_counter = 0; // frame is done
        _frame_counter++;
    }
}

// Composite UI
// Constant: font8x8_basic
// Contains an 8x8 font map for unicode points U+0000 - U+007F (basic latin)
static constexpr char font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00 }, // U+0021 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0022 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00 }, // U+0023 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00 }, // U+0024 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00 }, // U+0025 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00 }, // U+0026 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0027 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00 }, // U+0028 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00 }, // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00 }, // U+002A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00 }, // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // U+002C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00 }, // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // U+002E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00 }, // U+002F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00 }, // U+0030 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00 }, // U+0031 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00 }, // U+0032 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00 }, // U+0033 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00 }, // U+0034 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00 }, // U+0035 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00 }, // U+0036 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00 }, // U+0037 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00 }, // U+0038 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00 }, // U+0039 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00 }, // U+003A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06 }, // U+003B (;)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00 }, // U+003C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00 }, // U+003D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00 }, // U+003E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00 }, // U+003F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00 }, // U+0040 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00 }, // U+0041 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00 }, // U+0042 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00 }, // U+0043 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00 }, // U+0044 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00 }, // U+0045 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00 }, // U+0046 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00 }, // U+0047 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00 }, // U+0048 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0049 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00 }, // U+004A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00 }, // U+004B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 }, // U+004C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00 }, // U+004D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00 }, // U+004E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00 }, // U+004F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00 }, // U+0050 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00 }, // U+0051 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00 }, // U+0052 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00 }, // U+0053 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0054 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00 }, // U+0055 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // U+0056 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00 }, // U+0057 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00 }, // U+0058 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0059 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00 }, // U+005A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00 }, // U+005B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00 }, // U+005C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00 }, // U+005D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00 }, // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF }, // U+005F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+0060 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00 }, // U+0061 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00 }, // U+0062 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00 }, // U+0063 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00 }, // U+0064 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00 }, // U+0065 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00 }, // U+0066 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // U+0067 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00 }, // U+0068 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+0069 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E }, // U+006A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00 }, // U+006B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00 }, // U+006C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00 }, // U+006D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00 }, // U+006E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00 }, // U+006F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F }, // U+0070 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78 }, // U+0071 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00 }, // U+0072 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00 }, // U+0073 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00 }, // U+0074 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00 }, // U+0075 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00 }, // U+0076 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00 }, // U+0077 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00 }, // U+0078 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F }, // U+0079 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00 }, // U+007A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00 }, // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00 }, // U+007C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00 }, // U+007D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // U+007F
};
// NES palette indices for UI colors
#define CV_BLACK     0x0F
#define CV_WHITE     0x30
#define CV_GRAY      0x00
#define CV_RED       0x16
#define CV_GREEN     0x1A
#define CV_DARK_BLUE 0x12
#define CV_DARK_GRAY 0x31

inline void cv_fill(uint8_t color)
{
    if (color >= 64) return;
    memset(_framebuffer, color, 240 * 256);
}

inline void cv_draw_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= 256 || y < 0 || y >= 240) return;
    if (color >= 64) return;
    _framebuffer[y * 256 + x] = color;
}

inline void cv_draw_rect(int x, int y, int w, int h, uint8_t color)
{
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++) cv_draw_pixel(col, row, color);
}

inline void cv_draw_char(int x, int y, char c, uint8_t color)
{
    if (c < 0 || c > 127) return;
    const uint8_t* glyph = (const uint8_t*)font8x8_basic[(uint8_t)c];
    for (int row = 0; row < 8; row++)
        for (int col = 0; col < 8; col++)
            if (glyph[row] & (1 << col)) cv_draw_pixel(x + col, y + row, color);
}

inline void cv_draw_string(const char* str, int x, int y, uint8_t color)
{
    while (*str)
    {
        cv_draw_char(x, y, *str++, color);
        x += 8;
    }
}

inline constexpr int cv_text_width(const char* str)
{
    if (!str) return 0;

    int width = 0;

    while (*str++) width += 8; // 8 pixels glyph

    // Remove trailing spacing after last character
    if (width > 0) width -= 1;

    return width;
}

inline void cv_getNesFiles(std::vector<std::string>& files)
{
    File root = SD.open("/");
    while (true)
    {
        File file = root.openNextFile();
        if (!file) break;
        if (!file.isDirectory())
        {
            std::string filename = file.name();
            if (filename.rfind(".nes") == filename.size() - 4) files.push_back(filename);
        }

        file.close();
    }
    root.close();
}

inline void cv_drawFileList(int size, int max_items, int selected, int scroll_offset,
                            std::vector<std::string>& files)
{
    for (int i = 0; i < max_items; i++)
    {
        int item = i + scroll_offset;
        if (item >= size) break;

        std::string file = files[item];
        int maxWidth = screen_width - 28;
        while (cv_text_width(file.c_str()) > maxWidth) { file.pop_back(); }
        if (file.size() < files[item].size()) { file.replace(file.size() - 3, 3, "..."); }

        const char* filename = file.c_str();
        int y = i * 10 + 32;
        if (item == selected) cv_draw_string(filename, 14, y, CV_GREEN);
        else cv_draw_string(filename, 14, y, CV_WHITE);
    }
}

Cartridge* cv_selectGame()
{
    cv_fill(CV_BLACK);

    int selected = 0;
    int scroll_offset = 0;
    std::vector<std::string> files;
    static constexpr int max_items = (screen_height - 30) / 10;

    cv_getNesFiles(files);

    const int size = files.size();
    cv_drawFileList(size, max_items, selected, scroll_offset, files);

    unsigned int last_input_time = 0;
    while (true)
    {
        static constexpr unsigned int delay = 250;
        unsigned int now = millis();

        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up))
            {
                selected--;
                if (selected < 0)
                {
                    selected = (size - 1);
                    scroll_offset = selected - max_items + 1;
                }
                else if (selected < scroll_offset) scroll_offset = selected;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                cv_draw_rect(10, 32, screen_width - 20, screen_height - 16, CV_BLACK);
                cv_drawFileList(size, max_items, selected, scroll_offset, files);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down))
            {
                selected++;
                if (selected > (size - 1))
                {
                    selected = 0;
                    scroll_offset = selected;
                }
                else if (selected >= scroll_offset + max_items)
                    scroll_offset = selected - max_items + 1;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                cv_draw_rect(10, 32, screen_width - 20, screen_height - 16, CV_BLACK);
                cv_drawFileList(size, max_items, selected, scroll_offset, files);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Left))
            {
                int screen_pos = selected - scroll_offset;
                selected -= max_items;
                if (selected < 0) selected = 0;
                scroll_offset = selected - screen_pos;
                if (scroll_offset < 0) scroll_offset = 0;
                cv_draw_rect(10, 32, screen_width - 20, screen_height - 16, CV_BLACK);
                cv_drawFileList(size, max_items, selected, scroll_offset, files);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Right))
            {
                int screen_pos = selected - scroll_offset;
                selected += max_items;
                if (selected > size - 1) selected = size - 1;
                scroll_offset = selected - screen_pos;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > size - 1) scroll_offset = size - 1;
                cv_draw_rect(10, 32, screen_width - 20, screen_height - 16, CV_BLACK);
                cv_drawFileList(size, max_items, selected, scroll_offset, files);
                last_input_time = now;
            }
        }

        if (isDownPressed(CONTROLLER::A) && (selected >= 0 && selected < size))
        {
            std::string game = "/" + files[selected];
            std::vector<std::string>().swap(files);
            return new Cartridge(game.c_str(), ROMBackend::FLASH);
        }
    }
}

bool cv_paused = false;
void cv_pauseMenu(Bus* nes)
{
    int prev_select = 0;
    int select = 0;
    static constexpr int window_w = 160;
    static constexpr int window_h = 96;
    static constexpr int window_x = (screen_width - window_w) / 2;
    static constexpr int window_y = (screen_height - window_h) / 2;
    static constexpr char* title = "PAUSE MENU";
    static constexpr int title_x = window_x + ((window_w - cv_text_width(title)) / 2);
    static constexpr int title_y = window_y + 8;

    enum ItemSelect : uint8_t
    {
        Resume,
        Reset,
        QuickSaveState,
        QuickLoadState,
        SaveAndQuit
    };
    static constexpr const char* items[] = { "Resume", "Reset", "Quick Save State",
                                             "Quick Load State", "Save and Quit" };
    static constexpr int num_items = sizeof(items) / sizeof(items[0]);
    static constexpr int item_height = 12;
    static constexpr int text_height = 8;
    static constexpr int item_spacing = 2;
    static constexpr int items_start_y = window_y + 24;
    static constexpr int items_y[] = {
        items_start_y + (item_height + item_spacing) * 0,
        items_start_y + (item_height + item_spacing) * 1,
        items_start_y + (item_height + item_spacing) * 2,
        items_start_y + (item_height + item_spacing) * 3,
        items_start_y + (item_height + item_spacing) * 4,
    };

    // Draw pause window
    cv_draw_rect(window_x - 2, window_y - 2, window_w + 4, window_h + 4, CV_GRAY);
    cv_draw_rect(window_x, window_y, window_w, window_h, CV_DARK_BLUE);
    cv_draw_string(title, title_x, title_y, CV_WHITE);

    // Draw items
    for (int i = 0; i < num_items; i++)
    {
        int y = items_y[i] + item_spacing;
        cv_draw_string(items[i], window_x + 12, y, CV_WHITE);
    }
    cv_draw_rect(window_x + 10, items_y[select], window_w - 20, item_height, CV_WHITE);
    cv_draw_string(items[select], window_x + 12, items_y[select] + item_spacing, CV_DARK_BLUE);

    static constexpr int initial_delay = 500;
    int last_input_time = millis() + initial_delay;
    while (true)
    {
        static constexpr int delay = 250;
        int now = millis();
        if (now - last_input_time > delay)
        {
            if (isDownPressed(CONTROLLER::Up))
            {
                select--;
                if (select < 0) select = (num_items - 1);
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::Down))
            {
                select++;
                if (select > (num_items - 1)) select = 0;
                last_input_time = now;
            }

            if (isDownPressed(CONTROLLER::A))
            {
                switch (select)
                {
                case Resume: cv_paused = false; return;

                case Reset:
                    nes->reset();
                    cv_paused = false;
                    return;

                case QuickSaveState:
                    nes->saveState();
                    cv_paused = false;
                    return;

                case QuickLoadState:
                    nes->loadState();
                    cv_paused = false;
                    return;

                case SaveAndQuit: ESP.restart(); return;
                default: break;
                }
            }
        }

        // Update Selection
        if (prev_select != select)
        {
            int y;
            // Redraw old selection
            cv_draw_rect(window_x + 10, items_y[prev_select], window_w - 19, item_height,
                         CV_DARK_BLUE);
            y = items_y[prev_select] + item_spacing;
            cv_draw_string(items[prev_select], window_x + 12, y, CV_WHITE);

            // Draw new selection
            cv_draw_rect(window_x + 10, items_y[select], window_w - 19, item_height, CV_WHITE);
            y = items_y[select] + item_spacing;
            cv_draw_string(items[select], window_x + 12, y, CV_DARK_BLUE);
        }

        prev_select = select;
    }
}

#endif
