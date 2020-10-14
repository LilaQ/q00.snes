#include "cpu.h"
#include "bus.h"
#include <map>

typedef int8_t		i8;
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

Registers regs;
Interrupts interrupts;
bool CPU_STOPPED = false;
u8 Interrupts::interrupt_value = 0;

void resetCPU() {
	CPU_STOPPED = false;
	regs.P.setEmulation(1);
	regs.setDirectPageRegister(0x00);
	regs.setSP((regs.getSP() & 0xff) | 0x0100);
	regs.P.setAccuMemSize(1);
	regs.P.setIndexSize(1, &regs);
	u8 lo = BUS_readFromMem(VECTOR_EMU_RESET);
	u8 hi = BUS_readFromMem(VECTOR_EMU_RESET + 1);
	regs.PC = (hi << 8) | lo;
	regs.resetStackPointer();
	//printf("Reset CPU, starting at PC: %x\n", regs.PC);
}

bool CPU_isStopped() {
	return CPU_STOPPED;
}

//		DEBUG
u16 CPU_getPC() {
	return regs.PC;
}


//		Helper

//	8-bit / 16-bit wide Y-Index templates (getter / setter)
void pushToStack(u8 val) {
	regs.setSP(regs.getSP() - 1);
	BUS_writeToMem(val, regs.getSP() + 1);
}
void pushToStack(u16 val) {
	regs.setSP(regs.getSP() - 2);
	BUS_writeToMem(val >> 8, regs.getSP() + 2);
	BUS_writeToMem(val & 0xff, regs.getSP() + 1);
}
u8 pullFromStack() {
	u8 val = BUS_readFromMem(regs.getSP() + 1);
	regs.setSP(regs.getSP() + 1);
	return val;
}
//	return P as readable string
inline const std::string byteToBinaryString(u8 val) {

	//	alpha representation (bsnes-like)
	std::string s = "";
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

//	exchange Accu-Low and Accu-High
u8 XBA() {
	u16 tmp = regs.getAccumulator();
	u8 lo = tmp & 0xff;
	u8 hi = tmp >> 8;
	regs.setAccumulator((u16)((lo << 8) | hi));
	regs.P.setNegative((regs.getAccumulator() >> 7) & 1);
	regs.P.setZero(hi == 0);
	regs.PC++;
	return 3;
}

//	push program bank register to stack
u8 PHK() {
	pushToStack(regs.getProgramBankRegister());
	regs.PC += 1;
	return 3;
}

//	push data bank register to stack
u8 PHB() {
	pushToStack(regs.getDataBankRegister());
	regs.PC += 1;
	return 3;
}

//	push direct page register to stack
u8 PHD() {
	pushToStack((u8)(regs.getDirectPageRegister() >> 8));
	pushToStack((u8)(regs.getDirectPageRegister() & 0xff));
	regs.PC += 1;
	return 4;
}

//	push processor status register to stack
u8 PHP() {
	pushToStack(regs.P.getByte());
	regs.PC += 1;
	return 3;
}

//	push effective absolute address
u8 PEA(u32(*f)(), u8 cycles) {
	u32 adr = f();
	pushToStack((u8)(adr >> 8));
	pushToStack((u8)(adr & 0xff));
	regs.PC++;
	return cycles;
}

//	push effectice indirect address
u8 PEI(u32(*f)(), u8 cycles) {
	u32 adr = f();
	adr |= regs.getDataBankRegister();
	pushToStack((u8)(adr >> 8));
	pushToStack((u8)(adr & 0xff));
	regs.PC++;
	return cycles;
}

//	push effective PC relative indirect address
u8 PER(u8 cycles) {
	regs.PC += 3;
	u8 lo = BUS_readFromMem(regs.PC - 2);
	u8 hi = BUS_readFromMem(regs.PC - 1);
	u16 adr = (hi << 8) | lo;
	u16 res = regs.PC + adr;
	pushToStack((u8)(res >> 8));
	pushToStack((u8)(res & 0xff));
	return cycles;
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
		regs.P.setZero((regs.getAccumulator() & 0xff) == 0x00);
	}
	else {
		u8 lo = pullFromStack();
		u8 hi = pullFromStack();
		u16 val = (hi << 8) | lo;
		regs.setAccumulator(val);
		regs.P.setNegative((regs.getAccumulator() & 0x8000) == 0x8000);
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

//	pull direct page register from stack
u8 PLD() {
	u8 lo = pullFromStack();
	u8 hi = pullFromStack();
	regs.setDirectPageRegister((hi << 8) | lo);
	regs.P.setNegative((regs.getDataBankRegister() & 0x8000) == 0x8000);
	regs.P.setZero(regs.getDataBankRegister() == 0x0000);
	regs.PC++;
	return 5;
}

//	pull status from stack
u8 PLP() {
	if (regs.P.getEmulation()) {
		u8 val = pullFromStack();
		regs.P.setByte(val);
		regs.P.setAccuMemSize(1);
		regs.P.setIndexSize(1, &regs);
	}
	else {
		u8 val = pullFromStack();
		regs.P.setByte(val);
	}
	regs.PC++;
	return 4;
}

//	pull X-register from stack
u8 PLX() {
	if (regs.P.getIndexSize()) {
		regs.setX(pullFromStack());
		regs.P.setNegative((regs.getX() & 0x80) == 0x80);
		regs.P.setZero(regs.getX() == 0x00);
	}
	else {
		u8 lo = pullFromStack();
		u8 hi = pullFromStack();
		regs.setX((u16)((hi << 8) | lo));
		regs.P.setNegative((regs.getX() & 0x8000) == 0x8000);
		regs.P.setZero(regs.getX() == 0x0000);
	}
	regs.PC++;
	return 4 + regs.P.isXReset();
}

//	pull Y-register from stack
u8 PLY() {
	if (regs.P.getIndexSize()) {
		regs.setY(pullFromStack());
		regs.P.setNegative((regs.getY() & 0x80) == 0x80);
		regs.P.setZero(regs.getY() == 0x00);
	}
	else {
		u8 lo = pullFromStack();
		u8 hi = pullFromStack();
		regs.setY((u16)((hi << 8) | lo));
		regs.P.setNegative((regs.getY() & 0x8000) == 0x8000);
		regs.P.setZero(regs.getY() == 0x0000);
	}
	regs.PC++;
	return 4 + regs.P.isXReset();
}

//	reset status bits (reset all bits set in the immediate value)
u8 REP() {
	u8 v = BUS_readFromMem(regs.PC+1);
	bool N = ((u8)regs.P.getNegative() & ~((v >> 7) & 1)) > 0;
	bool V = ((u8)regs.P.getOverflow() & ~((v >> 6) & 1)) > 0;
	bool M = ((u8)regs.P.getAccuMemSize() & ~((v >> 5) & 1)) > 0;
	bool X = ((u8)regs.P.getIndexSize() & ~((v >> 4) & 1)) > 0;
	bool B = ((u8)regs.P.getBreak() & ~((v >> 4) & 1)) > 0;
	bool D = ((u8)regs.P.getDecimal() & ~((v >> 3) & 1)) > 0;
	bool I = ((u8)regs.P.getIRQDisable() & ~((v >> 2) & 1)) > 0;
	bool Z = ((u8)regs.P.getZero() & ~((v >> 1) & 1)) > 0;
	bool C = ((u8)regs.P.getCarry() & ~(v & 1)) > 0;

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
	u8 v = BUS_readFromMem(regs.PC + 1);
	u8 N = (regs.P.getNegative() | ((v >> 7) & 1)) > 0;
	u8 V = (regs.P.getOverflow() | ((v >> 6) & 1)) > 0;
	u8 M = (regs.P.getAccuMemSize() | ((v >> 5) & 1)) > 0;
	u8 X = (regs.P.getIndexSize() | ((v >> 4) & 1)) > 0;
	u8 B = (regs.P.getBreak() | ((v >> 4) & 1)) > 0;
	u8 D = (regs.P.getDecimal() | ((v >> 3) & 1)) > 0;
	u8 I = (regs.P.getIRQDisable() | ((v >> 2) & 1)) > 0;
	u8 Z = (regs.P.getZero() | ((v >> 1) & 1)) > 0;
	u8 C = (regs.P.getCarry() | (v & 1)) > 0;

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

//	Set Decimal Flag
u8 SED() {
	regs.P.setDecimal(1);
	regs.PC++;
	return 2;
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
		u8 val = BUS_readFromMem(f());
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
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
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

//	Subtract with borrow
u8 SBC(u32(*f)(), u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = BUS_readFromMem(f());
		val = ~val;
		u32 res = 0;
		if (regs.P.getDecimal()) {
			res = (regs.getAccumulator() & 0xf) + (val & 0x0f) + regs.P.getCarry();
			if (res <= 0x0f) {
				res -= 0x06;
			}
			regs.P.setCarry(res > 0x0f);
			res = (regs.getAccumulator() & 0xf0) + (val & 0xf0) + (regs.P.getCarry() << 4) + (res & 0x0f);
		}
		else {
			res = (regs.getAccumulator() & 0xff) + val + regs.P.getCarry();
		}
		regs.P.setOverflow((~(regs.getAccumulator() ^ val) & (regs.getAccumulator() ^ res) & 0x80) == 0x80);
		if (regs.P.getDecimal() && res <= 0xff) {
			res -= 0x60;
		}
		regs.P.setCarry(res > 0xff);
		regs.P.setZero((u8)res == 0);
		regs.P.setNegative((res & 0x80) == 0x80);
		regs.setAccumulator((u8)(res & 0xff));
	}
	else {
		u16 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		val = ~val;
		u32 res = 0;
		if (regs.P.getDecimal()) {
			res = (regs.getAccumulator() & 0x000f) + (val & 0x000f) + regs.P.getCarry();
			if (res <= 0x000f) {
				res -= 0x0006;
			}
			regs.P.setCarry(res > 0x000f);
			res = (regs.getAccumulator() & 0x00f0) + (val & 0x00f0) + (regs.P.getCarry() << 4) + (res & 0x000f);
			if (res <= 0x00ff) {
				res -= 0x0060;
			}
			regs.P.setCarry(res > 0x00ff);
			res = (regs.getAccumulator() & 0x0f00) + (val & 0x0f00) + (regs.P.getCarry() << 8) + (res & 0x00ff);
			if (res <= 0x0fff) {
				res -= 0x0600;
			}
			regs.P.setCarry(res > 0x0fff);
			res = (regs.getAccumulator() & 0xf000) + (val & 0xf000) + (regs.P.getCarry() << 12) + (res & 0x0fff);
		}
		else {
			res = regs.getAccumulator() + val + regs.P.getCarry();
		}
		regs.P.setOverflow((~(regs.getAccumulator() ^ val) & (regs.getAccumulator() ^ res) & 0x8000) == 0x8000);
		if (regs.P.getDecimal() && res <= 0xffff) {
			res -= 0x6000;
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
		u8 val = BUS_readFromMem(adr);
		regs.setAccumulator((u8)((regs.getAccumulator() & 0xff) & val));
		regs.P.setNegative((regs.getAccumulator() & 0xff) >> 7);
		regs.P.setZero((regs.getAccumulator() & 0xff) == 0);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
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
		u8 m = BUS_readFromMem(adr);
		u8 val = (u8)regs.getX() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getX() >= m);
	}
	else {
		u32 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 m = (hi << 8) | lo;
		u16 val = regs.getX() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getX() >= m);
	}
	regs.PC++;
	return cycles;
}

//	Compare Y-Register with memory
u8 CPY(u32(*f)(), u8 cycles) {
	if (regs.P.getIndexSize()) {
		u32 adr = f();
		u8 m = BUS_readFromMem(adr);
		u8 val = (u8)regs.getY() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getY() >= m);
	}
	else {
		u32 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 m = (hi << 8) | lo;
		u16 val = regs.getY() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry(regs.getY() >= m);
	}
	regs.PC++;
	return cycles;
}

//	Compare Accumulator with memory
u8 CMP(u32(*f)(), u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u32 adr = f();
		u8 m = BUS_readFromMem(adr);
		u8 val = regs.getAccumulator() - m;
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
		regs.P.setCarry((regs.getAccumulator() & 0xff) >= m);
	}
	else {
		u32 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
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
		u8 val = BUS_readFromMem(adr);
		u8 res = val | (regs.getAccumulator() & 0xff);
		regs.setAccumulator(res);
		regs.P.setNegative(res >> 7);
		regs.P.setZero(res == 0);
	}
	else {
		u32 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		u16 res = val | regs.getAccumulator();
		regs.setAccumulator(res);
		regs.P.setNegative(res >> 15);
		regs.P.setZero(res == 0);
	}
	regs.PC++;
	return cycles;
}

//	XOR Accumulator with memory
u8 EOR(u32(*f)(), u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u32 adr = f();
		u8 val = BUS_readFromMem(adr);
		u8 res = val ^ (regs.getAccumulator() & 0xff);
		regs.setAccumulator(res);
		regs.P.setNegative(res >> 7);
		regs.P.setZero(res == 0);
	}
	else {
		u32 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		u16 res = val ^ regs.getAccumulator();
		regs.setAccumulator(res);
		regs.P.setNegative(res >> 15);
		regs.P.setZero(res == 0);
	}
	regs.PC++;
	return cycles;
}

//	Decrement (accumulator)
u8 DEC_A() {
	if (regs.P.getAccuMemSize()) {
		u8 val = (u8)regs.getAccumulator();
		val--;
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getAccumulator();
		val--;
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Decrement
u8 DEC(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = BUS_readFromMem(adr);
		val--;
		BUS_writeToMem(val, adr);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		val--;
		BUS_writeToMem(val & 0xff, adr);
		BUS_writeToMem(val >> 8, adr + 1);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
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

//	Increment (accumulator)
u8 INC_A() {
	if (regs.P.getAccuMemSize()) {
		u8 val = (u8)regs.getAccumulator();
		val++;
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getAccumulator();
		val++;
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Increment
u8 INC(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = BUS_readFromMem(adr);
		val++;
		BUS_writeToMem(val, adr);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		val++;
		BUS_writeToMem(val & 0xff, adr);
		BUS_writeToMem(val >> 8, adr + 1);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return cycles;
}

//	Increment X
u8 INX() {
	if (regs.P.getIndexSize()) {
		regs.setX((u8)((regs.getX() & 0xff) + 1));
		regs.P.setZero((regs.getX() & 0xff) == 0);
		regs.P.setNegative((regs.getX() & 0xff) >> 7);
	}
	else {
		regs.setX((u16)(regs.getX() + 1));
		regs.P.setZero(regs.getX() == 0);
		regs.P.setNegative(regs.getX() >> 15);
	}
	regs.PC++;
	return 2;
}

//	Increment Y
u8 INY() {
	if (regs.P.getIndexSize()) {
		regs.setY((u8)((regs.getY() & 0xff) + 1));
		regs.P.setZero((regs.getY() & 0xff) == 0);
		regs.P.setNegative((regs.getY() & 0xff) >> 7);
	}
	else {
		regs.setY((u16)(regs.getY() + 1));
		regs.P.setZero(regs.getY() == 0);
		regs.P.setNegative(regs.getY() >> 15);
	}
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
		u8 val = BUS_readFromMem(adr);
		regs.P.setCarry(val >> 7);
		BUS_writeToMem(val + val, adr);
		regs.P.setZero((u8)(val + val) == 0);
		regs.P.setNegative((val + val) >> 7);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setCarry(val >> 15);
		BUS_writeToMem((val + val) & 0xff, adr);
		BUS_writeToMem((val + val) >> 8, adr + 1);
		regs.P.setZero((u16)(val + val) == 0);
		regs.P.setNegative((u16)(val + val) >> 15);
	}
	regs.PC++;
	return cycles;
}

//	Logical shift right
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

//	Logical shift right
u8 LSR(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = BUS_readFromMem(adr);
		regs.P.setNegative(0);
		regs.P.setCarry(val & 1);
		BUS_writeToMem((u8)((val & 0xff) >> 1), adr);
		regs.P.setZero((u8)((regs.getAccumulator() & 0xff) >> 1) == 0);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setNegative(0);
		regs.P.setCarry(val & 1);
		BUS_writeToMem(((val & 0xffff) >> 1) & 0xff, adr);
		BUS_writeToMem(((val & 0xffff) >> 1) >> 8, adr + 1);
		regs.P.setZero((u8)((regs.getAccumulator() & 0xffff) >> 1) == 0);
	}
	regs.PC++;
	return cycles;
}

//	Rotate Left
u8 ROL(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = BUS_readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val >> 7);
		val = val + val + old_C;
		BUS_writeToMem(val, adr);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = (BUS_readFromMem(adr + 1) << 8) | BUS_readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val >> 15);
		val = val + val + old_C;
		BUS_writeToMem((u8)val, adr);
		BUS_writeToMem(val >> 8, adr + 1);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}
//	Rotate Left (Accumulator)
u8 ROL_A(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = (u8)regs.getAccumulator();
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
		u8 val = BUS_readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val & 1);
		val = (old_C << 7) | (val >> 1);
		BUS_writeToMem(val, adr);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = (BUS_readFromMem(adr + 1) << 8) | BUS_readFromMem(adr);
		u8 old_C = regs.P.getCarry();
		regs.P.setCarry(val & 1);
		val = (old_C << 15) | (val >> 1);
		BUS_writeToMem((u8)val, adr);
		BUS_writeToMem(val >> 8, adr + 1);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
	regs.PC++;
	return cycles;
}
//	Rotate Right (Accumulator)
u8 ROR_A(u8 cycles) {
	if (regs.P.getAccuMemSize()) {
		u8 val = (u8)regs.getAccumulator();
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
		u8 val = BUS_readFromMem(adr);
		if (!is_immediate) {
			regs.P.setNegative(val >> 7);
			regs.P.setOverflow((val >> 6) & 1);
		}
		regs.P.setZero((val & regs.getAccumulator()) == 0x00);
	} 
	else {
		u32 adr = f();
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
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
		u8 val = BUS_readFromMem(adr);
		regs.P.setZero((val & (regs.getAccumulator() & 0xff)) == 0);
		u8 res = val & ~(regs.getAccumulator() & 0xff);
		BUS_writeToMem(res, adr);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setZero((val & regs.getAccumulator()) == 0);
		u16 res = val & ~regs.getAccumulator();
		BUS_writeToMem(res & 0xff, adr);
		BUS_writeToMem(res >> 8, adr + 1);
	}
	regs.PC++;
	return cycles;
}

//	Test and Set Memory Bits agains accumulator
u8 TSB(u32(*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 val = BUS_readFromMem(adr);
		regs.P.setZero((val & (regs.getAccumulator() & 0xff)) == 0);
		u8 res = val | (regs.getAccumulator() & 0xff);
		BUS_writeToMem(res, adr);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
		u16 val = (hi << 8) | lo;
		regs.P.setZero((val & regs.getAccumulator()) == 0);
		u16 res = val | regs.getAccumulator();
		BUS_writeToMem(res & 0xff, adr);
		BUS_writeToMem(res >> 8, adr + 1);
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

//	Transfer DP to 16 bit A
u8 TDC() {
	regs.setAccumulator(regs.getDirectPageRegister());
	regs.P.setZero(regs.getAccumulator() == 0);
	regs.P.setNegative(regs.getAccumulator() >> 15);
	regs.PC++;
	return 2;
}

//	Transfer 16 bit A to SP
u8 TCS() {
	regs.setSP(regs.getAccumulator());
	regs.PC++;
	return 2;
}

//	Transfer SP to 16 bit A
u8 TSC() {
	regs.setAccumulator(regs.getSP());
	regs.P.setZero(regs.getAccumulator() == 0);
	regs.P.setNegative(regs.getAccumulator() >> 15);
	regs.PC++;
	return 2;
}

//	Transfer SP to X
u8 TSX() {
	if (regs.P.getIndexSize()) {
		u16 val = (regs.getSP() & 0xff);
		regs.setX(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 7);
	}
	else {
		u16 val = regs.getSP();
		regs.setX(val);
		regs.P.setZero(val == 0);
		regs.P.setNegative(val >> 15);
	}
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
	regs.PC++;
	return 2;
}

//	Transfer A to X
u8 TAX() {
	if (regs.P.getIndexSize()) {
		u8 val = (u8)(regs.getAccumulator() & 0xff);
		regs.setX(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getAccumulator();
		regs.setX(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Transfer X to A
u8 TXA() {
	if (regs.P.getAccuMemSize()) {
		u8 val = (u8)(regs.getX() & 0xff);
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getX();
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Transfer A to Y
u8 TAY() {
	if (regs.P.getIndexSize()) {
		u8 val = (u8)(regs.getAccumulator() & 0xff);
		regs.setY(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getAccumulator();
		regs.setY(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Transfer X to Y
u8 TXY() {
	if (regs.P.getIndexSize()) {
		u8 val = (u8)(regs.getX() & 0xff);
		regs.setY(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getX();
		regs.setY(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Transfer Y to A
u8 TYA() {
	if (regs.P.getAccuMemSize()) {
		u8 val = (u8)(regs.getY() & 0xff);
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getY();
		regs.setAccumulator(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}

//	Transfer Y to Y
u8 TYX() {
	if (regs.P.getIndexSize()) {
		u8 val = (u8)(regs.getY() & 0xff);
		regs.setX(val);
		regs.P.setNegative(val >> 7);
		regs.P.setZero(val == 0);
	}
	else {
		u16 val = regs.getY();
		regs.setX(val);
		regs.P.setNegative(val >> 15);
		regs.P.setZero(val == 0);
	}
	regs.PC++;
	return 2;
}


//		Loads

//	Load accumulator from memory
u8 LDA(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	if (regs.P.getAccuMemSize()) {
		u8 lo = BUS_readFromMem(adr);
		regs.setAccumulator(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
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
		u8 lo = BUS_readFromMem(adr);
		regs.setX(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr + 1);
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
		u8 lo = BUS_readFromMem(adr);
		regs.setY(lo);
		regs.P.setZero(lo == 0);
		regs.P.setNegative(lo >> 7);
	}
	else {
		u8 lo = BUS_readFromMem(adr);
		u8 hi = BUS_readFromMem(adr+1);
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
	BUS_writeToMem((u8)regs.getAccumulator(), adr);
	if (regs.P.isMReset())
		BUS_writeToMem(regs.getAccumulator() >> 8, adr + 1);
	regs.PC++;
	return cycles;
}
//	Store zero to memory
u8 STZ(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	BUS_writeToMem(0x00, adr);
	if (regs.P.isMReset())
		BUS_writeToMem(0x00, adr + 1);
	regs.PC++;
	return cycles;
}
//	Store X-Register to memory
u8 STX(u32(*f)(), u8 cycles) {
	u32 adr = f();
	BUS_writeToMem((u8)regs.getX(), adr);
	if (regs.P.isXReset())
		BUS_writeToMem(regs.getX() >> 8, adr + 1);
	regs.PC++;
	return cycles;
}
//	Store Y-Register to memory
u8 STY(u32 (*f)(), u8 cycles) {
	u32 adr = f();
	BUS_writeToMem((u8)regs.getY(), adr);
	if(regs.P.isXReset())
		BUS_writeToMem(regs.getY() >> 8, adr + 1);
	regs.PC++;
	return cycles;
}



//		Branches

//	Branch if not equal (Z = 0)
u8 BNE(u32 (*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = BUS_readFromMem(f());
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
	i8 offset = BUS_readFromMem(f());
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
	i8 offset = BUS_readFromMem(f());
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
	i8 offset = BUS_readFromMem(f());
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
	i8 offset = BUS_readFromMem(f());
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
	i8 offset = BUS_readFromMem(f());
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
	i8 offset = BUS_readFromMem(f());
	regs.PC++;
	if (regs.P.getOverflow() == 1) {
		regs.PC += offset;
		branch_taken = 1;
	}
	return cycles + branch_taken;
}

//	Branch always
u8 BRA(u32(*f)(), u8 cycles) {
	i8 offset = BUS_readFromMem(f());
	regs.PC++;
	regs.PC += offset;
	return cycles;
}

//	Branch always long
u8 BRL(u32(*f)(), u8 cycles) {
	u32 adr = f();
	i8 offset = (BUS_readFromMem(adr + 1) << 8) | BUS_readFromMem(adr);
	regs.PC += 2;
	regs.PC += offset;
	return cycles;
}

//	Branch if equal (Z = 1)
u8 BEQ(u32(*f)(), u8 cycles) {
	u8 branch_taken = 0;
	i8 offset = BUS_readFromMem(f());
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



//		Block Move

//	Block move next
u8 MVN() {
	u8 dst_bank = BUS_readFromMem(regs.PC + 1);
	u8 src_bank = BUS_readFromMem(regs.PC + 2);
	regs.setDataBankRegister(dst_bank);
	u32 dst = (dst_bank << 16) | regs.getY();
	u32 src = (src_bank << 16) | regs.getX();
	u8 val = BUS_readFromMem(src);
	BUS_writeToMem(val, dst);
	regs.setAccumulator((u16)(regs.getAccumulator()-1));
	regs.setX((u16)(regs.getX() + 1));
	regs.setY((u16)(regs.getY() + 1));
	if (regs.getAccumulator() == 0xffff)
		regs.PC += 3;
	return 7;
}

//	Block move prev
u8 MVP() {
	u8 dst_bank = BUS_readFromMem(regs.PC + 1);
	u8 src_bank = BUS_readFromMem(regs.PC + 2);
	regs.setDataBankRegister(dst_bank);
	u32 dst = (dst_bank << 16) | regs.getY();
	u32 src = (src_bank << 16) | regs.getX();
	u8 val = BUS_readFromMem(src);
	BUS_writeToMem(val, dst);
	regs.setAccumulator((u16)(regs.getAccumulator() - 1));
	regs.setX((u16)(regs.getX() - 1));
	regs.setY((u16)(regs.getY() - 1));
	if (regs.getAccumulator() == 0xffff)
		regs.PC += 3;
	return 7;
}



//		Misc

//	Unimplemented (does nothing)
u8 WDM() {
	regs.PC += 2;
	return 2;
}

//	Break
u8 BRK() {
	if (regs.P.getEmulation()) {
		regs.PC += 2;
		pushToStack((u8)(regs.PC >> 8));
		pushToStack((u8)(regs.PC & 0xff));
		regs.P.setBreak(1);
		pushToStack(regs.P.getByte());
		regs.P.setIRQDisable(1);
		regs.P.setDecimal(0);
		u8 lo = BUS_readFromMem(VECTOR_EMU_IRQBRK);
		u8 hi = BUS_readFromMem(VECTOR_EMU_IRQBRK + 1);
		regs.PC = (hi << 8) | lo;
	}
	else {
		pushToStack(regs.getProgramBankRegister());
		regs.PC += 2;
		pushToStack((u8)(regs.PC >> 8));
		pushToStack((u8)(regs.PC & 0xff));
		regs.P.setBreak(1);
		pushToStack(regs.P.getByte());
		regs.setProgramBankRegister(0x00);
		regs.P.setIRQDisable(1);
		regs.P.setDecimal(0);
		u8 lo = BUS_readFromMem(VECTOR_NATIVE_BRK);
		u8 hi = BUS_readFromMem(VECTOR_NATIVE_BRK + 1);
		regs.PC = (hi << 8) | lo;
	}
	return 7 + regs.P.getEmulation();
}

//	Co-copressor enable
u8 COP() {
	if (regs.P.getEmulation()) {
		regs.PC += 2;
		pushToStack((u8)(regs.PC >> 8));
		pushToStack((u8)(regs.PC & 0xff));
		pushToStack(regs.P.getByte());
		regs.P.setIRQDisable(1);
		u8 lo = BUS_readFromMem(VECTOR_EMU_COP);
		u8 hi = BUS_readFromMem(VECTOR_EMU_COP + 1);
		regs.PC = (hi << 8) | lo;
		regs.P.setDecimal(0);
	}
	else {
		pushToStack(regs.getProgramBankRegister());
		regs.PC += 2;
		pushToStack((u8)(regs.PC >> 8));
		pushToStack((u8)(regs.PC & 0xff));
		pushToStack(regs.P.getByte());
		regs.P.setIRQDisable(1);
		regs.setProgramBankRegister(0x00);
		u8 lo = BUS_readFromMem(VECTOR_NATIVE_COP);
		u8 hi = BUS_readFromMem(VECTOR_NATIVE_COP + 1);
		regs.PC = (hi << 8) | lo;
		regs.P.setDecimal(0);
	}
	return 7 + regs.P.getEmulation();
}

//	Return from interrupt
u8 RTI() {
	if (regs.P.getEmulation()) {
		regs.P.setByte(pullFromStack());
		regs.P.setAccuMemSize(1);
		regs.P.setIndexSize(1, &regs);
		u8 lo = pullFromStack();
		u8 hi = pullFromStack();
		regs.PC = (hi << 8) | lo;
	}
	else {
		u8 p = pullFromStack();
		regs.P.setByte(p);
		u8 lo = pullFromStack();
		u8 hi = pullFromStack();
		regs.PC = (hi << 8) | lo;
		regs.setProgramBankRegister(pullFromStack());
	}
	return 6 + regs.P.getEmulation();
}

//	Wait for interrupt
u8 WAI() {
	if (interrupts.is(Interrupts::NMI)) {
		if (regs.P.getEmulation()) {
			regs.PC++;
			pushToStack((u8)(regs.PC >> 8));
			pushToStack((u8)(regs.PC & 0xff));
			pushToStack(regs.P.getByte());
			regs.P.setIRQDisable(1);
			u8 lo = BUS_readFromMem(VECTOR_EMU_NMI);
			u8 hi = BUS_readFromMem(VECTOR_EMU_NMI + 1);
			regs.PC = (hi << 8) | lo;
			regs.P.setDecimal(0);
		}
		else {
			pushToStack(regs.getProgramBankRegister());
			regs.PC++;
			pushToStack((u8)(regs.PC >> 8));
			pushToStack((u8)(regs.PC & 0xff));
			pushToStack(regs.P.getByte());
			regs.P.setIRQDisable(1);
			regs.setProgramBankRegister(0x00);
			u8 lo = BUS_readFromMem(VECTOR_NATIVE_NMI);
			u8 hi = BUS_readFromMem(VECTOR_NATIVE_NMI + 1);
			regs.PC = (hi << 8) | lo;
			regs.P.setDecimal(0);
		}
		return 3;
	}
	return 3;
}

//	Stop the processor
u8 STP() {
	CPU_STOPPED = true;
	return 3;
}


//		Addressing modes
bool pbr = false;
u32 ADDR_getImmediate_8() {
	regs.PC++;
	u8 pbr = regs.getProgramBankRegister();
	return (pbr << 16) | regs.PC;
}
u32 ADDR_getImmediate_16() {
	regs.PC += 2;
	u8 pbr = regs.getProgramBankRegister();
	u32 adr = (pbr << 16) | regs.PC - 1;
	return adr;
}
u32 ADDR_getAbsolute() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u16 adr = ((BUS_readFromMem(regs.PC) << 8) | BUS_readFromMem(regs.PC-1));
	return (dbr << 16) | adr;
}
u32 ADDR_getAbsoluteLong() {
	regs.PC += 3;
	u16 adr = ((BUS_readFromMem(regs.PC) << 16) | (BUS_readFromMem(regs.PC - 1) << 8) | BUS_readFromMem(regs.PC - 2));
	return adr;
}
u32 ADDR_getAbsoluteIndexedX() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u16 adr = ((BUS_readFromMem(regs.PC) << 8) | BUS_readFromMem(regs.PC - 1));
	pbr = (adr & 0xff00) != ((adr + regs.getX()) & 0xff00);
	adr = adr + regs.getX();
	return (dbr << 16) | adr;
}
u32 ADDR_getAbsoluteIndexedY() {
	regs.PC += 2;
	u8 dbr = regs.getDataBankRegister();
	u16 adr = ((BUS_readFromMem(regs.PC) << 8) | BUS_readFromMem(regs.PC - 1));
	pbr = (adr & 0xff00) != ((adr + regs.getY()) & 0xff00);
	adr = adr + regs.getY();
	return (dbr << 16) | adr;
}
u32 ADDR_getAbsoluteLongIndexedX() {
	regs.PC += 3;
	u8 dbr = regs.getDataBankRegister();
	return (dbr << 16) | (BUS_readFromMem(regs.PC - 1) << 8) | BUS_readFromMem(regs.PC - 2) + regs.getX();
}
u32 ADDR_getAbsoluteIndirect() {
	regs.PC += 2;
	u8 lo = BUS_readFromMem(regs.PC-1);
	u8 hi = BUS_readFromMem(regs.PC);
	u16 adr = (hi << 8) | lo;
	u8 i_lo = BUS_readFromMem(adr);
	u8 i_hi = BUS_readFromMem(adr + 1);
	return (i_hi << 8) | i_lo;
}
u32 ADDR_getAbsoluteIndirectLong() {
	regs.PC += 2;
	u8 lo = BUS_readFromMem(regs.PC - 1);
	u8 hi = BUS_readFromMem(regs.PC);
	u16 adr = (hi << 8) | lo;
	u8 i_lo = BUS_readFromMem(adr);
	u8 i_hi = BUS_readFromMem(adr + 1);
	u8 i_bnk = BUS_readFromMem(adr + 2);
	return (i_bnk << 16) | (i_hi << 8) | i_lo;
}
u32 ADDR_getAbsoluteIndexedIndirectX() {
	regs.PC += 2;
	u8 lo = BUS_readFromMem(regs.PC - 1);
	u8 hi = BUS_readFromMem(regs.PC);
	u16 adr = (hi << 8) | lo + regs.getX();
	u8 i_lo = BUS_readFromMem(adr);
	u8 i_hi = BUS_readFromMem(adr + 1);
	return (i_hi << 8) | i_lo;
}
u32 ADDR_getLong() {
	regs.PC += 3;
	u8 dbr = regs.getDataBankRegister();
	return (dbr << 16) | (BUS_readFromMem(regs.PC - 1) << 8) | BUS_readFromMem(regs.PC - 2);
}
u32 ADDR_getDirectPage() {
	regs.PC++;
	return regs.getDirectPageRegister() | BUS_readFromMem(regs.PC);
}
u32 ADDR_getDirectPageIndexedX() {
	regs.PC++;
	return regs.getDirectPageRegister() | BUS_readFromMem(regs.PC) + regs.getX();
}
u32 ADDR_getDirectPageIndexedY() {
	regs.PC++;
	return regs.getDirectPageRegister() | BUS_readFromMem(regs.PC) + regs.getY();
}
u32 ADDR_getDirectPageIndirect() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = BUS_readFromMem(regs.PC + regs.getDirectPageRegister());
	u16 dp_adr = (BUS_readFromMem(dp_index + 1) << 8) | BUS_readFromMem(dp_index);
	return (dbr << 16) | dp_adr;
}
u32 ADDR_getDirectPageIndirectLong() {
	regs.PC++;
	/*u8 dbr = regs.getDataBankRegister();
	u8 dp_index = readFromMem(regs.PC + regs.getDirectPageRegister());
	u32 dp_adr = (readFromMem(dp_index + 2) << 16) | (readFromMem(dp_index + 1) << 8) | readFromMem(dp_index);*/
	u16 dp_index = BUS_readFromMem(regs.PC) + regs.getDirectPageRegister();
	u32 dp_adr = (BUS_readFromMem(dp_index + 2) << 16) | (BUS_readFromMem(dp_index + 1) << 8) | BUS_readFromMem(dp_index);
	return dp_adr;
}
u32 ADDR_getDirectPageIndirectX() {
	regs.PC++;
	u8 dbr = regs.getDataBankRegister();
	u8 dp_index = BUS_readFromMem(regs.PC + regs.getDirectPageRegister()) + regs.getX();
	u16 dp_adr = (BUS_readFromMem(dp_index + 1) << 8) | BUS_readFromMem(dp_index);
	return (dbr << 16) | dp_adr;
}
u32 ADDR_getDirectPageIndirectIndexedY() {
	regs.PC++;
	u8 dp_index = BUS_readFromMem(regs.PC + regs.getDirectPageRegister());
	u16 dp_adr = (regs.getDataBankRegister() << 16) | (BUS_readFromMem(dp_index + 1) << 8) | BUS_readFromMem(dp_index);
	pbr = (dp_adr & 0xff00) != ((dp_adr + regs.getY()) & 0xff00);
	dp_adr += regs.getY();
	return dp_adr;
}
u32 ADDR_getDirectPageIndirectLongIndexedY() {
	regs.PC++;
	u8 dp_index = BUS_readFromMem(regs.PC + regs.getDirectPageRegister());
	u16 dp_adr = (BUS_readFromMem(dp_index + 2) << 16) | (BUS_readFromMem(dp_index + 1) << 8) | BUS_readFromMem(dp_index);
	pbr = (dp_adr & 0xff00) != ((dp_adr + regs.getY()) & 0xff00);
	dp_adr += regs.getY();
	return dp_adr;
}
u32 ADDR_getStackRelative() {
	regs.PC++;
	u8 byte = BUS_readFromMem(regs.PC);
	return regs.getSP() + byte;
}
u32 ADDR_getStackRelativeIndirectIndexedY() {
	regs.PC++;
	u8 byte = BUS_readFromMem(regs.PC);
	u8 base = BUS_readFromMem((regs.getDataBankRegister() << 16) | regs.getSP() + byte);
	return base + regs.getY();
}
bool pageBoundaryCrossed() {
	bool v = pbr;
	pbr = false;
	return v;
}


u8 CPU_step() {
	//string flags = byteToBinaryString(regs.P.getByte());
	//printf("Op: %02x %02x %02x %02x  PC : 0x%04x A: 0x%04x X: 0x%04x Y: 0x%04x SP: 0x%04x D: 0x%04x DB: 0x%02x P: %s (0x%02x) Emu: %s\n", readFromMem(regs.PC, regs.getDataBankRegister()), readFromMem(regs.PC+1, regs.getDataBankRegister()), readFromMem(regs.PC+2, regs.getDataBankRegister()), readFromMem(regs.PC + 3, regs.getDataBankRegister()), regs.PC, regs.getAccumulator(), regs.getX(), regs.getY(), regs.getSP(), regs.getDirectPageRegister(), regs.getDataBankRegister(), flags.c_str(), regs.P.getByte(), regs.P.getEmulation() ? "true" : "false");
	//printf("%02x%04x A:%04x X:%04x Y:%04x S:%04x D:%04x DB:%02x %s \n", regs.getProgramBankRegister(), regs.PC, regs.getAccumulator(), regs.getX(), regs.getY(), regs.getSP(), regs.getDirectPageRegister(), regs.getDataBankRegister(), flags.c_str());
	if (!CPU_STOPPED) {
		switch (BUS_readFromMem((regs.getProgramBankRegister() << 16) | regs.PC)) {

		case 0x00:	return BRK(); break;
		case 0x01:	return ORA(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x02:	return COP(); break;
		case 0x03:	return ORA(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0x04:	return TSB(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0x05:	return ORA(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x06:	return ASL(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0x07:	return ORA(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x08:	return PHP(); break;
		case 0x09:	return (regs.P.getAccuMemSize()) ? ORA(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : ORA(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
		case 0x0a:	return ASL_A(2); break;
		case 0x0b:	return PHD(); break;
		case 0x0c:	return TSB(ADDR_getAbsolute, 6 + (2 * regs.P.isMReset())); break;
		case 0x0d:	return ORA(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0x0e:	return ASL(ADDR_getAbsolute, 6 + regs.P.isMReset()); break;
		case 0x0f:	return ORA(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
		case 0x10:	return BPL(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0x11:	return ORA(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0x12:	return ORA(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x13:	return ORA(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0x14:	return TRB(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0x15:	return ORA(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x16:	return ASL(ADDR_getDirectPageIndexedX, 6 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0x17:	return ORA(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x18:	return CLC(); break;
		case 0x19:	return ORA(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x1a:	return INC_A(); break;
		case 0x1b:	return TCS(); break;
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
		case 0x28:	return PLP(); break;
		case 0x29:	return (regs.P.getAccuMemSize()) ? AND(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : AND(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
		case 0x2a:	return ROL_A(2); break;
		case 0x2b:	return PLD(); break;
		case 0x2c:	return BIT(ADDR_getAbsolute, 4, false); break;
		case 0x2d:	return AND(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0x2e:	return ROL(ADDR_getAbsolute, 6 + regs.P.isMReset()); break;
		case 0x2f:	return AND(ADDR_getLong, 5 + regs.P.isMReset()); break;
		case 0x30:	return BMI(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0x31:	return AND(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0x32:	return AND(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x33:	return AND(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0x34:	return BIT(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero(), false); break;
		case 0x35:	return AND(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x36:	return ROL(ADDR_getDirectPageIndexedX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x37:	return AND(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x38:	return SEC(2); break;
		case 0x39:	return AND(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x3a:	return DEC_A(); break;
		case 0x3b:	return TSC(); break;
		case 0x3c:	return BIT(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed(), false); break;
		case 0x3d:	return AND(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x3e:	return ROL(ADDR_getAbsoluteIndexedX, 7 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x3f:	return AND(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
		case 0x40:	return RTI(); break;
		case 0x41:	return EOR(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x42:	return WDM(); break;
		case 0x43:	return EOR(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0x44:	return MVP(); break;
		case 0x45:	return EOR(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x46:	return LSR(ADDR_getDirectPage, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x47:	return EOR(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x48:	return PHA(3 + regs.P.isMReset()); break;
		case 0x49:	return (regs.P.getAccuMemSize()) ? EOR(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : EOR(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
		case 0x4a:	return LSR_A(); break;
		case 0x4b:	return PHK(); break;
		case 0x4c:	return JMP(ADDR_getAbsolute, 3); break;
		case 0x4d:	return EOR(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0x4e:	return LSR(ADDR_getAbsolute, 6 + regs.P.isMReset()); break;
		case 0x4f:	return EOR(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
		case 0x50:	return BVC(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0x51:	return EOR(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0x52:	return EOR(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x53:	return EOR(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0x54:	return MVN(); break;
		case 0x55:	return EOR(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x56:	return LSR(ADDR_getDirectPageIndexedX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x57:	return EOR(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x58:	return CLI(); break;
		case 0x59:	return EOR(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x5a:	return PHY(3 + regs.P.getIndexSize()); break;
		case 0x5b:	return TCD(); break;
		case 0x5c:	return JML(ADDR_getLong, 4); break;
		case 0x5d:	return EOR(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x5e:	return LSR(ADDR_getAbsoluteIndexedX, 7 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x5f:	return EOR(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
		case 0x60:	return RTS(6); break;
		case 0x61:	return ADC(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x62:	return PER(6); break;
		case 0x63:	return ADC(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0x64:	return STZ(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
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
		case 0x70:	return BVS(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0x71:	return ADC(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0x72:	return ADC(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x73:	return ADC(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0x74:	return STZ(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x75:	return ADC(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x76:	return ROR(ADDR_getDirectPageIndexedX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x77:	return ADC(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x78:	return SEI(); break;
		case 0x79:	return ADC(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x7a:	return PLY(); break;
		case 0x7b:	return TDC(); break;
		case 0x7c:	return JMP(ADDR_getAbsoluteIndexedIndirectX, 5); break;
		case 0x7d:	return ADC(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x7e:	return ROR(ADDR_getAbsoluteIndexedX, 7 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0x7f:	return ADC(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
		case 0x80:	return BRA(ADDR_getImmediate_8, 3 + regs.P.getEmulation()); break;
		case 0x81:	return STA(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x82:	return BRL(ADDR_getImmediate_16, 2 + regs.P.getEmulation()); break;
		case 0x83:	return STA(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0x84:	return STY(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0x85:	return STA(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x86:	return STX(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0x87:	return STA(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x88:	return DEY(); break;
		case 0x89:	return (regs.P.getAccuMemSize()) ? BIT(ADDR_getImmediate_8, 2 + regs.P.isMReset(), true) : BIT(ADDR_getImmediate_16, 2 + regs.P.isMReset(), true); break;
		case 0x8a:	return TXA(); break;
		case 0x8b:	return PHB(); break;
		case 0x8c:	return STY(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
		case 0x8d:	return STA(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0x8e:	return STX(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
		case 0x8f:	return STA(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
		case 0x90:	return BCC(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0x91:	return STA(ADDR_getDirectPageIndirectIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x92:	return STA(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x93:	return STA(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0x94:	return STY(ADDR_getDirectPageIndexedX, 4 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0x95:	return STA(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x96:	return STX(ADDR_getDirectPageIndexedY, 4 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0x97:	return STA(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0x98:	return TYA(); break;
		case 0x99:	return STA(ADDR_getAbsoluteIndexedY, 5 + regs.P.isMReset()); break;
		case 0x9a:	return TXS(); break;
		case 0x9b:	return TXY(); break;
		case 0x9c:	return STZ(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0x9d:	return STA(ADDR_getAbsoluteIndexedX, 5 + regs.P.isMReset()); break;
		case 0x9e:	return STZ(ADDR_getAbsoluteIndexedX, 5 + regs.P.isMReset()); break;
		case 0x9f:	return STA(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
		case 0xa0:	return (regs.P.getIndexSize()) ? LDY(ADDR_getImmediate_8, 2 + regs.P.isXReset()) : LDY(ADDR_getImmediate_16, 2 + regs.P.isXReset()); break;
		case 0xa1:	return LDA(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xa2:	return (regs.P.getIndexSize()) ? LDX(ADDR_getImmediate_8, 2 + regs.P.isXReset()) : LDX(ADDR_getImmediate_16, 2 + regs.P.isXReset()); break;
		case 0xa3:	return LDA(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0xa4:	return LDY(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0xa5:	return LDA(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xa6:	return LDX(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0xa7:	return LDA(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xa8:	return TAY(); break;
		case 0xa9:	return (regs.P.getAccuMemSize()) ? LDA(ADDR_getImmediate_8, 2 + regs.P.isXReset()) : LDA(ADDR_getImmediate_16, 2 + regs.P.isXReset()); break;
		case 0xaa:	return TAX(); break;
		case 0xab:	return PLB(); break;
		case 0xac:	return LDY(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
		case 0xad:	return LDA(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0xae:	return LDX(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
		case 0xaf:	return LDA(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
		case 0xb0:	return BCS(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0xb1:	return LDA(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0xb2:	return LDA(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xb3:	return LDA(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0xb4:	return LDY(ADDR_getDirectPageIndexedX, 4 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0xb5:	return LDA(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xb6:	return LDX(ADDR_getDirectPageIndexedY, 4 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0xb7:	return LDA(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xb8:	return CLV(); break;
		case 0xb9:	return LDA(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0xba:	return TSX(); break;
		case 0xbb:	return TYX(); break;
		case 0xbc:	return LDY(ADDR_getAbsoluteIndexedX, 4 + regs.P.isXReset() + pageBoundaryCrossed()); break;
		case 0xbd:	return LDA(ADDR_getAbsoluteIndexedX, 4 + pageBoundaryCrossed() + regs.P.isMReset()); break;
		case 0xbe:	return LDX(ADDR_getAbsoluteIndexedY, 4 + regs.P.isXReset() + pageBoundaryCrossed()); break;
		case 0xbf:	return LDA(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
		case 0xc0:	return (regs.P.getIndexSize()) ? CPY(ADDR_getImmediate_8, 2 + regs.P.getIndexSize()) : CPY(ADDR_getImmediate_16, 2 + regs.P.getIndexSize()); break;
		case 0xc1:	return CMP(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xc2:	return REP(); break;
		case 0xc3:	return CMP(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0xc4:	return CPY(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0xc5:	return CMP(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xc6:	return DEC(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0xc7:	return CMP(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xc8:	return INY(); break;
		case 0xc9:	return (regs.P.getAccuMemSize()) ? CMP(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : CMP(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
		case 0xca:	return DEX(); break;
		case 0xcb:	return WAI(); break;
		case 0xcc:	return CPY(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
		case 0xcd:	return CMP(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0xce:	return DEC(ADDR_getAbsolute, +6 + (2 * regs.P.isMReset())); break;
		case 0xcf:	return CMP(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
		case 0xd0:	return BNE(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0xd1:	return CMP(ADDR_getDirectPageIndirectLongIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0xd2:	return CMP(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xd3:	return CMP(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0xd4:	return PEI(ADDR_getDirectPageIndirect, 6 + regs.isDPLowNotZero()); break;
		case 0xd5:	return CMP(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xd6:	return DEC(ADDR_getDirectPageIndexedX, 6 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0xd7:	return CMP(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xd8:	return CLD(); break;
		case 0xd9:	return CMP(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0xda:	return PHX(3 + regs.P.getIndexSize()); break;
		case 0xdb:	return STP(); break;
		case 0xdc:	return JMP(ADDR_getAbsoluteIndirectLong, 6); break;
		case 0xdd:	return CMP(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0xde:	return DEC(ADDR_getAbsoluteIndexedX, 7 + (2 * regs.P.isMReset()) + pageBoundaryCrossed()); break;
		case 0xdf:	return CMP(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;
		case 0xe0:	return (regs.P.getIndexSize()) ? CPX(ADDR_getImmediate_8, 2 + regs.P.getIndexSize()) : CPX(ADDR_getImmediate_16, 2 + regs.P.getIndexSize()); break;
		case 0xe1:	return SBC(ADDR_getDirectPageIndirectX, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xe2:	return SEP(); break;
		case 0xe3:	return SBC(ADDR_getStackRelative, 4 + regs.P.isMReset()); break;
		case 0xe4:	return CPX(ADDR_getDirectPage, 3 + regs.P.isXReset() + regs.isDPLowNotZero()); break;
		case 0xe5:	return SBC(ADDR_getDirectPage, 3 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xe6:	return INC(ADDR_getDirectPage, 5 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0xe7:	return SBC(ADDR_getDirectPageIndirectLong, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xe8:	return INX(); break;
		case 0xe9:	return (regs.P.getAccuMemSize()) ? SBC(ADDR_getImmediate_8, 2 + regs.P.isMReset()) : SBC(ADDR_getImmediate_16, 2 + regs.P.isMReset()); break;
		case 0xea:	return NOP(); break;
		case 0xeb:	return XBA(); break;
		case 0xec:	return CPX(ADDR_getAbsolute, 4 + regs.P.isXReset()); break;
		case 0xed:	return SBC(ADDR_getAbsolute, 4 + regs.P.isMReset()); break;
		case 0xee:	return INC(ADDR_getAbsolute, 6 + (2 * regs.P.isMReset())); break;
		case 0xef:	return SBC(ADDR_getAbsoluteLong, 5 + regs.P.isMReset()); break;
		case 0xf0:	return BEQ(ADDR_getImmediate_8, 2 + regs.P.getEmulation()); break;
		case 0xf1:	return SBC(ADDR_getDirectPageIndirectIndexedY, 5 + regs.P.isMReset() + regs.isDPLowNotZero() + pageBoundaryCrossed()); break;
		case 0xf2:	return SBC(ADDR_getDirectPageIndirect, 5 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xf3:	return SBC(ADDR_getStackRelativeIndirectIndexedY, 7 + regs.P.isMReset()); break;
		case 0xf4:	return PEA(ADDR_getAbsolute, 5); break;
		case 0xf5:	return SBC(ADDR_getDirectPageIndexedX, 4 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xf6:	return INC(ADDR_getDirectPageIndexedX, 6 + (2 * regs.P.isMReset()) + regs.isDPLowNotZero()); break;
		case 0xf7:	return SBC(ADDR_getDirectPageIndirectLongIndexedY, 6 + regs.P.isMReset() + regs.isDPLowNotZero()); break;
		case 0xf8:	return SED(); break;
		case 0xf9:	return SBC(ADDR_getAbsoluteIndexedY, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0xfa:	return PLX(); break;
		case 0xfb:	return XCE(); break;
		case 0xfc:	return JSR(ADDR_getAbsoluteIndexedIndirectX, 8); break;
		case 0xfd:	return SBC(ADDR_getAbsoluteIndexedX, 4 + regs.P.isMReset() + pageBoundaryCrossed()); break;
		case 0xfe:	return INC(ADDR_getAbsoluteIndexedX, 7 + (2 * regs.P.isMReset()) + pageBoundaryCrossed()); break;
		case 0xff:	return SBC(ADDR_getAbsoluteLongIndexedX, 5 + regs.P.isMReset()); break;

		default:
			printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", BUS_readFromMem(regs.PC), regs.PC);
			exit(1);
			break;
		}
	}
	return 0;
}