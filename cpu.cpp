#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include <map>

typedef int8_t		i8;
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

Registers regs;

void resetCPU(u16 reset_vector) {
	regs.PC = reset_vector;
	regs.resetStackPointer();
	//printf("Reset CPU, starting at PC: %x\n", regs.PC);
}


//		Helper

//	8-bit / 16-bit wide Y-Index templates (getter / setter)
void pushToStack(u8 val) {
	regs.setSP(regs.getSP() - 1);
	writeToMem(val, regs.getSP() + 1, 0);
}
void pushToStack(u16 val) {
	regs.setSP(regs.getSP() - 2);
	writeToMem(val >> 8, regs.getSP() + 2, 0);
	writeToMem(val & 0xff, regs.getSP() + 1, 0);
}
u8 pullFromStack() {
	u8 val = readFromMem(regs.getSP() + 1, 0);
	regs.setSP(regs.getSP() + 1);
	return val;
}
//	return P as readable string
string byteToBinaryString(u8 val) {

	//	alpha representation (bsnes-like)
	string s = "";
	s += regs.P.getNegative() ? "N" : "n";
	s += regs.P.getOverflow() ? "V" : "v";
	if (regs.P.getEmulation()) {
		s += "1";
		s += regs.P.getBreak() ? "B" : "b";
	}
	else {
		s += regs.P.getAccuMemSize() ? "M" : "m";
		s += regs.P.getIndexSize() ? "X" : "x";
	}
	s += regs.P.getDecimal() ? "D" : "d";
	s += regs.P.getIRQDisable() ? "I" : "i";
	s += regs.P.getZero() ? "Z" : "z";
	s += regs.P.getCarry() ? "C" : "c";

	//	binary representation
	/*string s = "";
	for (u8 i = 0; i < 8; i++) {
		s += to_string((val >> (7 - i)) & 1);
	}*/
	return s;
}


//		Instructions

//	nop
u8 NOP() {
	regs.PC++;
	return 2;
}

//	set interrupt disable flag
u8 SEI() {
	regs.P.setIRQDisable(1);
	regs.PC += 1;
	return 2;
}

//	clear carry
u8 CLC() {
	regs.P.setCarry(0);
	regs.PC += 1;
	return 2;
}

//	exchange carry and emulation flags
u8 XCE() {
	u8 tmp = regs.P.getCarry();
	regs.P.setCarry(regs.P.getEmulation());
	regs.P.setEmulation(tmp);
	if (tmp == 0) {	//	if emulation mode
		regs.P.setAccuMemSize(1);
		regs.P.setIndexSize(1, &regs);
	}
	regs.PC += 1;
	return 2;
}

//	push program bank register to stack
u8 PHK() {
	pushToStack(regs.getProgramBankRegister());
	regs.PC += 1;
	return 3;
}

//	pull data bank register from stack
u8 PLB() {
	regs.setDataBankRegister(pullFromStack());
	if (regs.P.getEmulation()) {
		regs.P.setAccuMemSize(1);
		regs.P.setIndexSize(1, &regs);
	}
	regs.P.setNegative((regs.getDataBankRegister() & 0x80) == 0x80 );
	regs.P.setZero(regs.getDataBankRegister() == 0x00);
	regs.PC += 1;
	return 4;
}

//	reset status bits (reset all bits set in the immediate value)
u8 REP() {
	u8 v = readFromMem(regs.PC+1);
	u8 N = regs.P.getNegative() & ~((v >> 7) & 1);
	u8 V = regs.P.getOverflow() & ~((v >> 6) & 1);
	u8 M = regs.P.getAccuMemSize() & ~((v >> 5) & 1);
	u8 X = regs.P.getIndexSize() & ~((v >> 4) & 1);
	u8 B = regs.P.getBreak() & ~((v >> 4) & 1);
	u8 D = regs.P.getDecimal() & ~((v >> 3) & 1);
	u8 I = regs.P.getIRQDisable() & ~((v >> 2) & 1);
	u8 Z = regs.P.getZero() & ~((v >> 1) & 1);

	regs.P.setNegative(N);
	regs.P.setOverflow(V);
	if (!regs.P.getEmulation()) {
		regs.P.setAccuMemSize(M);
		regs.P.setIndexSize(X, &regs);
	}
	else {
		regs.P.setBreak(B);
		regs.P.setAccuMemSize(1);
		regs.P.setIndexSize(1, &regs);
	}
	regs.P.setDecimal(D);
	regs.P.setIRQDisable(I);
	regs.P.setZero(Z);

	regs.PC += 2;
	return 3;
}

//	set status bits (set all bits set in the immediate value)
u8 SEP() {
	u8 v = readFromMem(regs.PC + 1);
	u8 N = regs.P.getNegative() | ((v >> 7) & 1);
	u8 V = regs.P.getOverflow() | ((v >> 6) & 1);
	u8 M = regs.P.getAccuMemSize() | ((v >> 5) & 1);
	u8 X = regs.P.getIndexSize() | ((v >> 4) & 1);
	u8 B = regs.P.getBreak() | ((v >> 4) & 1);
	u8 D = regs.P.getDecimal() | ((v >> 3) & 1);
	u8 I = regs.P.getIRQDisable() | ((v >> 2) & 1);
	u8 Z = regs.P.getZero() | ((v >> 1) & 1);

	regs.P.setNegative(N);
	regs.P.setOverflow(V);
	if (!regs.P.getEmulation()) {
		regs.P.setAccuMemSize(M);
		regs.P.setIndexSize(X, &regs);
	}
	else {
		regs.P.setBreak(B);
		regs.P.setAccuMemSize(1);
		regs.P.setIndexSize(1, &regs);
	}
	regs.P.setDecimal(D);
	regs.P.setIRQDisable(I);
	regs.P.setZero(Z);

	regs.PC += 2;
	return 3;
}

//	Decrement X
u8 DEX() {
	regs.setX((u16)(regs.getX()-1));
	regs.P.setZero(regs.getX() == 0);
	regs.P.setNegative(regs.getX() >> (7 + ((1 - regs.P.getIndexSize()) * 8)));
	regs.PC++;
	return 2;
}

//	Decrement Y
u8 DEY() {
	regs.setY((u16)(regs.getY() - 1));
	regs.P.setZero(regs.getY() == 0);
	regs.P.setNegative(regs.getY() >> (7 + ((1 - regs.P.getIndexSize()) * 8)));
	regs.PC++;
	return 2;
}

//	Test memory bits against Accumulator
u8 BIT(u16(*f)(), u8 bytes, u8 cycles, bool is_immediate) {
	
	if (bytes == 2) {
		u8 val = readFromMem(f(), regs.getProgramBankRegister());
		if (!is_immediate) {
			regs.P.setNegative(val >> 7);
			regs.P.setOverflow(val >> 6);
		}
		regs.P.setZero((val & regs.getAccumulator()) == 0x00);
	} 
	else if (bytes == 3) {
		u8 lo = readFromMem(f(), regs.getProgramBankRegister());
		u8 hi = readFromMem(f(), regs.getProgramBankRegister());
		u16 val = (hi << 8) | lo;
		if (!is_immediate) {
			regs.P.setNegative(val >> 15);
			regs.P.setOverflow(val >> 14);
		}
		regs.P.setZero((val & regs.getAccumulator()) == 0x0000);
	}
	regs.PC++;
	return cycles;
}


//		Transfers

//	Transfer 16 bit A to DP
u8 TCD() {
	regs.setDirectPageRegister(regs.getAccumulator());
	regs.P.setZero(regs.getDirectPageRegister() == 0);
	regs.P.setNegative(regs.getDirectPageRegister() >> 15);
	regs.PC++;
	return 2;
}

//	Transfer X to SP
u8 TXS() {
	if (regs.P.getEmulation()) {
		regs.setSP(0x100 | (regs.getX() & 0xff));
	} 
	else {
		regs.setSP(regs.getX());
	}
	regs.P.setZero(regs.getX() == 0);
	regs.P.setNegative(regs.getX() >> (7 + ((1 - regs.P.getAccuMemSize()) * 8)));
	regs.PC++;
	return 2;
}


//		Loads

//	Load accumulator from memory
u8 LDA(u16 (*f)(), u8 bytes, u8 cycles) {
	u16 adr = f();
	if (bytes == 2) {
		u8 lo = readFromMem(adr, regs.getProgramBankRegister());
		regs.setAccumulator(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else if (bytes == 3) {
		u8 lo = readFromMem(adr, regs.getProgramBankRegister());
		u8 hi = readFromMem(adr + 1, regs.getProgramBankRegister());
		u16 val = (hi << 8) | lo;
		regs.setAccumulator(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC += (bytes-1);
	return cycles;
}

//	Load X-Register from memory
u8 LDX(u16(*f)(), u8 bytes, u8 cycles) {
	u16 adr = f();
	if (bytes == 2) {
		u8 lo = readFromMem(adr, regs.getProgramBankRegister());
		regs.setX(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else if (bytes == 3) {
		u8 lo = readFromMem(adr, regs.getProgramBankRegister());
		u8 hi = readFromMem(adr + 1, regs.getProgramBankRegister());
		u16 val = (hi << 8) | lo;
		regs.setX(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC += (bytes-1);
	return 2;
}

//	Load Y-Register from memory
u8 LDY(u16(*f)(), u8 bytes, u8 cycles) {
	u16 adr = f();
	if (bytes == 2) {
		u8 lo = readFromMem(adr, regs.getProgramBankRegister());
		regs.setY(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else if (bytes == 3) {
		u8 lo = readFromMem(adr, regs.getProgramBankRegister());
		u8 hi = readFromMem(adr+1, regs.getProgramBankRegister());
		u16 val = (hi << 8) | lo;
		regs.setY(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC += (bytes-1);
	return cycles;
}


//		Stores

//	Store accumulator to memory
u8 STA(u16 (*f)(), u8 bytes, u8 cycles) {
	writeToMem(regs.getAccumulator(), f(), regs.getDataBankRegister());
	regs.PC += (bytes-1);
	return cycles;
}
//	Store zero to memory
u8 STZ(u16 (*f)(), u8 bytes, u8 cycles) {
	writeToMem(0x00, f(), regs.getDataBankRegister());
	regs.PC += (bytes - 1);
	return cycles;
}
//	Store X-Register to memory
u8 STX(u16(*f)(), u8 bytes, u8 cycles) {
	writeToMem(regs.getX(), f(), regs.getDataBankRegister());
	regs.PC += (bytes - 1);
	return cycles;
}
//	Store Y-Register to memory
u8 STY(u16 (*f)(), u8 bytes, u8 cycles) {
	writeToMem(regs.getY(), f(), regs.getDataBankRegister());
	regs.PC += (bytes - 1);
	return cycles;
}


//		Branches

//	Branch if not equal (Z = 0)
u8 BNE(u16 (*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f(), regs.getProgramBankRegister());
	regs.PC++;
	if (regs.P.getZero() == 0) {
		regs.PC += offset;
		branch_taken = 1;
	} 
	return cycles + branch_taken;
}


//		Jumps

//	Jump Long
u8 JML(u32 (*f)()) {
	regs.PC = f();
	regs.setProgramBankRegister((regs.PC >> 16) & 0xff);
	return 4;
}



//		Addressing modes

u16 ADDR_getImmediate() {
	regs.PC++;
	return regs.PC;
}
u16 ADDR_getAbsolute() {
	regs.PC++;
	return ((readFromMem(regs.PC, regs.getProgramBankRegister()) << 8) | readFromMem(regs.PC-1, regs.getProgramBankRegister()));
}
u32 ADDR_getLong() {
	regs.PC++;
	return (u16)((readFromMem(regs.PC, regs.getProgramBankRegister()) << 16) | ((readFromMem(regs.PC - 1, regs.getProgramBankRegister())) << 8) | (readFromMem(regs.PC - 2, regs.getProgramBankRegister())));
}


u8 stepCPU() {
	string flags = byteToBinaryString(regs.P.getByte());
	//printf("Op: %02x %02x %02x %02x  PC : 0x%04x A: 0x%04x X: 0x%04x Y: 0x%04x SP: 0x%04x D: 0x%04x DB: 0x%02x P: %s (0x%02x) Emu: %s\n", readFromMem(regs.PC, regs.getDataBankRegister()), readFromMem(regs.PC+1, regs.getDataBankRegister()), readFromMem(regs.PC+2, regs.getDataBankRegister()), readFromMem(regs.PC + 3, regs.getDataBankRegister()), regs.PC, regs.getAccumulator(), regs.getX(), regs.getY(), regs.getSP(), regs.getDirectPageRegister(), regs.getDataBankRegister(), flags.c_str(), regs.P.getByte(), regs.P.getEmulation() ? "true" : "false");
	printf("%02x%04x A:%04x X:%04x Y:%04x S:%04x D:%04x DB:%02x %s \n", regs.getProgramBankRegister(), regs.PC, regs.getAccumulator(), regs.getX(), regs.getY(), regs.getSP(), regs.getDirectPageRegister(), regs.getDataBankRegister(), flags.c_str());
	
	switch (readFromMem(regs.PC, regs.getProgramBankRegister())) {

	case 0x18:	return CLC(); break;

	case 0x2c:	return BIT(ADDR_getAbsolute, 3, 4, false);

	case 0x4b:	return PHK(); break;

	case 0x5b:	return TCD(); break;
	case 0x5c:	return JML(ADDR_getLong); break;

	case 0x78:	return SEI(); break;

	case 0x88:	return DEY(); break;
	//case 0x89:	return (regs.P.getAccuMemSize()) ? BIT(ADDR_getImmediate, 2, 2) : BIT(ADDR_getImmediate, 3, 3); break;

	case 0x8c:	return STY(ADDR_getAbsolute, 3, 4 + regs.P.isXReset()); break;
	case 0x8d:	return STA(ADDR_getAbsolute, 3, 4 + regs.P.isMReset()); break;
	case 0x8e:	return STX(ADDR_getAbsolute, 3, 4 + regs.P.isXReset()); break;

	case 0x9a:	return TXS(); break;

	case 0x9c:	return STZ(ADDR_getAbsolute, 3, 4 + regs.P.isMReset()); break;

	case 0xa0:	return LDY(ADDR_getImmediate, 2 + regs.P.isXReset(), 2 + regs.P.isXReset()); break;

	case 0xa2:	return LDX(ADDR_getImmediate, 2 + regs.P.isXReset(), 2 + regs.P.isXReset()); break;

	case 0xa9:	return LDA(ADDR_getImmediate, 2 + regs.P.isMReset(), 2 + regs.P.isMReset()); break;

	case 0xab:	return PLB(); break;

	case 0xc2:	return REP(); break;

	case 0xca:	return DEX(); break;

	case 0xd0:	return BNE(ADDR_getImmediate, 2 + (regs.P.getEmulation())); break;

	case 0xe2:	return SEP(); break;

	case 0xea:	return NOP(); break;
			
	case 0xfb:	return XCE(); break;

		default:
			printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", readFromMem(regs.PC), regs.PC);
			std::exit(1);
			break;
	}
	
	return 0;
}