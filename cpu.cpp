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
	writeToMem(val, regs.getSP() + 1);
}
void pushToStack(u16 val) {
	regs.setSP(regs.getSP() - 2);
	writeToMem(val >> 8, regs.getSP() + 2);
	writeToMem(val & 0xff, regs.getSP() + 1);
}
u8 pullFromStack() {
	u8 val = readFromMem(regs.getSP() + 1);
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
//	convert hex to decimal
u8 getBCDtoDecimal_8(u8 in) {
	u8 ones = in % 0x10;
	u8 tens = (in / 0x10) * 10;
	return tens + ones;
}
u16 getBCDtoDecimal_16(u16 in) {
	u16 ones = in % 0x10;
	u16 tens = (in / 0x10) * 10;
	u16 hunds = (in / 0x100) * 100;
	u16 grand = (in / 0x1000) * 1000;
	return grand + hunds + tens + ones;
}
u8 getDecimalToBCD_8(u8 in) {
	in %= 100;
	u8 ones = in % 10;
	u8 tens = (in / 10) * 0x10;
	return tens + ones;
}
u16 getDecimalToBCD_16(u16 in) {
	in %= 10000;
	u16 ones = in % 10;
	u16 tens = (in / 10) * 0x10;
	u16 hunds = (in / 100) * 0x100;
	u16 grand = (in / 1000) * 0x1000;
	return grand + hunds + tens + ones;
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

//	push processor status register to stack
u8 PHP() {
	pushToStack(regs.P.getByte());
	regs.PC += 1;
	return 3;
}

//	push accumulator to stack
u8 PHA(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		pushToStack((u8)(regs.getAccumulator() & 0xff));
	}
	else {
		pushToStack(regs.getAccumulator());
	}
	regs.PC++;
	return cycles;
}

//	push X-register to stack
u8 PHX(u8 cycles) {
	if (regs.P.getIndexSize()) {
		pushToStack((u8)(regs.getX() & 0xff));
	}
	else {
		pushToStack(regs.getX());
	}
	regs.PC++;
	return cycles;
}

//	push Y-register to stack
u8 PHY(u8 cycles) {
	if (regs.P.getIndexSize()) {
		pushToStack((u8)(regs.getY() & 0xff));
	}
	else {
		pushToStack(regs.getY());
	}
	regs.PC++;
	return cycles;
}

//	pull accumulator from stack
u8 PLA(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		regs.setAccumulator(pullFromStack());
		regs.P.setNegative((regs.getAccumulator() & 0x80) == 0x80);
		regs.P.setZero(regs.getAccumulator() == 0x00);
	}
	else {
		u8 lo = pullFromStack();
		u8 hi = pullFromStack();
		u16 val = (hi << 8) | lo;
		regs.setAccumulator(val);
		regs.P.setNegative((regs.getAccumulator() & 0x80) == 0x80);
		regs.P.setZero(regs.getAccumulator() == 0x00);
	}
	regs.PC++;
	return cycles;
}

//	pull data bank register from stack
u8 PLB() {
	regs.setDataBankRegister(pullFromStack());
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
	u8 C = regs.P.getCarry() & ~(v & 1);

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
	regs.P.setCarry(C);

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
	u8 C = regs.P.getCarry() | (v & 1);

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
	regs.P.setCarry(C);

	regs.PC += 2;
	return 3;
}

//	Set Carry flag
u8 SEC(u8 cycles) {
	regs.P.setCarry(1);
	regs.PC++;
	return cycles;
}

//	Clear Overflow Flag
u8 CLV() {
	regs.P.setOverflow(0);
	regs.PC++;
	return 2;
}

//	Clear Decimal Flag
u8 CLD() {
	regs.P.setDecimal(0);
	regs.PC++;
	return 2;
}

//	Clear Interrupt Flag
u8 CLI() {
	regs.P.setIRQDisable(0);
	regs.PC++;
	return 2;
}

//	Clear Carry Flag
u8 CLC() {
	regs.P.setCarry(0);
	regs.PC++;
	return 2;
}



//	Add with carry
u8 ADC(u32(*f)(), u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(f());
		u32 res = 0;
		if (regs.P.getDecimal()) {
			res = (regs.getAccumulator() & 0xf) + (val & 0x0f) + regs.P.getCarry();
			if (res > 0x09) {
				res += 0x06;
			}
			regs.P.setCarry(res > 0x0f);
			res = (regs.getAccumulator() & 0xf0) + (val & 0xf0) + (regs.P.getCarry() << 4) + (res & 0x0f);
		}
		else {
			res = (regs.getAccumulator() & 0xff) + val + regs.P.getCarry();
		}
		regs.P.setOverflow((~(regs.getAccumulator() ^ val) & (regs.getAccumulator() ^ res) & 0x80) == 0x80);
		if (regs.P.getDecimal() && res > 0x9f) {
			res += 0x60;
		}
		regs.P.setCarry(res > 0xff);
		regs.P.setZero((u8)res == 0);
		regs.P.setNegative((res & 0x80) == 0x80);
		regs.setAccumulator((u8)(res & 0xff));
	}
	else {
		u16 adr = f();
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		u32 res = 0;
		if (regs.P.getDecimal()) {
			res = (regs.getAccumulator() & 0x000f) + (val & 0x000f) + regs.P.getCarry();
			if (res > 0x0009) {
				res += 0x0006;
			}
			regs.P.setCarry(res > 0x000f);
			res = (regs.getAccumulator() & 0x00f0) + (val & 0x00f0) + (regs.P.getCarry() << 4) + (res & 0x000f);
			if (res > 0x009f) {
				res += 0x0060;
			}
			regs.P.setCarry(res > 0x00ff);
			res = (regs.getAccumulator() & 0x0f00) + (val & 0x0f00) + (regs.P.getCarry() << 8) + (res & 0x00ff);
			if (res > 0x09ff) {
				res += 0x0600;
			}
			regs.P.setCarry(res > 0x0fff);
			res = (regs.getAccumulator() & 0xf000) + (val & 0xf000) + (regs.P.getCarry() << 12) + (res & 0x0fff);
		}
		else {
			res = regs.getAccumulator() + val + regs.P.getCarry();
		}
		regs.P.setOverflow((~(regs.getAccumulator() ^ val) & (regs.getAccumulator() ^ res) & 0x8000) == 0x8000);
		if (regs.P.getDecimal() && res > 0x9fff) {
			res += 0x6000;
		}
		regs.P.setCarry(res > 0xffff);
		regs.P.setZero((u16)(res) == 0);
		regs.P.setNegative((res & 0x8000) == 0x8000);
		regs.setAccumulator((u16)res);
	}
	regs.PC++;
	return cycles;
}




//	And Accumulator with memory
u8 AND(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		regs.setAccumulator((u8)((regs.getAccumulator() & 0xff) & val));
		regs.P.setNegative((regs.getAccumulator() & 0xff) >> 7);
		regs.P.setZero((regs.getAccumulator() & 0xff) == 0);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.setAccumulator((u16)(regs.getAccumulator() & val));
		regs.P.setNegative(regs.getAccumulator() >> 15);
		regs.P.setZero(regs.getAccumulator() == 0);
	}
	regs.PC++;
	return cycles;
}

//	Compare X-Register with memory
u8 CPX(u32(*f)(), u8 cycles) {
	if (regs.P.getIndexSize()) {
		u32 adr = f();
		u8 m = readFromMem(adr);
		u8 val = regs.getX() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getX() >= m);
	}
	else {
		u32 adr = f();
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 m = (hi << 8) | lo;
		u16 val = regs.getX() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getX() >= m);
	}
	regs.PC++;
	return cycles;
}

//	Compare Accumulator with memory
u8 CMP(u32(*f)(), u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u32 adr = f();
		u8 m = readFromMem(adr);
		u8 val = regs.getAccumulator() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry((regs.getAccumulator() & 0xff) >= m);
	}
	else {
		u32 adr = f();
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 m = (hi << 8) | lo;
		u16 val = regs.getAccumulator() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getAccumulator() >= m);
	}
	regs.PC++;
	return cycles;
}

//	OR Accumulator with memory
u8 ORA(u32(*f)(), u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u32 adr = f();
		u8 val = readFromMem(adr);
		u8 res = val | (regs.getAccumulator() & 0xff);
		regs.setAccumulator(res);
		regs.P.setNegative(res >> 7);
		regs.P.setZero(res == 0);
	}
	else {
		u32 adr = f();
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		u16 res = val | regs.getAccumulator();
		regs.setAccumulator(res);
		regs.P.setNegative(res >> 15);
		regs.P.setZero(res == 0);
	}
	regs.PC++;
	return cycles;
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

//	Increment
u8 INC(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		val += 1;
		writeToMem(val, adr);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		val += 1;
		writeToMem(val, adr);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return cycles;
}

//	Increment X
u8 INX() {
	regs.setX((u16)(regs.getX() + 1));
	regs.P.setZero(regs.getX() == 0);
	regs.P.setNegative(regs.getX() >> (7 + ((1 - regs.P.getIndexSize()) * 8)));
	regs.PC++;
	return 2;
}

//	Increment Y
u8 INY() {
	regs.setY((u16)(regs.getY() + 1));
	regs.P.setZero(regs.getY() == 0);
	regs.P.setNegative(regs.getY() >> (7 + ((1 - regs.P.getIndexSize()) * 8)));
	regs.PC++;
	return 2;
}

//	Arithmetic Shift Left
u8 ASL_A(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = regs.getAccumulator() & 0xff;
		regs.P.setCarry(val >> 7);
		regs.setAccumulator((u8)((val + val) & 0xff));
		regs.P.setZero((regs.getAccumulator() & 0xff) == 0);
		regs.P.setNegative((regs.getAccumulator() & 0xff) >> 7);
	}
	else {
		u16 val = regs.getAccumulator();
		regs.P.setCarry(val >> 15);
		regs.setAccumulator((u16)(val + val));
		regs.P.setZero(regs.getAccumulator() == 0);
		regs.P.setNegative(regs.getAccumulator() >> 15);
	}
	regs.PC++;
	return cycles;
}
u8 ASL(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		regs.P.setCarry(val >> 7);
		writeToMem(val + val, adr);
		regs.P.setZero((u8)(val + val) == 0);
		regs.P.setNegative((val + val) >> 7);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setCarry(val >> 15);
		writeToMem((val + val) & 0xff, adr);
		writeToMem((val + val) >> 8, adr + 1);
		regs.P.setZero((u16)(val + val) == 0);
		regs.P.setNegative((u16)(val + val) >> 15);
	}
	regs.PC++;
	return cycles;
}

//	Logical shit right
u8 LSR_A() {
	if (regs.P.getAccuMemSize()) {
		regs.P.setNegative(0);
		regs.P.setCarry(regs.getAccumulator() & 1);
		regs.setAccumulator((u8)((regs.getAccumulator() & 0xff) >> 1));
		regs.P.setZero((regs.getAccumulator() & 0xff) == 0);
	}
	else {
		regs.P.setNegative(0);
		regs.P.setCarry(regs.getAccumulator() & 1);
		regs.setAccumulator((u16)((regs.getAccumulator() >> 1) & 0xffff));
		regs.P.setZero(regs.getAccumulator() == 0);
	}
	regs.PC++;
	return 2;
}

//	Rotate Left
u8 ROL(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val >> 7);
		val = val + val + old_C;
		writeToMem(val, adr);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = (readFromMem(adr + 1) << 8) | readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val >> 15);
		val = val + val + old_C;
		writeToMem(val, adr);
		writeToMem(val >> 8, adr + 1);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}
//	Rotate Left (Accumulator)
u8 ROL_A(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = regs.getAccumulator();
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val >> 7);
		val = val + val + old_C;
		regs.setAccumulator(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = regs.getAccumulator();
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val >> 15);
		val = val + val + old_C;
		regs.setAccumulator(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}

//	Rotate Right
u8 ROR(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val & 1);
		val = (old_C << 7) | (val >> 1);
		writeToMem(val, adr);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = (readFromMem(adr + 1) << 8) | readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val & 1);
		val = (old_C << 15) | (val >> 1);
		writeToMem(val, adr);
		writeToMem(val >> 8, adr + 1);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}
//	Rotate Right (Accumulator)
u8 ROR_A(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = regs.getAccumulator();
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val & 1);
		val = (old_C << 7) | (val >> 1);
		regs.setAccumulator(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = regs.getAccumulator();
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val & 1);
		val = (old_C << 15) | (val >> 1);
		regs.setAccumulator(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}


//	Test memory bits against Accumulator
u8 BIT(u32(*f)(), u8 cycles, bool is_immediate) {
	
	if (regs.P.getAccuMemSize()) {
		u32 adr = f();
		u8 val = readFromMem(adr);
		if (!is_immediate) {
			regs.P.setNegative(val >> 7);
			regs.P.setOverflow((val >> 6) & 1);
		}
		regs.P.setZero((val & regs.getAccumulator()) == 0x00);
	} 
	else {
		u32 adr = f();
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		if (!is_immediate) {
			regs.P.setNegative(val >> 15);
			regs.P.setOverflow((val >> 14) & 1);
		}
		regs.P.setZero((val & regs.getAccumulator()) == 0x0000);
	}
	regs.PC++;
	return cycles;
}

//	Test and Reset memory bits against accumulator
u8 TRB(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		regs.P.setZero((val & (regs.getAccumulator() & 0xff)) == 0);
		u8 res = val & ~(regs.getAccumulator() & 0xff);
		writeToMem(res, adr);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setZero((val & regs.getAccumulator()) == 0);
		u16 res = val & ~regs.getAccumulator();
		writeToMem(res & 0xff, adr);
		writeToMem(res >> 8, adr + 1);
	}
	regs.PC++;
	return cycles;
}

//	Test and Set Memory Bits agains accumulator
u8 TSB(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = readFromMem(adr);
		regs.P.setZero((val & (regs.getAccumulator() & 0xff)) == 0);
		u8 res = val | (regs.getAccumulator() & 0xff);
		writeToMem(res, adr);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setZero((val & regs.getAccumulator()) == 0);
		u16 res = val | regs.getAccumulator();
		writeToMem(res & 0xff, adr);
		writeToMem(res >> 8, adr + 1);
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
u8 LDA(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 lo = readFromMem(adr);
		regs.setAccumulator(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.setAccumulator(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}

//	Load X-Register from memory
u8 LDX(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getIndexSize()) {
		u8 lo = readFromMem(adr);
		regs.setX(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.setX(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return 2;
}

//	Load Y-Register from memory
u8 LDY(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getIndexSize()) {
		u8 lo = readFromMem(adr);
		regs.setY(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else {
		u8 lo = readFromMem(adr);
		u8 hi = readFromMem(adr+1);
		u16 val = (hi << 8) | lo;
		regs.setY(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}


//		Stores

//	Store accumulator to memory
u8 STA(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	writeToMem(regs.getAccumulator(), adr);
	if (regs.P.isMReset())
		writeToMem(regs.getAccumulator() >> 8, adr + 1);
	regs.PC++;
	return cycles;
}
//	Store zero to memory
u8 STZ(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	writeToMem(0x00, adr);
	if (regs.P.isMReset())
		writeToMem(0x00, adr + 1);
	regs.PC++;
	return cycles;
}
//	Store X-Register to memory
u8 STX(u32(*f)(), u8 cycles) {
	u32 adr = f();
	writeToMem(regs.getX(), adr);
	if (regs.P.isXReset())
		writeToMem(regs.getX() >> 8, adr + 1);
	regs.PC++;
	return cycles;
}
//	Store Y-Register to memory
u8 STY(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	writeToMem(regs.getY(), adr);
	if(regs.P.isXReset())
		writeToMem(regs.getY() >> 8, adr + 1);
	regs.PC++;
	return cycles;
}



//		Branches

//	Branch if not equal (Z = 0)
u8 BNE(u32 (*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getZero() == 0) {
		regs.PC += offset;
		branch_taken = 1;
	} 
	return cycles + branch_taken;
}

//	Branch if plus
u8 BPL(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getNegative() == 0) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch if minus
u8 BMI(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getNegative() == 1) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch if carry clear
u8 BCC(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getCarry() == 0) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch if carry set
u8 BCS(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getCarry() == 1) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch if overflow clear
u8 BVC(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getOverflow() == 0) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch if overflow set
u8 BVS(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getOverflow() == 1) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch always
u8 BRA(u32(*f)(), u8 cycles) {
	i8 offset = readFromMem(f());
	regs.PC++;
	regs.PC += offset;
	return cycles;
}

//	Branch always long
u8 BRL(u32(*f)(), u8 cycles) {
	u32 adr = f();
	i8 offset = (readFromMem(adr + 1) << 8) | readFromMem(adr);
	regs.PC += 2;
	regs.PC += offset;
	return cycles;
}

//	Branch if equal (Z = 1)
u8 BEQ(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = readFromMem(f());
	regs.PC++;
	if (regs.P.getZero() == 1) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}



//		Jumps

//	Jump Long
u8 JML(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	u8 bnk = (adr >> 16) & 0xff;
	regs.PC = adr;
	regs.setProgramBankRegister(bnk);
	return cycles;
}

//	Jump
u8 JMP(u32(*f)(), u8 cycles) {
	regs.PC = f();
	return cycles;
}

//	Jump to subroutine
u8 JSR(u32(*f)(), u8 cycles) {
	u32 adr = f();
	pushToStack((u8)(regs.PC >> 8));
	pushToStack((u8)(regs.PC & 0xff));
	regs.PC = adr;
	return cycles;
}

//	Jump to subroutine
u8 JSL(u32(*f)(), u8 cycles) {
	u32 adr = f();
	pushToStack(regs.getProgramBankRegister());
	pushToStack((u8)(regs.PC >> 8));
	pushToStack((u8)(regs.PC & 0xff));
	regs.PC = adr;
	return cycles;
}

//	Return from subroutine
u8 RTS(u8 cycles) {
	u8 lo = pullFromStack();
	u8 hi = pullFromStack();
	regs.PC = (hi << 8) | lo;
	regs.PC++;
	return cycles;
}

//	Return from subroutine (long)
u8 RTL(u8 cycles) {
	u8 lo = pullFromStack();
	u8 hi = pullFromStack();
	u8 bnk = pullFromStack();
	regs.PC = (bnk << 16) | (hi << 8) | lo;
	regs.PC++;
	return cycles;
}




//		Addressing modes
bool pbr = false;
u32 ADDR_getImmediate_8() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	return (dbr << 16) | regs.PC;
}
u32 ADDR_getImmediate_16() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u32 adr = (dbr << 16) | regs.PC - 1;
	return adr;
}
u32 ADDR_getAbsolute() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u16 adr = ((readFromMem(regs.PC) << 8) | readFromMem(regs.PC-1));
	return (dbr << 16) | adr;
}
u32 ADDR_getAbsoluteLong() {
	regs.PC += 3;
	u16 adr = ((readFromMem(regs.PC) << 16) | (readFromMem(regs.PC - 1) << 8) | readFromMem(regs.PC - 2));
	return adr;
}
u32 ADDR_getAbsoluteIndexedX() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u16 adr = ((readFromMem(regs.PC) << 8) | readFromMem(regs.PC - 1));
	pbr = (adr & 0xff00) != ((adr + regs.getX()) & 0xff00);
	adr = adr + regs.getX();
	return (dbr << 16) | adr;
}
u32 ADDR_getAbsoluteIndexedY() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u16 adr = ((readFromMem(regs.PC) << 8) | readFromMem(regs.PC - 1));
	pbr = (adr & 0xff00) != ((adr + regs.getY()) & 0xff00);
	adr = adr + regs.getY();
	return (dbr << 16) | adr;
}
u32 ADDR_getAbsoluteLongIndexedX() {
	regs.PC += 3;
	u8 dbr = regs.getDataBankRegister();
	return (dbr << 16) | (readFromMem(regs.PC - 1) << 8) | readFromMem(regs.PC - 2) + regs.getX();
}
u32 ADDR_getAbsoluteIndirect() {
	regs.PC += 2;
	u8 lo = readFromMem(regs.PC-1);
	u8 hi = readFromMem(regs.PC);
	u16 adr = (hi << 8) | lo;
	u8 i_lo = readFromMem(adr);
	u8 i_hi = readFromMem(adr + 1);
	return (i_hi << 8) | i_lo;
}
u32 ADDR_getAbsoluteIndirectLong() {
	regs.PC += 2;
	u8 lo = readFromMem(regs.PC - 1);
	u8 hi = readFromMem(regs.PC);
	u16 adr = (hi << 8) | lo;
	u8 i_lo = readFromMem(adr);
	u8 i_hi = readFromMem(adr + 1);
	u8 i_bnk = readFromMem(adr + 2);
	return (i_bnk << 16) | (i_hi << 8) | i_lo;
}
u32 ADDR_getAbsoluteIndexedIndirectX() {
	regs.PC += 2;
	u8 lo = readFromMem(regs.PC - 1);
	u8 hi = readFromMem(regs.PC);
	u16 adr = (hi << 8) | lo + regs.getX();
	u8 i_lo = readFromMem(adr);
	u8 i_hi = readFromMem(adr + 1);
	return (i_hi << 8) | i_lo;
}
u32 ADDR_getLong() {
	regs.PC += 3;
	u8 dbr = regs.getDataBankRegister();
	return (dbr << 16) | (readFromMem(regs.PC - 1) << 8) | readFromMem(regs.PC - 2);
}
u32 ADDR_getDirectPage() {
	regs.PC++;
	return regs.getDirectPageRegister() | readFromMem(regs.PC);
}
u32 ADDR_getDirectPageIndexedX() {
	regs.PC++;
	return regs.getDirectPageRegister() | readFromMem(regs.PC) + regs.getX();
}
u32 ADDR_getDirectPageIndirect() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = readFromMem(regs.PC + regs.getDirectPageRegister());
	u16 dp_adr = (readFromMem(dp_index + 1) << 8) | readFromMem(dp_index);
	return (dbr << 16) | dp_adr;
}
u32 ADDR_getDirectPageIndirectLong() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = readFromMem(regs.PC + regs.getDirectPageRegister());
	u32 dp_adr = (readFromMem(dp_index + 2) << 16) | (readFromMem(dp_index + 1) << 8) | readFromMem(dp_index);
	return dp_adr;
}
u32 ADDR_getDirectPageIndirectX() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = readFromMem(regs.PC + regs.getDirectPageRegister()) + regs.getX();
	u16 dp_adr = (readFromMem(dp_index + 1) << 8) | readFromMem(dp_index);
	return (dbr << 16) | dp_adr;
}
u32 ADDR_getDirectPageIndirectIndexedY() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = readFromMem(regs.PC + regs.getDirectPageRegister());
	u16 dp_adr = (regs.getDataBankRegister() << 16) | (readFromMem(dp_index + 1) << 8) | readFromMem(dp_index);
	pbr = (dp_adr & 0xff00) != ((dp_adr + regs.getY()) & 0xff00);
	dp_adr += regs.getY();
	return dp_adr;
}
u32 ADDR_getDirectPageIndirectLongIndexedY() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = readFromMem(regs.PC + regs.getDirectPageRegister());
	u16 dp_adr = (readFromMem(dp_index + 2) << 16) | (readFromMem(dp_index + 1) << 8) | readFromMem(dp_index);
	pbr = (dp_adr & 0xff00) != ((dp_adr + regs.getY()) & 0xff00);
	dp_adr += regs.getY();
	return dp_adr;
}
u32 ADDR_getStackRelative() {
	regs.PC++;
	u8 byte = readFromMem(regs.PC);
	return regs.getSP() + byte;
}
u32 ADDR_getStackRelativeIndirectIndexedY() {
	regs.PC++;
	u8 byte = readFromMem(regs.PC);
	u8 base = readFromMem((regs.getDataBankRegister() << 16) | regs.getSP() + byte);
	return base + regs.getY();
}
bool pageBoundaryCrossed() {
	bool v = pbr;
	pbr = false;
	return v;
}


u8 stepCPU() {
	string flags = byteToBinaryString(regs.P.getByte());
	//printf("Op: %02x %02x %02x %02x  PC : 0x%04x A: 0x%04x X: 0x%04x Y: 0x%04x SP: 0x%04x D: 0x%04x DB: 0x%02x P: %s (0x%02x) Emu: %s\n", readFromMem(regs.PC, regs.getDataBankRegister()), readFromMem(regs.PC+1, regs.getDataBankRegister()), readFromMem(regs.PC+2, regs.getDataBankRegister()), readFromMem(regs.PC + 3, regs.getDataBankRegister()), regs.PC, regs.getAccumulator(), regs.getX(), regs.getY(), regs.getSP(), regs.getDirectPageRegister(), regs.getDataBankRegister(), flags.c_str(), regs.P.getByte(), regs.P.getEmulation() ? "true" : "false");
	printf("%02x%04x A:%04x X:%04x Y:%04x S:%04x D:%04x DB:%02x %s \n", regs.getProgramBankRegister(), regs.PC, regs.getAccumulator(), regs.getX(), regs.getY(), regs.getSP(), regs.getDirectPageRegister(), regs.getDataBankRegister(), flags.c_str());
	
	switch (readFromMem((regs.getProgramBankRegister() << 16) | regs.PC)) {

	case 0x01:	return ORA(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;

	case 0x03:	return ORA(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
	case 0x04:	return TSB(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
	case 0x05:	return ORA(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x06:	return ASL(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
	case 0x07:	return ORA(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x08:	return PHP(); break;
	case 0x09:	return (regs.P.getAccuMemSize()) ? ORA(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : ORA(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
	case 0x0a:	return ASL_A(2); break;

	case 0x0c:	return TSB(ADDR_getAbsolute, 6 + (2 * regs.P.isMReset())); break;
	case 0x0d:	return ORA(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
	case 0x0e:	return ASL(ADDR_getAbsolute, 6 + regs.P.isMReset()); break;
	case 0x0f:	return ORA(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
	case 0x10:	return (regs.P.getAccuMemSize()) ? BPL(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BPL(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;
	case 0x11:	return ORA(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
	case 0x12:	return ORA(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x13:	return ORA(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
	case 0x14:	return TRB(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
	case 0x15:	return ORA(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x16:	return ASL(ADDR_getDirectPageIndexedX, 6 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
	case 0x17:	return ORA(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x18:	return CLC(); break;
	case 0x19:	return ORA(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;

	case 0x1c:	return TRB(ADDR_getAbsolute, 6 + (2 * regs.P.isMReset())); break;
	case 0x1d:	return ORA(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
	case 0x1e:	return ASL(ADDR_getAbsoluteIndexedX, 7 + (2 * regs.P.isMReset()) + pageBoundaryCrossed()); break;
	case 0x1f:	return ORA(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
	case 0x20:	return JSR(ADDR_getAbsolute, 6); break;
	case 0x21:	return AND(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x22:	return JSL(ADDR_getAbsoluteLong, 8); break;
	case 0x23:	return AND(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
	case 0x24:	return BIT(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero(), false); break;
	case 0x25:	return AND(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x26:	return ROL(ADDR_getDirectPage, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x27:	return AND(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;

	case 0x29:	return (regs.P.getAccuMemSize()) ? AND(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : AND(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
	case 0x2a:	return ROL_A(2); break;

	case 0x2c:	return BIT(ADDR_getAbsolute, 4, false); break;
	case 0x2d:	return AND(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
	case 0x2e:	return ROL(ADDR_getAbsolute, 6 + regs.P.isMReset()); break;
	case 0x2f:	return AND(ADDR_getLong, 5 + regs.P.isMReset()); break;
	case 0x30:	return (regs.P.getAccuMemSize()) ? BMI(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BMI(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;
	case 0x31:	return AND(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
	case 0x32:	return AND(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x33:	return AND(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
	case 0x34:	return BIT(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero(), false); break;
	case 0x35:	return AND(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x36:	return ROL(ADDR_getDirectPageIndexedX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x37:	return AND(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x38:	return SEC(2); break;
	case 0x39:	return AND(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;

	case 0x3c:	return BIT(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed(), false); break;
	case 0x3d:	return AND(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x3e:	return ROL(ADDR_getAbsoluteIndexedX, 7 + regs.P.isMReset() + pageBoundaryCrossed()); break;
	case 0x3f:	return AND(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;

	case 0x48:	return PHA(3 + regs.P.isMReset()); break;

	case 0x4a:	return LSR_A(); break;
	case 0x4b:	return PHK(); break;
	case 0x4c:	return JMP(ADDR_getAbsolute, 3); break;

	case 0x50:	return (regs.P.getAccuMemSize()) ? BVC(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BVC(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;

	case 0x58:	return CLI(); break;

	case 0x5a:	return PHY(3 + regs.P.getIndexSize()); break;
	case 0x5b:	return TCD(); break;
	case 0x5c:	return JML(ADDR_getLong, 4); break;

	case 0x60:	return RTS(6); break;
	case 0x61:	return ADC(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;

	case 0x63:	return ADC(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;

	case 0x65:	return ADC(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x66:	return ROR(ADDR_getDirectPage, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x67:	return ADC(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x68:	return PLA(4 + regs.P.isMReset()); break;
	case 0x69:	return (regs.P.getAccuMemSize()) ? ADC(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : ADC(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
	case 0x6a:	return ROR_A(2); break;
	case 0x6b:	return RTL(6); break;
	case 0x6c:	return JMP(ADDR_getAbsoluteIndirect, 5); break;
	case 0x6d:	return ADC(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
	case 0x6e:	return ROR(ADDR_getAbsolute, 6 + regs.P.isMReset()); break;
	case 0x6f:	return ADC(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
	case 0x70:	return (regs.P.getAccuMemSize()) ? BVS(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BVS(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;
	case 0x71:	return ADC(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
	case 0x72:	return ADC(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x73:	return ADC(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;

	case 0x75:	return ADC(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x76:	return ROR(ADDR_getDirectPageIndexedX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x77:	return ADC(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x78:	return SEI(); break;
	case 0x79:	return ADC(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;

	case 0x7c:	return JMP(ADDR_getAbsoluteIndexedIndirectX, 5); break;
	case 0x7d:	return ADC(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
	case 0x7e:	return ROR(ADDR_getAbsoluteIndexedX, 7 + regs.P.isMReset() + pageBoundaryCrossed()); break;
	case 0x7f:	return ADC(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;

	case 0x80:	return (regs.P.getAccuMemSize()) ? BRA(ADDR_getImmediate_8, 3 + regs.P.getEmulation()) : BRA(ADDR_getImmediate_16, 3 + regs.P.getEmulation()); break;

	case 0x82:	return (regs.P.getAccuMemSize()) ? BRL(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BRL(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;

	case 0x85:	return STA(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0x86:	return STX(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;

	case 0x88:	return DEY(); break;
	case 0x89:	return (regs.P.getAccuMemSize()) ? BIT(ADDR_getImmediate_8, 2 + regs.P.isMReset(), true) : BIT(ADDR_getImmediate_16, 2 + regs.P.isMReset(), true); break;

	case 0x8c:	return STY(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
	case 0x8d:	return STA(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
	case 0x8e:	return STX(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;

	case 0x90:	return (regs.P.getAccuMemSize()) ? BCC(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BCC(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;

	case 0x9a:	return TXS(); break;

	case 0x9c:	return STZ(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;

	case 0xa0:	return (regs.P.getIndexSize()) ? LDY(ADDR_getImmediate_8, 2 + regs.P.isXReset()) : LDY(ADDR_getImmediate_16, 2 + regs.P.isXReset()); break;

	case 0xa2:	return (regs.P.getIndexSize()) ? LDX(ADDR_getImmediate_8, 2 + regs.P.isXReset()) : LDX(ADDR_getImmediate_16, 2 + regs.P.isXReset()); break;

	case 0xa5:	return LDA(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
	case 0xa6:	return LDX(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;

	case 0xa9:	return (regs.P.getAccuMemSize()) ? LDA(ADDR_getImmediate_8, 2 + regs.P.isXReset()) : LDA(ADDR_getImmediate_16, 2 + regs.P.isXReset()); break;

	case 0xab:	return PLB(); break;

	case 0xb0:	return (regs.P.getAccuMemSize()) ? BCS(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BCS(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;

	case 0xb8:	return CLV(); break;

	case 0xbd:	return LDA(ADDR_getAbsoluteIndexedX, 4 + pageBoundaryCrossed() + regs.P.isMReset()); break;

	case 0xc2:	return REP(); break;

	case 0xc8:	return INY(); break;
	case 0xc9:	return (regs.P.getAccuMemSize()) ? CMP(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : CMP(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;

	case 0xca:	return DEX(); break;

	case 0xcd:	return CMP(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;

	case 0xd0:	return (regs.P.getAccuMemSize()) ? BNE(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BNE(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;

	case 0xd8:	return CLD(); break;

	case 0xda:	return PHX(3 + regs.P.getIndexSize()); break;

	case 0xdc:	return JMP(ADDR_getAbsoluteIndirectLong, 6); break;

	case 0xe0:	return (regs.P.getIndexSize()) ? CPX(ADDR_getImmediate_8, 2 + regs.P.getIndexSize()) : CPX(ADDR_getImmediate_16, 2 + regs.P.getIndexSize()); break;

	case 0xe2:	return SEP(); break;

	case 0xe6:	return INC(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;

	case 0xe8:	return INX(); break;

	case 0xea:	return NOP(); break;

	case 0xec:	return CPX(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;

	case 0xf0:	return (regs.P.getAccuMemSize()) ? BEQ(ADDR_getImmediate_8, 2 + regs.P.getEmulation()) : BEQ(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;
			
	case 0xfb:	return XCE(); break;
	case 0xfc:	return JSR(ADDR_getAbsoluteIndexedIndirectX, 8); break;

		default:
			printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", readFromMem(regs.PC), regs.PC);
			std::exit(1);
			break;
	}
	
	return 0;
}