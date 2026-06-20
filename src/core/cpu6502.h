#ifndef CPU6502_H
#define CPU6502_H

#include <Arduino.h>
#include <SD.h>
#include <stdint.h>

#include "apu2A03.h"
#include "cartridge.h"

#define GET_FLAG(f)    ((status & (f)) != 0)
#define SET_FLAG(f, v) (status = (v) ? (status | (f)) : (status & ~(f)))
#define SET_ZN(v)      (status = ((status & ~(Z | N)) | zn_table[(v)]))

class Bus;
class Cpu6502
{
public:
    Cpu6502();
    ~Cpu6502();

public:
    Apu2A03 apu;

    // Status Register Flags
    enum FLAGS : uint8_t
    {
        C = (1 << 0), // Carry Bit
        Z = (1 << 1), // Zero Bit
        I = (1 << 2), // Interrupt Bit
        D = (1 << 3), // Decimal Bit
        B = (1 << 4), // Break Bit
        U = (1 << 5), // Unused Bit
        V = (1 << 6), // Overflow Bit
        N = (1 << 7)  // Negative Bit
    };

    void apuWrite(uint16_t addr, uint8_t data);
    uint8_t apuRead(uint16_t addr);
    void clock(int i);
    void OAM_DMA(uint8_t page);
    void reset();

    void IRQ();
    void NMI();

    void dumpState(File& state);
    void loadState(File& state);

    void connectBus(Bus* n)
    {
        bus = n;
    }
    void connectCartridge(Cartridge* cartridge)
    {
        cart = cartridge;
    }

    // Registers
    uint8_t A = 0x00;      // Accumulator
    uint8_t X = 0x00;      // X Index
    uint8_t Y = 0x00;      // Y Index
    uint16_t PC = 0x0000;  // Program Counter
    uint8_t SP = 0x00;     // Stack Pointer
    uint8_t status = 0x00; // Status register

    uint8_t fetched = 0x00;
    uint16_t addr_abs = 0x0000;
    uint16_t addr_rel = 0x0000;
    int cycles = 0;

private:
    Cartridge* __restrict cart = nullptr;
    Bus* __restrict bus = nullptr;
    bool addrmode_implied = false;
    uint8_t fetch();
    uint8_t fast_fetch();
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    void OAM_Write(uint8_t addr, uint8_t data);

    // clang-format off
	// Addressing Modes
	uint8_t ABS();	uint8_t IDX();
	uint8_t ABX();	uint8_t IDY();
	uint8_t ABY();	uint8_t REL();
	uint8_t IMM();	uint8_t ZPG();
	uint8_t IMP();	uint8_t ZPX();
	uint8_t IND();	uint8_t ZPY();

	// Instructions
	uint8_t Instr_ADC(); uint8_t Instr_CLI(); uint8_t Instr_LDX(); uint8_t Instr_SED();
	uint8_t Instr_AND(); uint8_t Instr_CLV(); uint8_t Instr_LDY(); uint8_t Instr_SEI();
	uint8_t Instr_ASL(); uint8_t Instr_CMP(); uint8_t Instr_LSR(); uint8_t Instr_STA();
	uint8_t Instr_BCC(); uint8_t Instr_CPX(); uint8_t Instr_NOP(); uint8_t Instr_STX();
	uint8_t Instr_BCS(); uint8_t Instr_CPY(); uint8_t Instr_ORA(); uint8_t Instr_STY();
	uint8_t Instr_BEQ(); uint8_t Instr_DEC(); uint8_t Instr_PHA(); uint8_t Instr_TAX();
	uint8_t Instr_BIT(); uint8_t Instr_DEX(); uint8_t Instr_PHP(); uint8_t Instr_TAY();
	uint8_t Instr_BMI(); uint8_t Instr_DEY(); uint8_t Instr_PLA(); uint8_t Instr_TSX();
	uint8_t Instr_BNE(); uint8_t Instr_EOR(); uint8_t Instr_PLP(); uint8_t Instr_TXA();
	uint8_t Instr_BPL(); uint8_t Instr_INC(); uint8_t Instr_ROL(); uint8_t Instr_TXS();
	uint8_t Instr_BRK(); uint8_t Instr_INX(); uint8_t Instr_ROR(); uint8_t Instr_TYA();
	uint8_t Instr_BVC(); uint8_t Instr_INY(); uint8_t Instr_RTI(); uint8_t Instr_CLD();
	uint8_t Instr_BVS(); uint8_t Instr_JMP(); uint8_t Instr_RTS(); uint8_t Instr_LDA();
	uint8_t Instr_CLC(); uint8_t Instr_JSR(); uint8_t Instr_SBC(); uint8_t Instr_SEC();
	uint8_t Instr_XXX();
    // clang-format on

    // Instruction cycle count
    static constexpr uint8_t instr_cycles[256] = {
        7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4,
        4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4,
        2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6,
        6, 2, 4, 2, 7, 4, 4, 7, 7, 6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, 2, 5, 2, 8,
        4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, 2,
        6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, 2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4,
        4, 4, 2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, 2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2,
        2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, 2, 6, 2, 8, 3, 3, 5, 5,
        2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
    };
    static constexpr uint8_t zn_table[256] = {
#define ENTRY(v) (((v) == 0 ? Cpu6502::Z : 0) | ((v) & Cpu6502::N))
        ENTRY(0x00), ENTRY(0x01), ENTRY(0x02), ENTRY(0x03), ENTRY(0x04), ENTRY(0x05), ENTRY(0x06),
        ENTRY(0x07), ENTRY(0x08), ENTRY(0x09), ENTRY(0x0A), ENTRY(0x0B), ENTRY(0x0C), ENTRY(0x0D),
        ENTRY(0x0E), ENTRY(0x0F), ENTRY(0x10), ENTRY(0x11), ENTRY(0x12), ENTRY(0x13), ENTRY(0x14),
        ENTRY(0x15), ENTRY(0x16), ENTRY(0x17), ENTRY(0x18), ENTRY(0x19), ENTRY(0x1A), ENTRY(0x1B),
        ENTRY(0x1C), ENTRY(0x1D), ENTRY(0x1E), ENTRY(0x1F), ENTRY(0x20), ENTRY(0x21), ENTRY(0x22),
        ENTRY(0x23), ENTRY(0x24), ENTRY(0x25), ENTRY(0x26), ENTRY(0x27), ENTRY(0x28), ENTRY(0x29),
        ENTRY(0x2A), ENTRY(0x2B), ENTRY(0x2C), ENTRY(0x2D), ENTRY(0x2E), ENTRY(0x2F), ENTRY(0x30),
        ENTRY(0x31), ENTRY(0x32), ENTRY(0x33), ENTRY(0x34), ENTRY(0x35), ENTRY(0x36), ENTRY(0x37),
        ENTRY(0x38), ENTRY(0x39), ENTRY(0x3A), ENTRY(0x3B), ENTRY(0x3C), ENTRY(0x3D), ENTRY(0x3E),
        ENTRY(0x3F), ENTRY(0x40), ENTRY(0x41), ENTRY(0x42), ENTRY(0x43), ENTRY(0x44), ENTRY(0x45),
        ENTRY(0x46), ENTRY(0x47), ENTRY(0x48), ENTRY(0x49), ENTRY(0x4A), ENTRY(0x4B), ENTRY(0x4C),
        ENTRY(0x4D), ENTRY(0x4E), ENTRY(0x4F), ENTRY(0x50), ENTRY(0x51), ENTRY(0x52), ENTRY(0x53),
        ENTRY(0x54), ENTRY(0x55), ENTRY(0x56), ENTRY(0x57), ENTRY(0x58), ENTRY(0x59), ENTRY(0x5A),
        ENTRY(0x5B), ENTRY(0x5C), ENTRY(0x5D), ENTRY(0x5E), ENTRY(0x5F), ENTRY(0x60), ENTRY(0x61),
        ENTRY(0x62), ENTRY(0x63), ENTRY(0x64), ENTRY(0x65), ENTRY(0x66), ENTRY(0x67), ENTRY(0x68),
        ENTRY(0x69), ENTRY(0x6A), ENTRY(0x6B), ENTRY(0x6C), ENTRY(0x6D), ENTRY(0x6E), ENTRY(0x6F),
        ENTRY(0x70), ENTRY(0x71), ENTRY(0x72), ENTRY(0x73), ENTRY(0x74), ENTRY(0x75), ENTRY(0x76),
        ENTRY(0x77), ENTRY(0x78), ENTRY(0x79), ENTRY(0x7A), ENTRY(0x7B), ENTRY(0x7C), ENTRY(0x7D),
        ENTRY(0x7E), ENTRY(0x7F), ENTRY(0x80), ENTRY(0x81), ENTRY(0x82), ENTRY(0x83), ENTRY(0x84),
        ENTRY(0x85), ENTRY(0x86), ENTRY(0x87), ENTRY(0x88), ENTRY(0x89), ENTRY(0x8A), ENTRY(0x8B),
        ENTRY(0x8C), ENTRY(0x8D), ENTRY(0x8E), ENTRY(0x8F), ENTRY(0x90), ENTRY(0x91), ENTRY(0x92),
        ENTRY(0x93), ENTRY(0x94), ENTRY(0x95), ENTRY(0x96), ENTRY(0x97), ENTRY(0x98), ENTRY(0x99),
        ENTRY(0x9A), ENTRY(0x9B), ENTRY(0x9C), ENTRY(0x9D), ENTRY(0x9E), ENTRY(0x9F), ENTRY(0xA0),
        ENTRY(0xA1), ENTRY(0xA2), ENTRY(0xA3), ENTRY(0xA4), ENTRY(0xA5), ENTRY(0xA6), ENTRY(0xA7),
        ENTRY(0xA8), ENTRY(0xA9), ENTRY(0xAA), ENTRY(0xAB), ENTRY(0xAC), ENTRY(0xAD), ENTRY(0xAE),
        ENTRY(0xAF), ENTRY(0xB0), ENTRY(0xB1), ENTRY(0xB2), ENTRY(0xB3), ENTRY(0xB4), ENTRY(0xB5),
        ENTRY(0xB6), ENTRY(0xB7), ENTRY(0xB8), ENTRY(0xB9), ENTRY(0xBA), ENTRY(0xBB), ENTRY(0xBC),
        ENTRY(0xBD), ENTRY(0xBE), ENTRY(0xBF), ENTRY(0xC0), ENTRY(0xC1), ENTRY(0xC2), ENTRY(0xC3),
        ENTRY(0xC4), ENTRY(0xC5), ENTRY(0xC6), ENTRY(0xC7), ENTRY(0xC8), ENTRY(0xC9), ENTRY(0xCA),
        ENTRY(0xCB), ENTRY(0xCC), ENTRY(0xCD), ENTRY(0xCE), ENTRY(0xCF), ENTRY(0xD0), ENTRY(0xD1),
        ENTRY(0xD2), ENTRY(0xD3), ENTRY(0xD4), ENTRY(0xD5), ENTRY(0xD6), ENTRY(0xD7), ENTRY(0xD8),
        ENTRY(0xD9), ENTRY(0xDA), ENTRY(0xDB), ENTRY(0xDC), ENTRY(0xDD), ENTRY(0xDE), ENTRY(0xDF),
        ENTRY(0xE0), ENTRY(0xE1), ENTRY(0xE2), ENTRY(0xE3), ENTRY(0xE4), ENTRY(0xE5), ENTRY(0xE6),
        ENTRY(0xE7), ENTRY(0xE8), ENTRY(0xE9), ENTRY(0xEA), ENTRY(0xEB), ENTRY(0xEC), ENTRY(0xED),
        ENTRY(0xEE), ENTRY(0xEF), ENTRY(0xF0), ENTRY(0xF1), ENTRY(0xF2), ENTRY(0xF3), ENTRY(0xF4),
        ENTRY(0xF5), ENTRY(0xF6), ENTRY(0xF7), ENTRY(0xF8), ENTRY(0xF9), ENTRY(0xFA), ENTRY(0xFB),
        ENTRY(0xFC), ENTRY(0xFD), ENTRY(0xFE), ENTRY(0xFF)
#undef ENTRY
    };

    void DMC_DMA_Load();
    void DMC_DMA_Reload();

    uint16_t OAM_DMA_page = 0x00;
};

#endif
