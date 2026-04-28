#ifndef CPU6502_H
#define CPU6502_H

#include <Arduino.h>
#include <SD.h>
#include <stdint.h>

#include "apu2A03.h"
#include "cartridge.h"

#define GET_FLAG(f) ((status & (f)) != 0)
#define SET_FLAG(f, v) (status = (v) ? (status | (f)) : (status & ~(f)))
#define SET_ZN(v) (status = ((status & ~(Z | N)) | zn_table[(v)]))

class Bus;
class Cpu6502
{
public:
    Cpu6502();
    ~Cpu6502();

public:
    Apu2A03 apu;

    // Status Register Flags
    enum FLAGS
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

    void connectBus(Bus* n) { bus = n; }
    void connectCartridge(Cartridge* cartridge) { cart = cartridge; }

    // Registers
    uint8_t A = 0x00; // Accumulator
    uint8_t X = 0x00; // X Index
    uint8_t Y = 0x00; // Y Index
    uint16_t PC = 0x0000; // Program Counter
    uint8_t SP = 0x00; // Stack Pointer
    uint8_t status = 0x00; // Status register

    uint8_t fetched = 0x00;
    uint16_t addr_abs = 0x0000;
    uint16_t addr_rel = 0x0000;
    uint8_t opcode = 0x00;
    uint16_t cycles = 0;
    uint16_t temp = 0x0000;

private:
    Cartridge* __restrict cart = nullptr;
    Bus* __restrict bus = nullptr;
    bool addrmode_implied = false;
    uint8_t additional_cycle1 = 0;
    uint8_t additional_cycle2 = 0;
    uint8_t fetch();
    uint8_t fast_fetch();
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    void OAM_Write(uint8_t addr, uint8_t data);

	// Addressing Modes
	void ABS();	void IDX();
	void ABX();	void IDY();
	void ABY();	void REL();
	void IMM();	void ZPG();
	void IMP();	void ZPX();
	void IND();	void ZPY();

	// Instructions
	void Instr_ADC(); void Instr_CLI(); void Instr_LDX(); void Instr_SED();
	void Instr_AND(); void Instr_CLV(); void Instr_LDY(); void Instr_SEI();
	void Instr_ASL(); void Instr_CMP(); void Instr_LSR(); void Instr_STA();
	void Instr_BCC(); void Instr_CPX(); void Instr_NOP(); void Instr_STX();
	void Instr_BCS(); void Instr_CPY(); void Instr_ORA(); void Instr_STY();
	void Instr_BEQ(); void Instr_DEC(); void Instr_PHA(); void Instr_TAX();
	void Instr_BIT(); void Instr_DEX(); void Instr_PHP(); void Instr_TAY();
	void Instr_BMI(); void Instr_DEY(); void Instr_PLA(); void Instr_TSX();
	void Instr_BNE(); void Instr_EOR(); void Instr_PLP(); void Instr_TXA();
	void Instr_BPL(); void Instr_INC(); void Instr_ROL(); void Instr_TXS();
	void Instr_BRK(); void Instr_INX(); void Instr_ROR(); void Instr_TYA();
	void Instr_BVC(); void Instr_INY(); void Instr_RTI(); void Instr_CLD();
	void Instr_BVS(); void Instr_JMP(); void Instr_RTS(); void Instr_LDA();
	void Instr_CLC(); void Instr_JSR(); void Instr_SBC(); void Instr_SEC();
	void Instr_XXX();

    // Instruction cycle count
    static const uint8_t instr_cycles[256];
    static const uint8_t zn_table[256];

    void DMC_DMA_Load();
    void DMC_DMA_Reload();

    uint16_t OAM_DMA_page = 0x00;
};

#endif