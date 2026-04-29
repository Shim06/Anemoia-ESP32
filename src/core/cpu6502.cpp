#include "cpu6502.h"
#include "bus.h"

constexpr uint8_t Cpu6502::instr_cycles[256] = {
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, 2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, 2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
    2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, 2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

constexpr uint8_t Cpu6502::zn_table[256] = {
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

#define EXECUTE(addrmode, instruction)                                                             \
    {                                                                                              \
        addrmode();                                                                                \
        instruction();                                                                             \
    }

Cpu6502::Cpu6502()
{
    apu.connectCPU(this);
}

Cpu6502::~Cpu6502()
{
}

inline uint8_t Cpu6502::read(uint16_t addr)
{
    return bus->cpuRead(addr);
}

inline void Cpu6502::write(uint16_t addr, uint8_t data)
{
    bus->cpuWrite(addr, data);
}

IRAM_ATTR void Cpu6502::OAM_DMA(uint8_t page)
{
    OAM_DMA_page = page << 8;
    for (int i = 0; i < 256; i++) { OAM_Write(i, read((OAM_DMA_page) | i)); }

    cycles += 512;
}

IRAM_ATTR void Cpu6502::OAM_Write(uint8_t addr, uint8_t data)
{
    bus->OAM_Write(addr, data);
}

IRAM_ATTR void Cpu6502::clock(int i)
{
    for (int remaining_cycles = i; remaining_cycles > 0; remaining_cycles--)
    {
        if (cycles > 0)
        {
            cycles--;
            cart->cpuCycle(1);
            continue;
        }

        opcode = read(PC++);
        cycles = instr_cycles[opcode];
        additional_cycle1 = 0;
        additional_cycle2 = 0;
        switch (opcode)
        {
        case 0x00: EXECUTE(IMM, Instr_BRK); break;
        case 0x01: EXECUTE(IDX, Instr_ORA); break;
        case 0x02: EXECUTE(IMP, Instr_XXX); break;
        case 0x03: EXECUTE(IMP, Instr_XXX); break;
        case 0x04: EXECUTE(IMP, Instr_NOP); break;
        case 0x05: EXECUTE(ZPG, Instr_ORA); break;
        case 0x06: EXECUTE(ZPG, Instr_ASL); break;
        case 0x07: EXECUTE(IMP, Instr_XXX); break;
        case 0x08: EXECUTE(IMP, Instr_PHP); break;
        case 0x09: EXECUTE(IMM, Instr_ORA); break;
        case 0x0A: EXECUTE(IMP, Instr_ASL); break;
        case 0x0B: EXECUTE(IMP, Instr_XXX); break;
        case 0x0C: EXECUTE(IMP, Instr_NOP); break;
        case 0x0D: EXECUTE(ABS, Instr_ORA); break;
        case 0x0E: EXECUTE(ABS, Instr_ASL); break;
        case 0x0F: EXECUTE(IMP, Instr_XXX); break;

        case 0x10: EXECUTE(REL, Instr_BPL); break;
        case 0x11: EXECUTE(IDY, Instr_ORA); break;
        case 0x12: EXECUTE(IMP, Instr_XXX); break;
        case 0x13: EXECUTE(IMP, Instr_XXX); break;
        case 0x14: EXECUTE(IMP, Instr_NOP); break;
        case 0x15: EXECUTE(ZPX, Instr_ORA); break;
        case 0x16: EXECUTE(ZPX, Instr_ASL); break;
        case 0x17: EXECUTE(IMP, Instr_XXX); break;
        case 0x18: EXECUTE(IMP, Instr_CLC); break;
        case 0x19: EXECUTE(ABY, Instr_ORA); break;
        case 0x1A: EXECUTE(IMP, Instr_NOP); break;
        case 0x1B: EXECUTE(IMP, Instr_XXX); break;
        case 0x1C: EXECUTE(IMP, Instr_NOP); break;
        case 0x1D: EXECUTE(ABX, Instr_ORA); break;
        case 0x1E: EXECUTE(ABX, Instr_ASL); break;
        case 0x1F: EXECUTE(IMP, Instr_XXX); break;

        case 0x20: EXECUTE(ABS, Instr_JSR); break;
        case 0x21: EXECUTE(IDX, Instr_AND); break;
        case 0x22: EXECUTE(IMP, Instr_XXX); break;
        case 0x23: EXECUTE(IMP, Instr_XXX); break;
        case 0x24: EXECUTE(ZPG, Instr_BIT); break;
        case 0x25: EXECUTE(ZPG, Instr_AND); break;
        case 0x26: EXECUTE(ZPG, Instr_ROL); break;
        case 0x27: EXECUTE(IMP, Instr_XXX); break;
        case 0x28: EXECUTE(IMP, Instr_PLP); break;
        case 0x29: EXECUTE(IMM, Instr_AND); break;
        case 0x2A: EXECUTE(IMP, Instr_ROL); break;
        case 0x2B: EXECUTE(IMP, Instr_XXX); break;
        case 0x2C: EXECUTE(ABS, Instr_BIT); break;
        case 0x2D: EXECUTE(ABS, Instr_AND); break;
        case 0x2E: EXECUTE(ABS, Instr_ROL); break;
        case 0x2F: EXECUTE(IMP, Instr_XXX); break;

        case 0x30: EXECUTE(REL, Instr_BMI); break;
        case 0x31: EXECUTE(IDY, Instr_AND); break;
        case 0x32: EXECUTE(IMP, Instr_XXX); break;
        case 0x33: EXECUTE(IMP, Instr_XXX); break;
        case 0x34: EXECUTE(IMP, Instr_NOP); break;
        case 0x35: EXECUTE(ZPX, Instr_AND); break;
        case 0x36: EXECUTE(ZPX, Instr_ROL); break;
        case 0x37: EXECUTE(IMP, Instr_XXX); break;
        case 0x38: EXECUTE(IMP, Instr_SEC); break;
        case 0x39: EXECUTE(ABY, Instr_AND); break;
        case 0x3A: EXECUTE(IMP, Instr_NOP); break;
        case 0x3B: EXECUTE(IMP, Instr_XXX); break;
        case 0x3C: EXECUTE(IMP, Instr_NOP); break;
        case 0x3D: EXECUTE(ABX, Instr_AND); break;
        case 0x3E: EXECUTE(ABX, Instr_ROL); break;
        case 0x3F: EXECUTE(IMP, Instr_XXX); break;

        case 0x40: EXECUTE(IMP, Instr_RTI); break;
        case 0x41: EXECUTE(IDX, Instr_EOR); break;
        case 0x42: EXECUTE(IMP, Instr_XXX); break;
        case 0x43: EXECUTE(IMP, Instr_XXX); break;
        case 0x44: EXECUTE(IMP, Instr_NOP); break;
        case 0x45: EXECUTE(ZPG, Instr_EOR); break;
        case 0x46: EXECUTE(ZPG, Instr_LSR); break;
        case 0x47: EXECUTE(IMP, Instr_XXX); break;
        case 0x48: EXECUTE(IMP, Instr_PHA); break;
        case 0x49: EXECUTE(IMM, Instr_EOR); break;
        case 0x4A: EXECUTE(IMP, Instr_LSR); break;
        case 0x4B: EXECUTE(IMP, Instr_XXX); break;
        case 0x4C: EXECUTE(ABS, Instr_JMP); break;
        case 0x4D: EXECUTE(ABS, Instr_EOR); break;
        case 0x4E: EXECUTE(ABS, Instr_LSR); break;
        case 0x4F: EXECUTE(IMP, Instr_XXX); break;

        case 0x50: EXECUTE(REL, Instr_BVC); break;
        case 0x51: EXECUTE(IDY, Instr_EOR); break;
        case 0x52: EXECUTE(IMP, Instr_XXX); break;
        case 0x53: EXECUTE(IMP, Instr_XXX); break;
        case 0x54: EXECUTE(IMP, Instr_NOP); break;
        case 0x55: EXECUTE(ZPX, Instr_EOR); break;
        case 0x56: EXECUTE(ZPX, Instr_LSR); break;
        case 0x57: EXECUTE(IMP, Instr_XXX); break;
        case 0x58: EXECUTE(IMP, Instr_CLI); break;
        case 0x59: EXECUTE(ABY, Instr_EOR); break;
        case 0x5A: EXECUTE(IMP, Instr_NOP); break;
        case 0x5B: EXECUTE(IMP, Instr_XXX); break;
        case 0x5C: EXECUTE(IMP, Instr_NOP); break;
        case 0x5D: EXECUTE(ABX, Instr_EOR); break;
        case 0x5E: EXECUTE(ABX, Instr_LSR); break;
        case 0x5F: EXECUTE(IMP, Instr_XXX); break;

        case 0x60: EXECUTE(IMP, Instr_RTS); break;
        case 0x61: EXECUTE(IDX, Instr_ADC); break;
        case 0x62: EXECUTE(IMP, Instr_XXX); break;
        case 0x63: EXECUTE(IMP, Instr_XXX); break;
        case 0x64: EXECUTE(IMP, Instr_NOP); break;
        case 0x65: EXECUTE(ZPG, Instr_ADC); break;
        case 0x66: EXECUTE(ZPG, Instr_ROR); break;
        case 0x67: EXECUTE(IMP, Instr_XXX); break;
        case 0x68: EXECUTE(IMP, Instr_PLA); break;
        case 0x69: EXECUTE(IMM, Instr_ADC); break;
        case 0x6A: EXECUTE(IMP, Instr_ROR); break;
        case 0x6B: EXECUTE(IMP, Instr_XXX); break;
        case 0x6C: EXECUTE(IND, Instr_JMP); break;
        case 0x6D: EXECUTE(ABS, Instr_ADC); break;
        case 0x6E: EXECUTE(ABS, Instr_ROR); break;
        case 0x6F: EXECUTE(IMP, Instr_XXX); break;

        case 0x70: EXECUTE(REL, Instr_BVS); break;
        case 0x71: EXECUTE(IDY, Instr_ADC); break;
        case 0x72: EXECUTE(IMP, Instr_XXX); break;
        case 0x73: EXECUTE(IMP, Instr_XXX); break;
        case 0x74: EXECUTE(IMP, Instr_NOP); break;
        case 0x75: EXECUTE(ZPX, Instr_ADC); break;
        case 0x76: EXECUTE(ZPX, Instr_ROR); break;
        case 0x77: EXECUTE(IMP, Instr_XXX); break;
        case 0x78: EXECUTE(IMP, Instr_SEI); break;
        case 0x79: EXECUTE(ABY, Instr_ADC); break;
        case 0x7A: EXECUTE(IMP, Instr_NOP); break;
        case 0x7B: EXECUTE(IMP, Instr_XXX); break;
        case 0x7C: EXECUTE(IMP, Instr_NOP); break;
        case 0x7D: EXECUTE(ABX, Instr_ADC); break;
        case 0x7E: EXECUTE(ABX, Instr_ROR); break;
        case 0x7F: EXECUTE(IMP, Instr_XXX); break;

        case 0x80: EXECUTE(IMP, Instr_NOP); break;
        case 0x81: EXECUTE(IDX, Instr_STA); break;
        case 0x82: EXECUTE(IMP, Instr_NOP); break;
        case 0x83: EXECUTE(IMP, Instr_XXX); break;
        case 0x84: EXECUTE(ZPG, Instr_STY); break;
        case 0x85: EXECUTE(ZPG, Instr_STA); break;
        case 0x86: EXECUTE(ZPG, Instr_STX); break;
        case 0x87: EXECUTE(IMP, Instr_XXX); break;
        case 0x88: EXECUTE(IMP, Instr_DEY); break;
        case 0x89: EXECUTE(IMP, Instr_NOP); break;
        case 0x8A: EXECUTE(IMP, Instr_TXA); break;
        case 0x8B: EXECUTE(IMP, Instr_XXX); break;
        case 0x8C: EXECUTE(ABS, Instr_STY); break;
        case 0x8D: EXECUTE(ABS, Instr_STA); break;
        case 0x8E: EXECUTE(ABS, Instr_STX); break;
        case 0x8F: EXECUTE(IMP, Instr_XXX); break;

        case 0x90: EXECUTE(REL, Instr_BCC); break;
        case 0x91: EXECUTE(IDY, Instr_STA); break;
        case 0x92: EXECUTE(IMP, Instr_XXX); break;
        case 0x93: EXECUTE(IMP, Instr_XXX); break;
        case 0x94: EXECUTE(ZPX, Instr_STY); break;
        case 0x95: EXECUTE(ZPX, Instr_STA); break;
        case 0x96: EXECUTE(ZPY, Instr_STX); break;
        case 0x97: EXECUTE(IMP, Instr_XXX); break;
        case 0x98: EXECUTE(IMP, Instr_TYA); break;
        case 0x99: EXECUTE(ABY, Instr_STA); break;
        case 0x9A: EXECUTE(IMP, Instr_TXS); break;
        case 0x9B: EXECUTE(IMP, Instr_XXX); break;
        case 0x9C: EXECUTE(IMP, Instr_XXX); break;
        case 0x9D: EXECUTE(ABX, Instr_STA); break;
        case 0x9E: EXECUTE(IMP, Instr_XXX); break;
        case 0x9F: EXECUTE(IMP, Instr_XXX); break;

        case 0xA0: EXECUTE(IMM, Instr_LDY); break;
        case 0xA1: EXECUTE(IDX, Instr_LDA); break;
        case 0xA2: EXECUTE(IMM, Instr_LDX); break;
        case 0xA3: EXECUTE(IMP, Instr_XXX); break;
        case 0xA4: EXECUTE(ZPG, Instr_LDY); break;
        case 0xA5: EXECUTE(ZPG, Instr_LDA); break;
        case 0xA6: EXECUTE(ZPG, Instr_LDX); break;
        case 0xA7: EXECUTE(IMP, Instr_XXX); break;
        case 0xA8: EXECUTE(IMP, Instr_TAY); break;
        case 0xA9: EXECUTE(IMM, Instr_LDA); break;
        case 0xAA: EXECUTE(IMP, Instr_TAX); break;
        case 0xAB: EXECUTE(IMP, Instr_XXX); break;
        case 0xAC: EXECUTE(ABS, Instr_LDY); break;
        case 0xAD: EXECUTE(ABS, Instr_LDA); break;
        case 0xAE: EXECUTE(ABS, Instr_LDX); break;
        case 0xAF: EXECUTE(IMP, Instr_XXX); break;

        case 0xB0: EXECUTE(REL, Instr_BCS); break;
        case 0xB1: EXECUTE(IDY, Instr_LDA); break;
        case 0xB2: EXECUTE(IMP, Instr_XXX); break;
        case 0xB3: EXECUTE(IMP, Instr_XXX); break;
        case 0xB4: EXECUTE(ZPX, Instr_LDY); break;
        case 0xB5: EXECUTE(ZPX, Instr_LDA); break;
        case 0xB6: EXECUTE(ZPY, Instr_LDX); break;
        case 0xB7: EXECUTE(IMP, Instr_XXX); break;
        case 0xB8: EXECUTE(IMP, Instr_CLV); break;
        case 0xB9: EXECUTE(ABY, Instr_LDA); break;
        case 0xBA: EXECUTE(IMP, Instr_TSX); break;
        case 0xBB: EXECUTE(IMP, Instr_XXX); break;
        case 0xBC: EXECUTE(ABX, Instr_LDY); break;
        case 0xBD: EXECUTE(ABX, Instr_LDA); break;
        case 0xBE: EXECUTE(ABY, Instr_LDX); break;
        case 0xBF: EXECUTE(IMP, Instr_XXX); break;

        case 0xC0: EXECUTE(IMM, Instr_CPY); break;
        case 0xC1: EXECUTE(IDX, Instr_CMP); break;
        case 0xC2: EXECUTE(IMP, Instr_NOP); break;
        case 0xC3: EXECUTE(IMP, Instr_XXX); break;
        case 0xC4: EXECUTE(ZPG, Instr_CPY); break;
        case 0xC5: EXECUTE(ZPG, Instr_CMP); break;
        case 0xC6: EXECUTE(ZPG, Instr_DEC); break;
        case 0xC7: EXECUTE(IMP, Instr_XXX); break;
        case 0xC8: EXECUTE(IMP, Instr_INY); break;
        case 0xC9: EXECUTE(IMM, Instr_CMP); break;
        case 0xCA: EXECUTE(IMP, Instr_DEX); break;
        case 0xCB: EXECUTE(IMP, Instr_XXX); break;
        case 0xCC: EXECUTE(ABS, Instr_CPY); break;
        case 0xCD: EXECUTE(ABS, Instr_CMP); break;
        case 0xCE: EXECUTE(ABS, Instr_DEC); break;
        case 0xCF: EXECUTE(IMP, Instr_XXX); break;

        case 0xD0: EXECUTE(REL, Instr_BNE); break;
        case 0xD1: EXECUTE(IDY, Instr_CMP); break;
        case 0xD2: EXECUTE(IMP, Instr_XXX); break;
        case 0xD3: EXECUTE(IMP, Instr_XXX); break;
        case 0xD4: EXECUTE(IMP, Instr_NOP); break;
        case 0xD5: EXECUTE(ZPX, Instr_CMP); break;
        case 0xD6: EXECUTE(ZPX, Instr_DEC); break;
        case 0xD7: EXECUTE(IMP, Instr_XXX); break;
        case 0xD8: EXECUTE(IMP, Instr_CLD); break;
        case 0xD9: EXECUTE(ABY, Instr_CMP); break;
        case 0xDA: EXECUTE(IMP, Instr_NOP); break;
        case 0xDB: EXECUTE(IMP, Instr_XXX); break;
        case 0xDC: EXECUTE(IMP, Instr_NOP); break;
        case 0xDD: EXECUTE(ABX, Instr_CMP); break;
        case 0xDE: EXECUTE(ABX, Instr_DEC); break;
        case 0xDF: EXECUTE(IMP, Instr_XXX); break;

        case 0xE0: EXECUTE(IMM, Instr_CPX); break;
        case 0xE1: EXECUTE(IDX, Instr_SBC); break;
        case 0xE2: EXECUTE(IMP, Instr_NOP); break;
        case 0xE3: EXECUTE(IMP, Instr_XXX); break;
        case 0xE4: EXECUTE(ZPG, Instr_CPX); break;
        case 0xE5: EXECUTE(ZPG, Instr_SBC); break;
        case 0xE6: EXECUTE(ZPG, Instr_INC); break;
        case 0xE7: EXECUTE(IMP, Instr_XXX); break;
        case 0xE8: EXECUTE(IMP, Instr_INX); break;
        case 0xE9: EXECUTE(IMM, Instr_SBC); break;
        case 0xEA: EXECUTE(IMP, Instr_NOP); break;
        case 0xEB: EXECUTE(IMP, Instr_XXX); break;
        case 0xEC: EXECUTE(ABS, Instr_CPX); break;
        case 0xED: EXECUTE(ABS, Instr_SBC); break;
        case 0xEE: EXECUTE(ABS, Instr_INC); break;
        case 0xEF: EXECUTE(IMP, Instr_XXX); break;

        case 0xF0: EXECUTE(REL, Instr_BEQ); break;
        case 0xF1: EXECUTE(IDY, Instr_SBC); break;
        case 0xF2: EXECUTE(IMP, Instr_XXX); break;
        case 0xF3: EXECUTE(IMP, Instr_XXX); break;
        case 0xF4: EXECUTE(IMP, Instr_NOP); break;
        case 0xF5: EXECUTE(ZPX, Instr_SBC); break;
        case 0xF6: EXECUTE(ZPX, Instr_INC); break;
        case 0xF7: EXECUTE(IMP, Instr_XXX); break;
        case 0xF8: EXECUTE(IMP, Instr_SED); break;
        case 0xF9: EXECUTE(ABY, Instr_SBC); break;
        case 0xFA: EXECUTE(IMP, Instr_NOP); break;
        case 0xFB: EXECUTE(IMP, Instr_XXX); break;
        case 0xFC: EXECUTE(IMP, Instr_NOP); break;
        case 0xFD: EXECUTE(ABX, Instr_SBC); break;
        case 0xFE: EXECUTE(ABX, Instr_INC); break;
        case 0xFF: EXECUTE(IMP, Instr_XXX); break;

        default: break;
        }

        cycles += (additional_cycle1 & additional_cycle2);
        addrmode_implied = false;

        if (remaining_cycles >= cycles)
        {
            remaining_cycles -= (cycles - 1);
            cart->cpuCycle(cycles);
            cycles = 0;
            continue;
        }
        else
        {
            cycles -= remaining_cycles;
            cart->cpuCycle(remaining_cycles);
            return;
        }
    }
}

void Cpu6502::reset()
{
    addr_abs = 0xFFFC;
    uint8_t low_byte = read(addr_abs);
    uint8_t high_byte = read(addr_abs + 1);

    PC = (high_byte << 8) | low_byte;
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD;
    status = 0x00 | U;

    addr_rel = 0x0000;
    addr_abs = 0x0000;
    fetched = 0x00;

    cycles = 8;
}

IRAM_ATTR void Cpu6502::apuWrite(uint16_t addr, uint8_t data)
{
    apu.cpuWrite(addr, data);
}

inline uint8_t Cpu6502::fetch()
{
    if (addrmode_implied == false) fetched = read(addr_abs);
    return fetched;
}

inline void Cpu6502::ABS()
{
    uint8_t low_byte = read(PC++);
    uint8_t high_byte = read(PC++);

    addr_abs = (high_byte << 8) | low_byte;
}

inline void Cpu6502::ABX()
{
    uint8_t low_byte = read(PC++);
    uint8_t high_byte = read(PC++);

    addr_abs = (high_byte << 8) | low_byte;
    addr_abs += X;

    if ((addr_abs & 0xFF00) != (high_byte << 8)) additional_cycle1 = 1;
}

inline void Cpu6502::ABY()
{
    uint8_t low_byte = read(PC++);
    uint8_t high_byte = read(PC++);

    addr_abs = (high_byte << 8) | low_byte;
    addr_abs += Y;

    if ((addr_abs & 0xFF00) != (high_byte << 8)) additional_cycle1 = 1;
}

inline void Cpu6502::IMM()
{
    addr_abs = PC++;
}

inline void Cpu6502::IMP()
{
    fetched = A;
    addrmode_implied = true;
}

inline void Cpu6502::IND()
{
    uint8_t low_byte = read(PC++);
    uint8_t high_byte = read(PC++);

    uint16_t ptr = (high_byte << 8) | low_byte;

    if (low_byte == 0xFF) addr_abs = (read(ptr & 0xFF00) << 8) | read(ptr);
    else addr_abs = (read(ptr + 1) << 8) | read(ptr);
}

inline void Cpu6502::IDX()
{
    uint8_t temp = read(PC++);

    uint8_t low_byte = read((uint16_t)(temp + (uint16_t)X) & 0x00FF);
    uint8_t high_byte = read((uint16_t)(temp + (uint16_t)X + 1) & 0x00FF);

    addr_abs = (high_byte << 8) | low_byte;
}

inline void Cpu6502::IDY()
{
    uint8_t temp = read(PC++);

    uint8_t low_byte = read(temp & 0x00FF);
    uint8_t high_byte = read((temp + 1) & 0x00FF);

    addr_abs = (high_byte << 8) | low_byte;
    addr_abs += Y;

    if ((addr_abs & 0xFF00) != (high_byte << 8)) additional_cycle1 = 1;
}

inline void Cpu6502::REL()
{
    addr_rel = read(PC++);
    if (addr_rel & 0x80) addr_rel |= 0xFF00;
}

inline void Cpu6502::ZPG()
{
    addr_abs = read(PC++);
}

inline void Cpu6502::ZPX()
{
    addr_abs = read(PC++) + X;
    addr_abs &= 0x00FF;
}

inline void Cpu6502::ZPY()
{
    addr_abs = read(PC++) + Y;
    addr_abs &= 0x00FF;
}

inline void Cpu6502::Instr_LDA()
{
    A = read(addr_abs);
    SET_ZN(A);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_LDX()
{
    X = read(addr_abs);
    SET_ZN(X);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_LDY()
{
    Y = read(addr_abs);
    SET_ZN(Y);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_STA()
{
    write(addr_abs, A);
}

inline void Cpu6502::Instr_STX()
{
    write(addr_abs, X);
}

inline void Cpu6502::Instr_STY()
{
    write(addr_abs, Y);
}

inline void Cpu6502::Instr_TAX()
{
    X = A;
    SET_ZN(X);
}

inline void Cpu6502::Instr_TAY()
{
    Y = A;
    SET_ZN(Y);
}

inline void Cpu6502::Instr_TSX()
{
    X = SP;
    SET_ZN(X);
}

inline void Cpu6502::Instr_TXA()
{
    A = X;
    SET_ZN(A);
}

inline void Cpu6502::Instr_TXS()
{
    SP = X;
}

inline void Cpu6502::Instr_TYA()
{
    A = Y;
    SET_ZN(A);
}

inline void Cpu6502::Instr_PHA()
{
    write(0x0100 + SP, A);
    SP--;
}

inline void Cpu6502::Instr_PHP()
{
    write(0x0100 + SP, status | B | U);
    status &= ~B;
    status |= U;
    SP--;
}

inline void Cpu6502::Instr_PLA()
{
    SP++;
    A = read(0x0100 + SP);
    SET_ZN(A);
}

inline void Cpu6502::Instr_PLP()
{
    SP++;
    status = read(0x0100 + SP);
    status &= ~B;
    status |= U;
}

inline void Cpu6502::Instr_DEC()
{
    temp = (read(addr_abs) - 1) & 0x00FF;
    SET_ZN(temp);
    write(addr_abs, (uint8_t)temp);
}

inline void Cpu6502::Instr_DEX()
{
    X--;
    SET_ZN(X);
}

inline void Cpu6502::Instr_DEY()
{
    Y--;
    SET_ZN(Y);
}

inline void Cpu6502::Instr_INC()
{
    temp = (read(addr_abs) + 1) & 0x00FF;
    SET_ZN(temp);
    write(addr_abs, (uint8_t)temp);
}

inline void Cpu6502::Instr_INX()
{
    X++;
    SET_ZN(X);
}

inline void Cpu6502::Instr_INY()
{
    Y++;
    SET_ZN(Y);
}

inline void Cpu6502::Instr_ADC()
{
    fetched = read(addr_abs);
    temp = (uint16_t)A + (uint16_t)fetched + (uint16_t)GET_FLAG(C);
    SET_FLAG(C, temp > 255);
    SET_FLAG(V, ((~((uint16_t)A ^ (uint16_t)fetched) & ((uint16_t)A ^ temp)) & 0x0080) != 0);
    A = temp & 0x00FF;
    SET_ZN(A);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_SBC()
{
    uint16_t value = ((uint16_t)read(addr_abs)) ^ 0x00FF;

    temp = (uint16_t)A + value + (uint16_t)GET_FLAG(C);
    SET_FLAG(C, temp > 255);
    SET_FLAG(V, ((temp ^ (uint16_t)A) & (temp ^ value) & 0x0080) != 0);
    A = temp & 0x00FF;
    SET_ZN(A);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_AND()
{
    A = A & read(addr_abs);
    SET_ZN(A);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_EOR()
{
    A = A ^ read(addr_abs);
    SET_ZN(A);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_ORA()
{
    A = A | read(addr_abs);
    SET_ZN(A);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_ASL()
{
    fetch();
    temp = (uint16_t)fetched << 1;
    SET_FLAG(C, (temp & 0xFF00) > 0);
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
}

inline void Cpu6502::Instr_LSR()
{
    fetch();
    SET_FLAG(C, (fetched & 0x0001) != 0);
    temp = fetched >> 1;
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
}

inline void Cpu6502::Instr_ROL()
{
    fetch();
    temp = (uint16_t)(fetched << 1) | GET_FLAG(C);
    SET_FLAG(C, (temp & 0xFF00) != 0);
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
}

inline void Cpu6502::Instr_ROR()
{
    fetch();
    temp = (uint16_t)(GET_FLAG(C) << 7) | (fetched >> 1);
    SET_FLAG(C, (fetched & 0x01) != 0);
    SET_ZN(temp & 0x00FF);
    if (addrmode_implied) A = temp & 0x00FF;
    else write(addr_abs, temp & 0x00FF);
}

inline void Cpu6502::Instr_CLC()
{
    status &= ~C;
}

inline void Cpu6502::Instr_CLD()
{
    status &= ~D;
}

inline void Cpu6502::Instr_CLI()
{
    status &= ~I;
}

inline void Cpu6502::Instr_CLV()
{
    status &= ~V;
}

inline void Cpu6502::Instr_SEC()
{
    status |= C;
}

inline void Cpu6502::Instr_SED()
{
    status |= D;
}

inline void Cpu6502::Instr_SEI()
{
    status |= I;
}

inline void Cpu6502::Instr_CMP()
{
    fetched = read(addr_abs);
    temp = (uint16_t)A - (uint16_t)fetched;
    SET_FLAG(C, A >= fetched);
    SET_ZN(temp & 0x00FF);
    additional_cycle2 = 1;
}

inline void Cpu6502::Instr_CPX()
{
    fetched = read(addr_abs);
    temp = (uint16_t)X - (uint16_t)fetched;
    SET_FLAG(C, X >= fetched);
    SET_ZN(temp & 0x00FF);
}

inline void Cpu6502::Instr_CPY()
{
    fetched = read(addr_abs);
    temp = (uint16_t)Y - (uint16_t)fetched;
    SET_FLAG(C, Y >= fetched);
    SET_ZN(temp & 0x00FF);
}

inline void Cpu6502::Instr_BCC()
{
    if (GET_FLAG(C) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BCS()
{
    if (GET_FLAG(C) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BEQ()
{
    if (GET_FLAG(Z) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BMI()
{
    if (GET_FLAG(N) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BNE()
{
    if (GET_FLAG(Z) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BPL()
{
    if (GET_FLAG(N) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BVC()
{
    if (GET_FLAG(V) == 0)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_BVS()
{
    if (GET_FLAG(V) == 1)
    {
        cycles++;
        addr_abs = PC + addr_rel;

        if ((addr_abs & 0xFF00) != (PC & 0xFF00)) cycles++;

        PC = addr_abs;
    }
}

inline void Cpu6502::Instr_JMP()
{
    PC = addr_abs;
}

inline void Cpu6502::Instr_JSR()
{
    PC--;
    write(0x0100 + SP, (PC >> 8) & 0x00FF);
    write(0x0100 + (uint8_t)(SP - 1), PC & 0x00FF);

    SP -= 2;
    PC = addr_abs;
}

inline void Cpu6502::Instr_RTS()
{
    PC = read(0x0100 | (uint8_t)(SP + 1));
    PC |= read(0x0100 | (uint8_t)(SP + 2)) << 8;

    SP += 2;
    PC++;
}

inline void Cpu6502::Instr_BRK()
{
    write(0x0100 | SP, (PC >> 8) & 0x00FF);
    write(0x0100 | (uint8_t)(SP - 1), PC & 0x00FF);
    status |= (U | B);
    write(0x0100 | (uint8_t)(SP - 2), status);
    status &= ~B;
    status |= I;

    uint8_t low_byte = read(0xFFFE);
    uint8_t high_byte = read(0xFFFF);

    SP -= 3;
    PC = (high_byte << 8) | low_byte;
}

inline void Cpu6502::Instr_RTI()
{
    status = read(0x0100 | (uint8_t)(SP + 1));
    status &= ~B;
    status |= U;

    PC = (uint16_t)read(0x0100 | (uint8_t)(SP + 2));
    PC |= (uint16_t)read(0x0100 | (uint8_t)(SP + 3)) << 8;
    SP += 3;
}

inline void Cpu6502::Instr_BIT()
{
    fetched = read(addr_abs);
    temp = A & fetched;
    SET_FLAG(Z, temp == 0);
    SET_FLAG(N, fetched & 0x80);
    SET_FLAG(V, fetched & 0x40);
}

inline void Cpu6502::Instr_NOP()
{
    return;
}

inline void Cpu6502::Instr_XXX()
{
    return;
}

void Cpu6502::IRQ()
{
    if (GET_FLAG(I) == 0)
    {
        write(0x0100 | SP, (PC >> 8) & 0x00FF);
        write(0x0100 | (uint8_t)(SP - 1), PC & 0x00FF);

        write(0x0100 | (uint8_t)(SP - 2), status);

        status |= I;

        uint8_t low_byte = read(0xFFFE);
        uint8_t high_byte = read(0xFFFF);

        PC = (high_byte << 8) | low_byte;

        SP -= 3;
        cycles += 7;
    }
}

void Cpu6502::NMI()
{
    write(0x0100 | SP, (PC >> 8) & 0x00FF);
    write(0x0100 | (uint8_t)(SP - 1), PC & 0x00FF);

    write(0x0100 | (uint8_t)(SP - 2), status);

    status |= I;

    uint8_t low_byte = read(0xFFFA);
    uint8_t high_byte = read(0xFFFB);

    PC = (high_byte << 8) | low_byte;

    SP -= 3;
    cycles += 8;
}

void Cpu6502::dumpState(File& state)
{
    state.write((uint8_t*)&A, sizeof(A));
    state.write((uint8_t*)&X, sizeof(X));
    state.write((uint8_t*)&Y, sizeof(Y));
    state.write((uint8_t*)&PC, sizeof(PC));
    state.write((uint8_t*)&SP, sizeof(SP));
    state.write((uint8_t*)&status, sizeof(status));

    state.write((uint8_t*)&fetched, sizeof(fetched));
    state.write((uint8_t*)&addr_abs, sizeof(addr_abs));
    state.write((uint8_t*)&addr_rel, sizeof(addr_rel));
    state.write((uint8_t*)&opcode, sizeof(opcode));
    state.write((uint8_t*)&cycles, sizeof(cycles));
    state.write((uint8_t*)&temp, sizeof(temp));

    state.write((uint8_t*)&addrmode_implied, sizeof(addrmode_implied));
    state.write((uint8_t*)&additional_cycle1, sizeof(additional_cycle1));
    state.write((uint8_t*)&additional_cycle2, sizeof(additional_cycle2));
    state.write((uint8_t*)&OAM_DMA_page, sizeof(OAM_DMA_page));
}

void Cpu6502::loadState(File& state)
{
    state.read((uint8_t*)&A, sizeof(A));
    state.read((uint8_t*)&X, sizeof(X));
    state.read((uint8_t*)&Y, sizeof(Y));
    state.read((uint8_t*)&PC, sizeof(PC));
    state.read((uint8_t*)&SP, sizeof(SP));
    state.read((uint8_t*)&status, sizeof(status));

    state.read((uint8_t*)&fetched, sizeof(fetched));
    state.read((uint8_t*)&addr_abs, sizeof(addr_abs));
    state.read((uint8_t*)&addr_rel, sizeof(addr_rel));
    state.read((uint8_t*)&opcode, sizeof(opcode));
    state.read((uint8_t*)&cycles, sizeof(cycles));
    state.read((uint8_t*)&temp, sizeof(temp));

    state.read((uint8_t*)&addrmode_implied, sizeof(addrmode_implied));
    state.read((uint8_t*)&additional_cycle1, sizeof(additional_cycle1));
    state.read((uint8_t*)&additional_cycle2, sizeof(additional_cycle2));
    state.read((uint8_t*)&OAM_DMA_page, sizeof(OAM_DMA_page));
}