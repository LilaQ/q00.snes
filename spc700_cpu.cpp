#include "spc700_cpu.h"
#include "spc700_mmu.h"

Registers regs;

void MOV_A(u8 val) {
	regs.A = val;
	regs.PSW.N = regs.A >> 7;
	regs.PSW.Z = regs.A == 0;
}
void MOV_A(u16(*f)()) {
	u8 adr = f();
	regs.A = SPC700_readFromMem(adr);
	regs.PSW.N = regs.A >> 7;
	regs.PSW.Z = regs.A == 0;
}

void MOV_X(u8 val) {
	regs.X = val;
	regs.PSW.N = regs.X >> 7;
	regs.PSW.Z = regs.X == 0;
}
void MOV_X(u16(*f)()) {
	u8 adr = f();
	regs.X = SPC700_readFromMem(adr);
	regs.PSW.N = regs.X >> 7;
	regs.PSW.Z = regs.X == 0;
}

void MOV_Y(u8 val) {
	regs.Y = val;
	regs.PSW.N = regs.Y >> 7;
	regs.PSW.Z = regs.Y == 0;
}
void MOV_Y(u16(*f)()) {
	u8 adr = f();
	regs.Y = SPC700_readFromMem(adr);
	regs.PSW.N = regs.Y >> 7;
	regs.PSW.Z = regs.Y == 0;
}

void MOVW_YA_nn() {
	u8 y = SPC700_readFromMem(regs.PC++);
	u8 a = SPC700_readFromMem(regs.PC++);
	regs.PSW.N = ((y << 8) | a) & 0x8000;
	regs.PSW.Z = !((y << 8) | a);
	regs.Y = y;
	regs.A = a;
}


/*
*	ADDRESSING MODES
*/
u16 ADDR_X() {
	u16 adr = regs.X | regs.PSW.zeropage;
	return adr;
}
u16 ADDR_X_INC() {
	u16 adr = regs.X | regs.PSW.zeropage;
	regs.X++;
	return adr;
}
u16 ADDR_immediate8() {
	u16 adr = SPC700_readFromMem(regs.PC++) | regs.PSW.zeropage;
	return adr;
}
u16 ADDR_immediate8X() {
	u16 adr = (u8)(SPC700_readFromMem(regs.PC++) + regs.X) | regs.PSW.zeropage;
	return adr;
}
u16 ADDR_immediate8Y() {
	u16 adr = (u8)(SPC700_readFromMem(regs.PC++) + regs.Y) | regs.PSW.zeropage;
	return adr;
}
u16 ADDR_immediate16() {
	u8 lo = SPC700_readFromMem(regs.PC++);
	u8 hi = SPC700_readFromMem(regs.PC++);
	u16 adr = SPC700_readFromMem((hi << 8) | lo);
	return adr;
}
u16 ADDR_immediate16X() {
	u8 lo = SPC700_readFromMem(regs.PC++);
	u8 hi = SPC700_readFromMem(regs.PC++);
	u16 adr = SPC700_readFromMem((hi << 8) | lo + regs.X);
	return adr;
}
u16 ADDR_immediate16Y() {
	u8 lo = SPC700_readFromMem(regs.PC++);
	u8 hi = SPC700_readFromMem(regs.PC++);
	u16 adr = SPC700_readFromMem((hi << 8) | lo + regs.Y);
	return adr;
}
u16 ADDR_indirectY() {
	u16 adr_inner = SPC700_readFromMem(regs.PC++) | regs.PSW.zeropage;
	u16 adr_outer = SPC700_readFromMem(adr_inner) + regs.Y;
	return adr_outer;
}
u16 ADDR_indirectX() {
	u16 adr_inner = SPC700_readFromMem(regs.PC++) | regs.PSW.zeropage;
	u16 adr_outer = SPC700_readFromMem(adr_inner + regs.Y);
	return adr_outer;
}

void SPC700_tick() {
	switch (regs.PC++)
	{
	case 0x5d: MOV_X(regs.A); break;
	case 0x7d: MOV_A(regs.X); break;
	case 0x8d: MOV_Y(SPC700_readFromMem(regs.PC++)); break;
	case 0x9d: MOV_X((u8)regs.SP); break;
	case 0xba: MOVW_YA_nn(); break;
	case 0xbd: regs.SP = 0x100 | regs.X; break;							
	case 0xbf: MOV_A(ADDR_X_INC); break;
	case 0xcd: MOV_X(SPC700_readFromMem(regs.PC++)); break;
	case 0xdd: MOV_A(regs.Y); break;
	case 0xe4: MOV_A(ADDR_immediate8); break;
	case 0xe5: MOV_A(ADDR_immediate16); break;
	case 0xe6: MOV_A(ADDR_X()); break;
	case 0xe7: MOV_A(ADDR_indirectX); break;
	case 0xe8: MOV_A(SPC700_readFromMem(regs.PC++)); break;
	case 0xe9: MOV_X(ADDR_immediate16); break;
	case 0xeb: MOV_Y(ADDR_immediate8); break;
	case 0xec: MOV_Y(ADDR_immediate16); break;
	case 0xf4: MOV_A(ADDR_immediate8X); break;
	case 0xf5: MOV_A(ADDR_immediate16X); break;
	case 0xf6: MOV_A(ADDR_immediate16Y); break;
	case 0xf7: MOV_A(ADDR_indirectY); break;
	case 0xf8: MOV_X(ADDR_immediate8); break;
	case 0xf9: MOV_X(ADDR_immediate8Y); break;
	case 0xfb: MOV_Y(ADDR_immediate8X); break;
	case 0xfd: MOV_Y(regs.A); break;
	default:
		break;
	}
}