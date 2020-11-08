#include "spc700_cpu.h"

SPC700_Registers SPC700_regs;

void SPC700_RESET() {
	SPC700_regs.PC = 0xFFFE;
	SPC700_regs.PSW.zeropage = 0;
	SPC700_regs.PSW.B = 0;
	SPC700_regs.PSW.I = 0;
}


//	INSTRUCTIONS

void MOV_A(u8 val) {
	SPC700_regs.A = val;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void MOV_A(u16(*f)()) {
	u16 adr = f();
	SPC700_regs.A = SPC700_readFromMem(adr);
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}

void MOV_X(u8 val) {
	SPC700_regs.X = val;
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void MOV_X(u16(*f)()) {
	u16 adr = f();
	SPC700_regs.X = SPC700_readFromMem(adr);
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}

void MOV_Y(u8 val) {
	SPC700_regs.Y = val;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void MOV_Y(u16(*f)()) {
	u16 adr = f();
	SPC700_regs.Y = SPC700_readFromMem(adr);
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}

void MOVW_YA_nn() {
	u8 adr = SPC700_readFromMem(SPC700_regs.PC++) | SPC700_regs.PSW.zeropage;
	u8 a = SPC700_readFromMem(adr);
	u8 y = SPC700_readFromMem(adr+1);
	SPC700_regs.PSW.N = ((y << 8) | a) & 0x8000;
	SPC700_regs.PSW.Z = !((y << 8) | a);
	SPC700_regs.Y = y;
	SPC700_regs.A = a;
}
void MOVW_nn_YA() {
	SPC700_readFromMem(SPC700_regs.A);
	u16 adr = SPC700_readFromMem(SPC700_regs.PC++);
	SPC700_writeToMem(adr, SPC700_regs.A);
	SPC700_writeToMem(adr + 1, SPC700_regs.Y);
}

void MOV_nn_WITH_READ(u16(*f)(), u8 val) {
	u16 adr = f();
	SPC700_readFromMem(adr);
	SPC700_writeToMem(adr, val);
}
void MOV_nn_NO_READ(u16(*f)(), u8 val) {
	u16 adr = f();
	SPC700_writeToMem(adr, val);
}

void PUSH(u8 val) {
	u16 adr = SPC700_readFromMem(SPC700_regs.SP) | SPC700_regs.PSW.zeropage;
	SPC700_regs.SP = (SPC700_regs.SP-- & 0xff) | 0x100;
	SPC700_writeToMem(adr, val);
}
void POP(u8 &val) {
	val = SPC700_readFromMem((++SPC700_regs.SP & 0xff) | 0x100);
}
void POP_SP() {
	SPC700_regs.PSW.setByte(SPC700_readFromMem((++SPC700_regs.SP & 0xff) | 0x100));
}

void OR_A(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.A = SPC700_regs.A | val;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void OR_n(u16(*f)(), u8 val) {
	u16 adr = f();
	u16 vil = SPC700_readFromMem(adr);
	vil = vil | val;
	SPC700_regs.PSW.N = vil >> 7;
	SPC700_regs.PSW.Z = vil == 0;
	SPC700_writeToMem(adr, vil);
}

void AND_A(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.A = SPC700_regs.A & val;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void AND_n(u16(*f)(), u8 val) {
	u16 adr = f();
	u16 vil = SPC700_readFromMem(adr);
	vil = vil & val;
	SPC700_regs.PSW.N = vil >> 7;
	SPC700_regs.PSW.Z = vil == 0;
	SPC700_writeToMem(adr, vil);
}

void XOR_A(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.A = SPC700_regs.A ^ val;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void XOR_n(u16(*f)(), u8 val) {
	u16 adr = f();
	u16 vil = SPC700_readFromMem(adr);
	vil = vil ^ val;
	SPC700_regs.PSW.N = vil >> 7;
	SPC700_regs.PSW.Z = vil == 0;
	SPC700_writeToMem(adr, vil);
}

void CMP_A(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val < SPC700_regs.A;
	SPC700_regs.PSW.N = ((SPC700_regs.A - val) & 0xff) >> 7;
	SPC700_regs.PSW.Z = (SPC700_regs.A - val) == 0;
}
void CMP_X(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val < SPC700_regs.X;
	SPC700_regs.PSW.N = ((SPC700_regs.X - val) & 0xff) >> 7;
	SPC700_regs.PSW.Z = (SPC700_regs.X - val) == 0;
}
void CMP_Y(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val < SPC700_regs.Y;
	SPC700_regs.PSW.N = ((SPC700_regs.Y - val) & 0xff) >> 7;
	SPC700_regs.PSW.Z = (SPC700_regs.Y - val) == 0;
}
void CMP_n(u16(*f)(), u8 val) {
	u16 adr = f();
	u8 vil = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val < vil;
	SPC700_regs.PSW.N = ((vil - val) & 0xff) >> 7;
	SPC700_regs.PSW.Z = vil == val;
}

void ADD_A(u8 val) {
	uint16_t sum = SPC700_regs.A + val + SPC700_regs.PSW.C;
	SPC700_regs.PSW.V = (~(SPC700_regs.A ^ val) & (SPC700_regs.A ^ sum) & 0x80) > 0;
	SPC700_regs.PSW.H = ((SPC700_regs.A & 0xf) + (val & 0xf) + SPC700_regs.PSW.C) & 0x10;
	SPC700_regs.PSW.C = sum & 0x100;
	SPC700_regs.A = (u8)sum;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void ADD_n(u16(*f)(), u8 val) {
	u16 adr = f();
	u8 vil = SPC700_readFromMem(adr);
	uint16_t sum = vil + val + SPC700_regs.PSW.C;
	SPC700_regs.PSW.V = (~(vil ^ val) & (vil ^ sum) & 0x80) > 0;
	SPC700_regs.PSW.H = ((vil & 0xf) + (val & 0xf) + SPC700_regs.PSW.C) & 0x10;
	SPC700_regs.PSW.C = sum & 0x100;
	SPC700_writeToMem(adr, (u8)sum);
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}

void ADC(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	ADD_A(val);
}
void ADC(u16(*f)(), u8 val) {
	ADD_n(f, val);
}
void SBC(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	ADD_A(~val);
}
void SBC(u16(*f)(), u8 val) {
	ADD_n(f, ~val);
}

void ASL_A() {
	SPC700_regs.PSW.C = SPC700_regs.A >> 7;
	SPC700_regs.A = SPC700_regs.A << 1;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void ASL_X() {
	SPC700_regs.PSW.C = SPC700_regs.X >> 7;
	SPC700_regs.X = SPC700_regs.X << 1;
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void ASL_Y() {
	SPC700_regs.PSW.C = SPC700_regs.Y >> 7;
	SPC700_regs.Y = SPC700_regs.Y << 1;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void ASL_n(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val >> 7;
	val = val << 1;
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	SPC700_writeToMem(adr, val);
}

void ROL_A() {
	SPC700_regs.PSW.C = SPC700_regs.A >> 7;
	SPC700_regs.A = (SPC700_regs.A << 1) | SPC700_regs.PSW.C;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void ROL_X() {
	SPC700_regs.PSW.C = SPC700_regs.X >> 7;
	SPC700_regs.X = (SPC700_regs.X << 1) | SPC700_regs.PSW.C;
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void ROL_Y() {
	SPC700_regs.PSW.C = SPC700_regs.Y >> 7;
	SPC700_regs.Y = (SPC700_regs.Y << 1) | SPC700_regs.PSW.C;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void ROL_n(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val >> 7;
	val = (val << 1) | SPC700_regs.PSW.C;
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	SPC700_writeToMem(adr, val);
}

void LSR_A() {
	SPC700_regs.PSW.C = SPC700_regs.A >> 7;
	SPC700_regs.A = SPC700_regs.A >> 1;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void LSR_X() {
	SPC700_regs.PSW.C = SPC700_regs.X >> 7;
	SPC700_regs.X = SPC700_regs.X >> 1;
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void LSR_Y() {
	SPC700_regs.PSW.C = SPC700_regs.Y >> 7;
	SPC700_regs.Y = SPC700_regs.Y >> 1;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void LSR_n(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val >> 7;
	val = val >> 1;
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	SPC700_writeToMem(adr, val);
}

void ROR_A() {
	SPC700_regs.PSW.C = SPC700_regs.A >> 7;
	SPC700_regs.A = (SPC700_regs.A >> 1) | (SPC700_regs.PSW.C << 7);
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void ROR_X() {
	SPC700_regs.PSW.C = SPC700_regs.X >> 7;
	SPC700_regs.X = (SPC700_regs.X >> 1) | (SPC700_regs.PSW.C << 7);
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void ROR_Y() {
	SPC700_regs.PSW.C = SPC700_regs.Y >> 7;
	SPC700_regs.Y = (SPC700_regs.Y >> 1) | (SPC700_regs.PSW.C << 7);
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void ROR_n(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = val >> 7;
	val = (val >> 1) | (SPC700_regs.PSW.C << 7);
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	SPC700_writeToMem(adr, val);
}

void DEC_A() {
	SPC700_regs.A = SPC700_regs.A - 1;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void DEC_X() {
	SPC700_regs.X = SPC700_regs.X - 1;
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void DEC_Y() {
	SPC700_regs.Y = SPC700_regs.Y - 1;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void DEC_n(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	val = val - 1;
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	SPC700_writeToMem(adr, val);
}

void INC_A() {
	SPC700_regs.A = SPC700_regs.A + 1;
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}
void INC_X() {
	SPC700_regs.X = SPC700_regs.X + 1;
	SPC700_regs.PSW.N = SPC700_regs.X >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.X == 0;
}
void INC_Y() {
	SPC700_regs.Y = SPC700_regs.Y + 1;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}
void INC_n(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	val = val + 1;
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	SPC700_writeToMem(adr, val);
}

void WADD(u16 val) {
	u32 sum = ((SPC700_regs.Y << 8) | SPC700_regs.A) + val;
	SPC700_regs.PSW.V = (~(((SPC700_regs.Y << 8) | SPC700_regs.A) ^ val) & (((SPC700_regs.Y << 8) | SPC700_regs.A) ^ sum) & 0x8000) > 0;
	SPC700_regs.PSW.H = ((((SPC700_regs.Y << 8) | SPC700_regs.A) & 0xff) + (val & 0xff)) & 0x100;
	SPC700_regs.PSW.C = sum & 0x10000;
	SPC700_regs.Y = (u8)(sum >> 8);
	SPC700_regs.A = (u8)sum;
	SPC700_regs.PSW.N = ((SPC700_regs.Y << 8) | SPC700_regs.A) >> 15;
	SPC700_regs.PSW.Z = ((SPC700_regs.Y << 8) | SPC700_regs.A) == 0;
}
void ADDW(u16(*f)()) {
	u16 adr = f();
	u8 lo = SPC700_readFromMem(adr);
	u8 hi = SPC700_readFromMem(adr+1);
	WADD((hi << 8) | lo);
}
void SUBW(u16(*f)()) {
	u16 adr = f();
	u8 lo = SPC700_readFromMem(adr);
	u8 hi = SPC700_readFromMem(adr + 1);
	WADD(~((hi << 8) | lo));
}
void CMPW(u16(*f)()) {
	u16 adr = f();
	u8 lo = SPC700_readFromMem(adr);
	u8 hi = SPC700_readFromMem(adr + 1);
	u16 val = (hi << 8) | lo;
	u32 sum = ((SPC700_regs.Y << 8) | SPC700_regs.A) - val;
	SPC700_regs.PSW.C = sum & 0x10000;
	SPC700_regs.PSW.N = ((SPC700_regs.Y << 8) | SPC700_regs.A) >> 15;
	SPC700_regs.PSW.Z = ((SPC700_regs.Y << 8) | SPC700_regs.A) == 0;
}
void INCW(u16(*f)()) {
	u16 adr = f();
	u8 lo = SPC700_readFromMem(adr);
	u8 hi = SPC700_readFromMem(adr + 1);
	u16 val = (hi << 8) | lo;
	val++;
	SPC700_writeToMem(adr, (u8)val);
	SPC700_writeToMem(adr + 1, (u8)val >> 8);
	SPC700_regs.PSW.N = ((SPC700_regs.Y << 8) | SPC700_regs.A) >> 15;
	SPC700_regs.PSW.Z = ((SPC700_regs.Y << 8) | SPC700_regs.A) == 0;
}
void DECW(u16(*f)()) {
	u16 adr = f();
	u8 lo = SPC700_readFromMem(adr);
	u8 hi = SPC700_readFromMem(adr + 1);
	u16 val = (hi << 8) | lo;
	val--;
	SPC700_writeToMem(adr, (u8)val);
	SPC700_writeToMem(adr + 1, (u8)val >> 8);
	SPC700_regs.PSW.N = ((SPC700_regs.Y << 8) | SPC700_regs.A) >> 15;
	SPC700_regs.PSW.Z = ((SPC700_regs.Y << 8) | SPC700_regs.A) == 0;
}
void DIV() {
	u32 sum = ((SPC700_regs.Y << 8) | SPC700_regs.A) / SPC700_regs.X;
	SPC700_regs.PSW.V = (~(((SPC700_regs.Y << 8) | SPC700_regs.A) ^ SPC700_regs.X) & (((SPC700_regs.Y << 8) | SPC700_regs.A) ^ sum) & 0x8000) > 0;
	SPC700_regs.PSW.H = ((((SPC700_regs.Y << 8) | SPC700_regs.A) & 0xff) + (SPC700_regs.X & 0xff)) & 0x100;
	SPC700_regs.Y = ((SPC700_regs.Y << 8) | SPC700_regs.A) % SPC700_regs.X;
	SPC700_regs.A = (u8)sum;
	SPC700_regs.PSW.N = ((SPC700_regs.Y << 8) | SPC700_regs.A) >> 15;
	SPC700_regs.PSW.Z = ((SPC700_regs.Y << 8) | SPC700_regs.A) == 0;
}
void MUL() {
	u16 sum = SPC700_regs.Y * SPC700_regs.A;
	SPC700_regs.Y = (u8)(sum >> 8);
	SPC700_regs.A = (u8)sum;
	SPC700_regs.PSW.N = SPC700_regs.Y >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.Y == 0;
}

void CLR(u16(*f)(), u8 bit) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	val &= ~(1 << bit);
	SPC700_writeToMem(adr, val);
}
void SET(u16(*f)(), u8 bit) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	val |= 1 << bit;
	SPC700_writeToMem(adr, val);
}
void NOT() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	val ^= 1 << bit;
	SPC700_writeToMem(adr, val);
}
void MOV_FROM_C() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	val &= 1 << bit;
	val |= SPC700_regs.PSW.C << bit;
	SPC700_writeToMem(adr, val);
}
void MOV_TO_C() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C = (bool)(val & (1 << bit));
	SPC700_writeToMem(adr, val);
}
void OR() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C |= (bool)(val & (1 << bit));
	SPC700_writeToMem(adr, val);
}
void OR_NOT() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C |= !(bool)(val & (1 << bit));
	SPC700_writeToMem(adr, val);
}
void AND() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C &= (bool)(val & (1 << bit));
	SPC700_writeToMem(adr, val);
}
void AND_NOT() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C &= !(bool)(val & (1 << bit));
	SPC700_writeToMem(adr, val);
}
void XOR() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 bit = hi >> 4;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.C ^= (bool)(val & (1 << bit));
	SPC700_writeToMem(adr, val);
}
void CLRC() {
	SPC700_regs.PSW.C = 0;
}
void SETC() {
	SPC700_regs.PSW.C = 1;
}
void NOTC() {
	SPC700_regs.PSW.C ^= 1;
}
void CLRV() {
	SPC700_regs.PSW.V = 0;
	SPC700_regs.PSW.H = 0;
}

void DAA() {
	if (SPC700_regs.PSW.C || SPC700_regs.A > 0x99) {
		SPC700_regs.A += 0x60;
		SPC700_regs.PSW.C = 1;
	}
	if (SPC700_regs.PSW.H || (SPC700_regs.A & 15) > 0x09) {
		SPC700_regs.A += 0x06;
	}
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
	SPC700_regs.PSW.N = SPC700_regs.A & 0x80;
}
void DAS() {
	if (!SPC700_regs.PSW.C || SPC700_regs.A > 0x99) {
		SPC700_regs.A -= 0x60;
		SPC700_regs.PSW.C = 0;
	}
	if (!SPC700_regs.PSW.H || (SPC700_regs.A & 15) > 0x09) {
		SPC700_regs.A -= 0x06;
	}
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
	SPC700_regs.PSW.N = SPC700_regs.A & 0x80;
}
void XCN() {
	SPC700_regs.A = (SPC700_regs.A >> 4) | (SPC700_regs.A << 4);
	SPC700_regs.PSW.N = SPC700_regs.A >> 7;
	SPC700_regs.PSW.Z = SPC700_regs.A == 0;
}

void TCLR() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	val = val & !SPC700_regs.A;
	SPC700_writeToMem(adr, val);
}
void TSET() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = ((hi & 0xf) << 8) | lo;
	u8 val = SPC700_readFromMem(adr);
	SPC700_regs.PSW.N = val >> 7;
	SPC700_regs.PSW.Z = val == 0;
	val = val | SPC700_regs.A;
	SPC700_writeToMem(adr, val);
}


//	BRANCHES

u8 BPL() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (!SPC700_regs.PSW.N) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BMI() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (SPC700_regs.PSW.N) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BVC() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (!SPC700_regs.PSW.V) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BVS() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (SPC700_regs.PSW.V) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BCC() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (!SPC700_regs.PSW.C) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BCS() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (SPC700_regs.PSW.C) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BNE() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (!SPC700_regs.PSW.Z) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BEQ() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (SPC700_regs.PSW.Z) {
		SPC700_regs.PC += off;
		return 4;
	}
	return 2;
}
u8 BBS(u8 bit) {
	u8 val = SPC700_readFromMem(SPC700_readFromMem(SPC700_regs.PC++) | SPC700_regs.PSW.zeropage);
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (val & (1 << bit)) {
		SPC700_regs.PC += off;
		return 7;
	}
	return 5;
}
u8 BBC(u8 bit) {
	u8 val = SPC700_readFromMem(SPC700_readFromMem(SPC700_regs.PC++) | SPC700_regs.PSW.zeropage);
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (!(val & (1 << bit))) {
		SPC700_regs.PC += off;
		return 7;
	}
	return 5;
}
u8 CBNE(u16(*f)()) {
	u16 adr = f();
	u8 val = SPC700_readFromMem(adr);
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (SPC700_regs.A != val) {
		SPC700_regs.PC += off;
		return 7;
	}
	return 5;
}
u8 DBNZ_Y() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (--SPC700_regs.Y == 0) {
		SPC700_regs.PC += off;
		return 6;
	}
	return 4;
}
u8 DBNZ_n() {
	u16 adr = SPC700_readFromMem(SPC700_regs.PC++);
	u8 val = SPC700_readFromMem(adr) - 1;
	SPC700_writeToMem(adr, val);
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	if (val == 0) {
		SPC700_regs.PC += off;
		return 7;
	}
	return 5;
}
void BRA() {
	i8 off = SPC700_readFromMem(SPC700_regs.PC++);
	SPC700_regs.PC += off;
}
void JMP() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	SPC700_regs.PC = (hi << 8) | lo;
}
void JMP_X() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = (hi << 8) | lo + SPC700_regs.X;
	u8 pl = SPC700_readFromMem(adr);
	u8 ph = SPC700_readFromMem(adr+1);
	SPC700_regs.PC = (ph << 8) | pl;
}
void SPC700_pushToStack(u8 val) {
	SPC700_writeToMem(SPC700_regs.SP, val);
	SPC700_regs.SP = (u8)(SPC700_regs.SP - 1) | 0x100;
}
u8 SPC700_popFromStack() {
	SPC700_regs.SP = (u8)(SPC700_regs.SP++) | 0x100;
	return SPC700_readFromMem(SPC700_regs.SP);
}
void CALL() {
	SPC700_pushToStack(SPC700_regs.PC >> 8);
	SPC700_pushToStack(SPC700_regs.PC & 0xff);
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	SPC700_regs.PC = (hi << 8) | lo;
}
void TCALL(u8 bit) {
	SPC700_pushToStack(SPC700_regs.PC >> 8);
	SPC700_pushToStack(SPC700_regs.PC & 0xff);
	u16 adr = 0xffde - (2 * bit);
	u8 lo = SPC700_readFromMem(adr);
	u8 hi = SPC700_readFromMem(adr + 1);
	SPC700_regs.PC = (hi << 8) | lo;
}
void PCALL() {
	SPC700_pushToStack(SPC700_regs.PC >> 8);
	SPC700_pushToStack(SPC700_regs.PC & 0xff);
	u16 adr = 0xff00 + SPC700_readFromMem(SPC700_regs.PC++);
	SPC700_regs.PC = adr;
}
void RET() {
	SPC700_regs.PC = SPC700_popFromStack() | (SPC700_popFromStack() << 8);
}
void RETI() {
	SPC700_regs.PSW.setByte(SPC700_popFromStack());
	SPC700_regs.PC = SPC700_popFromStack() | (SPC700_popFromStack() << 8);
}
void BRK() {
	SPC700_pushToStack(SPC700_regs.PC >> 8);
	SPC700_pushToStack(SPC700_regs.PC & 0xff);
	SPC700_pushToStack(SPC700_regs.PSW.getByte());
	u8 lo = SPC700_readFromMem(0xffde);
	u8 hi = SPC700_readFromMem(0xffde + 1);
	SPC700_regs.PC = (hi << 8) | lo;
	SPC700_regs.PSW.I = 0;
	SPC700_regs.PSW.B = 1;
}
void CLRP() {
	SPC700_regs.PSW.zeropage &= 0xff;
}
void SETP() {
	SPC700_regs.PSW.zeropage |= 0x100;
}
void EI() {
	SPC700_regs.PSW.I = 1;
}
void DI() {
	SPC700_regs.PSW.I = 0;
}

/*
*	ADDRESSING MODES
*/
u16 ADDR_PC() {
	return SPC700_regs.PC++;
}
u16 ADDR_X() {
	u16 adr = SPC700_regs.X | SPC700_regs.PSW.zeropage;
	return adr;
}
u16 ADDR_Y() {
	u16 adr = SPC700_regs.Y | SPC700_regs.PSW.zeropage;
	return adr;
}
u16 ADDR_X_INC() {
	u16 adr = SPC700_regs.X | SPC700_regs.PSW.zeropage;
	SPC700_regs.X++;
	return adr;
}
u16 ADDR_immediate8() {
	u16 adr = SPC700_readFromMem(SPC700_regs.PC++) | SPC700_regs.PSW.zeropage;
	return adr;
}
u16 ADDR_immediate8X() {
	u16 adr = (u8)(SPC700_readFromMem(SPC700_regs.PC++) + SPC700_regs.X) | SPC700_regs.PSW.zeropage;
	return adr;
}
u16 ADDR_immediate8Y() {
	u16 adr = (u8)(SPC700_readFromMem(SPC700_regs.PC++) + SPC700_regs.Y) | SPC700_regs.PSW.zeropage;
	return adr;
}
u16 ADDR_immediate16() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = SPC700_readFromMem((hi << 8) | lo);
	return adr;
}
u16 ADDR_immediate16X() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = SPC700_readFromMem((hi << 8) | lo + SPC700_regs.X);
	return adr;
}
u16 ADDR_immediate16Y() {
	u8 lo = SPC700_readFromMem(SPC700_regs.PC++);
	u8 hi = SPC700_readFromMem(SPC700_regs.PC++);
	u16 adr = SPC700_readFromMem((hi << 8) | lo + SPC700_regs.Y);
	return adr;
}
u16 ADDR_indirectY() {
	u16 adr_inner = SPC700_readFromMem(SPC700_regs.PC++) | SPC700_regs.PSW.zeropage;
	u16 adr_outer = SPC700_readFromMem(adr_inner) + SPC700_regs.Y;
	return adr_outer;
}
u16 ADDR_indirectX() {
	u16 adr_inner = SPC700_readFromMem(SPC700_regs.PC++) | SPC700_regs.PSW.zeropage;
	u16 adr_outer = SPC700_readFromMem(adr_inner + SPC700_regs.Y);
	return adr_outer;
}

std::string mStr() {
	//Nvpbhizc
	std::string s = "";
	if (SPC700_regs.PSW.N)
		s += "N";
	else
		s += "n";
	if (SPC700_regs.PSW.V)
		s += "V";
	else
		s += "v";
	if (SPC700_regs.PSW.zeropage >> 2)
		s += "P";
	else
		s += "p";
	if (SPC700_regs.PSW.B)
		s += "B";
	else
		s += "b";
	if (SPC700_regs.PSW.H)
		s += "H";
	else
		s += "h";
	if (SPC700_regs.PSW.I)
		s += "I";
	else
		s += "i";
	if (SPC700_regs.PSW.Z)
		s += "Z";
	else
		s += "z";
	if (SPC700_regs.PSW.C)
		s += "C";
	else
		s += "c";
	return s;
}

u8 SPC700_TICK() {
	//printf("%04x op: %02x %02x %02x A:%02x X:%02x Y:%02x SP:%04x YA:%04x %s\n", SPC700_regs.PC, SPC700_readFromMem(SPC700_regs.PC), SPC700_readFromMem(SPC700_regs.PC+1), SPC700_readFromMem(SPC700_regs.PC + 2), SPC700_regs.A, SPC700_regs.X, SPC700_regs.Y, SPC700_regs.SP, (SPC700_regs.Y << 8) | SPC700_regs.A, mStr().c_str());
	//printf("%04x A:%02x X:%02x Y:%02x SP:%04x YA:%04x %s\n", SPC700_regs.PC, SPC700_regs.A, SPC700_regs.X, SPC700_regs.Y, SPC700_regs.SP, (SPC700_regs.Y << 8) | SPC700_regs.A, mStr().c_str());
	switch (SPC700_readFromMem(SPC700_regs.PC++))
	{
	case 0x00: SPC700_regs.PC++; return 2; break;
	case 0x01: TCALL(0); return 8; break;
	case 0x02: SET(ADDR_immediate8, 0); return 4; break;
	case 0x03: return BBS(0); break;
	case 0x04: OR_A(ADDR_immediate8); return 3; break;
	case 0x05: OR_A(ADDR_immediate16); return 4; break;
	case 0x06: OR_A(ADDR_X); return 3; break;
	case 0x07: OR_A(ADDR_indirectX); return 6; break;
	case 0x08: OR_A(ADDR_PC); return 2; break;
	case 0x09: OR_n(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 6; break;
	case 0x0a: OR(); return 5; break;
	case 0x0b: ASL_n(ADDR_immediate8); return 4; break;
	case 0x0c: ASL_n(ADDR_immediate16); return 5; break;
	case 0x0d: PUSH(SPC700_regs.PSW.getByte()); return 4; break;
	case 0x0e: TSET(); return 6; break;
	case 0x0f: BRK(); return 8; break;
	case 0x10: return BPL(); break;
	case 0x11: TCALL(1); return 8; break;
	case 0x12: CLR(ADDR_immediate8, 0); return 4; break;
	case 0x13: return BBC(0); break;
	case 0x14: OR_A(ADDR_immediate8X); return 4; break;
	case 0x15: OR_A(ADDR_immediate16X); return 5; break;
	case 0x16: OR_A(ADDR_immediate16Y); return 5; break;
	case 0x17: OR_A(ADDR_indirectY); return 6; break;
	case 0x18: OR_n(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0x19: OR_n(ADDR_X, SPC700_readFromMem(ADDR_Y())); return 5; break;
	case 0x1a: DECW(ADDR_immediate8); return 6; break;
	case 0x1b: ASL_n(ADDR_immediate8X); return 5; break;
	case 0x1c: ASL_A(); return 2; break;
	case 0x1d: DEC_X(); return 2; break;
	case 0x1e: CMP_X(ADDR_immediate16); return 4; break;
	case 0x1f: JMP_X(); return 6; break;
	case 0x20: CLRP(); return 2; break;
	case 0x21: TCALL(2); return 8; break;
	case 0x22: SET(ADDR_immediate8, 1); return 4; break;
	case 0x23: return BBS(1); break;
	case 0x24: AND_A(ADDR_immediate8); return 3; break;
	case 0x25: AND_A(ADDR_immediate16); return 4; break;
	case 0x26: AND_A(ADDR_X); return 3; break;
	case 0x27: AND_A(ADDR_indirectX); return 6; break;
	case 0x28: AND_A(ADDR_PC); return 2; break;
	case 0x29: AND_n(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 6; break;
	case 0x2a: OR_NOT(); return 5; break;
	case 0x2b: ROL_n(ADDR_immediate8); return 4; break;
	case 0x2c: ROL_n(ADDR_immediate16); return 5; break;
	case 0x2d: PUSH(SPC700_regs.A); return 4; break;
	case 0x2e: return CBNE(ADDR_immediate8); break;
	case 0x2f: BRA(); return 4; break;
	case 0x30: return BMI(); break;
	case 0x31: TCALL(3); return 8; break;
	case 0x32: CLR(ADDR_immediate8, 1); return 4; break;
	case 0x33: return BBC(1); break;
	case 0x34: AND_A(ADDR_immediate8X); return 4; break;
	case 0x35: AND_A(ADDR_immediate16X); return 5; break;
	case 0x36: AND_A(ADDR_immediate16Y); return 5; break;
	case 0x37: AND_A(ADDR_indirectY); return 6; break;
	case 0x38: AND_n(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0x39: AND_n(ADDR_X, SPC700_readFromMem(ADDR_Y())); return 5; break;
	case 0x3a: INCW(ADDR_immediate8); return 6; break;
	case 0x3b: ROL_n(ADDR_immediate8X); return 5; break;
	case 0x3c: ROL_A(); return 2; break;
	case 0x3d: INC_X(); return 2; break;
	case 0x3e: CMP_X(ADDR_immediate8); return 3; break;
	case 0x3f: CALL(); return 8; break;
	case 0x40: SETP(); return 2; break;
	case 0x41: TCALL(4); return 8; break;
	case 0x42: SET(ADDR_immediate8, 2); return 4; break;
	case 0x43: return BBS(2); break;
	case 0x44: XOR_A(ADDR_immediate8); return 3; break;
	case 0x45: XOR_A(ADDR_immediate16); return 4; break;
	case 0x46: XOR_A(ADDR_X); return 3; break;
	case 0x47: XOR_A(ADDR_indirectX); return 6; break;
	case 0x48: XOR_A(ADDR_PC); return 2; break;
	case 0x49: XOR_n(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 6; break;
	case 0x4a: AND(); return 4; break;
	case 0x4b: LSR_n(ADDR_immediate8); return 4; break;
	case 0x4c: LSR_n(ADDR_immediate16); return 5; break;
	case 0x4d: PUSH(SPC700_regs.X); return 4; break;
	case 0x4e: TCLR(); return 6; break;
	case 0x4f: PCALL(); return 6; break;
	case 0x50: return BVC(); break;
	case 0x51: TCALL(5); return 8; break;
	case 0x52: CLR(ADDR_immediate8, 2); return 4; break;
	case 0x53: return BBC(2); break;
	case 0x54: XOR_A(ADDR_immediate8X); return 4; break;
	case 0x55: XOR_A(ADDR_immediate16X); return 5; break;
	case 0x56: XOR_A(ADDR_immediate16Y); return 5; break;
	case 0x57: XOR_A(ADDR_indirectY); return 6; break;
	case 0x58: XOR_n(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0x59: XOR_n(ADDR_X, SPC700_readFromMem(ADDR_Y())); return 5; break;
	case 0x5a: CMPW(ADDR_immediate8); return 4; break;
	case 0x5b: LSR_n(ADDR_immediate8X); return 5; break;
	case 0x5c: LSR_A(); return 2; break;
	case 0x5d: MOV_X(SPC700_regs.A); return 2; break;
	case 0x5e: CMP_Y(ADDR_immediate16); return 4; break;
	case 0x5f: JMP(); return 3; break;
	case 0x60: CLRC(); return 2; break;
	case 0x61: TCALL(6); return 8; break;
	case 0x62: SET(ADDR_immediate8, 3); return 4; break;
	case 0x63: return BBS(3); break;
	case 0x64: CMP_A(ADDR_immediate8); return 3; break;
	case 0x65: CMP_A(ADDR_immediate16); return 4; break;
	case 0x66: CMP_A(ADDR_X); return 3; break;
	case 0x67: CMP_A(ADDR_indirectX); return 6; break;
	case 0x68: CMP_A(ADDR_PC); return 2; break;
	case 0x69: CMP_n(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 6; break;
	case 0x6a: AND_NOT(); return 4; break;
	case 0x6b: ROR_n(ADDR_immediate8); return 4; break;
	case 0x6c: ROR_n(ADDR_immediate16); return 5; break;
	case 0x6d: PUSH(SPC700_regs.Y); return 4; break;
	case 0x6e: return DBNZ_n(); break;
	case 0x6f: RET(); return 5; break;
	case 0x70: return BVS(); break;
	case 0x71: TCALL(7); return 8; break;
	case 0x72: CLR(ADDR_immediate8, 3); return 4; break;
	case 0x73: return BBC(3); break;
	case 0x74: CMP_A(ADDR_immediate8X); return 4; break;
	case 0x75: CMP_A(ADDR_immediate16X); return 5; break;
	case 0x76: CMP_A(ADDR_immediate16Y); return 5; break;
	case 0x77: CMP_A(ADDR_indirectY); return 6; break;
	case 0x78: CMP_n(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0x79: CMP_n(ADDR_X, SPC700_readFromMem(ADDR_Y())); return 5; break;
	case 0x7a: ADDW(ADDR_immediate8); return 5; break;
	case 0x7b: ROR_n(ADDR_immediate8X); return 5; break;
	case 0x7c: ROR_A(); return 2; break;
	case 0x7d: MOV_A(SPC700_regs.X); return 2; break;
	case 0x7e: CMP_Y(ADDR_immediate8); return 3; break;
	case 0x7f: RETI(); return 6; break;
	case 0x80: SETC(); return 2; break;
	case 0x81: TCALL(8); return 8; break;
	case 0x82: SET(ADDR_immediate8, 4); return 4; break;
	case 0x83: return BBS(4); break;
	case 0x84: ADC(ADDR_immediate8); return 3; break;
	case 0x85: ADC(ADDR_immediate16); return 4; break;
	case 0x86: ADC(ADDR_X); return 3; break;
	case 0x87: ADC(ADDR_indirectX); return 6; break;
	case 0x88: ADC(ADDR_PC); return 2; break;
	case 0x89: ADC(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 6; break;
	case 0x8a: XOR(); return 5; break;
	case 0x8b: DEC_n(ADDR_immediate8); return 4; break;
	case 0x8c: DEC_n(ADDR_immediate16); return 5; break;
	case 0x8d: MOV_Y(ADDR_PC); return 2; break;
	case 0x8e: POP_SP(); return 4; break;
	case 0x8f: MOV_nn_WITH_READ(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0x90: return BCC(); break;
	case 0x91: TCALL(9); return 8; break;
	case 0x92: CLR(ADDR_immediate8, 4); return 4; break;
	case 0x93: return BBC(4); break;
	case 0x94: ADC(ADDR_immediate8X); return 4; break;
	case 0x95: ADC(ADDR_immediate16X); return 5; break;
	case 0x96: ADC(ADDR_immediate16Y); return 5; break;
	case 0x97: ADC(ADDR_indirectY); return 6; break;
	case 0x98: ADC(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0x99: ADC(ADDR_X, SPC700_readFromMem(ADDR_Y())); return 5; break;
	case 0x9a: SUBW(ADDR_immediate8); return 5; break;
	case 0x9b: DEC_n(ADDR_immediate8X); return 5; break;
	case 0x9c: DEC_A(); return 2; break;
	case 0x9d: MOV_X((u8)SPC700_regs.SP); return 2; break;
	case 0x9e: DIV(); return 12; break;
	case 0x9f: XCN(); return 5; break;
	case 0xa0: EI(); return 3; break;
	case 0xa1: TCALL(10); return 8; break;
	case 0xa2: SET(ADDR_immediate8, 5); return 4; break;
	case 0xa3: return BBS(5); break;
	case 0xa4: SBC(ADDR_immediate8); return 3; break;
	case 0xa5: SBC(ADDR_immediate16); return 4; break;
	case 0xa6: SBC(ADDR_X); return 3; break;
	case 0xa7: SBC(ADDR_indirectX); return 6; break;
	case 0xa8: SBC(ADDR_PC); return 2; break;
	case 0xa9: SBC(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 6; break;
	case 0xaa: MOV_TO_C(); return 4; break;
	case 0xab: INC_n(ADDR_immediate8); return 4; break;
	case 0xac: INC_n(ADDR_immediate16); return 5; break;
	case 0xad: CMP_Y(ADDR_PC); return 2; break;
	case 0xae: POP(SPC700_regs.A); return 4; break;
	case 0xaf: MOV_nn_NO_READ(ADDR_X_INC, SPC700_regs.A); return 4; break;
	case 0xb0: return BCS(); break;
	case 0xb1: TCALL(11); return 8; break;
	case 0xb2: CLR(ADDR_immediate8, 5); return 4; break;
	case 0xb3: return BBC(5); break;
	case 0xb4: SBC(ADDR_immediate8X); return 4; break;
	case 0xb5: SBC(ADDR_immediate16X); return 5; break;
	case 0xb6: SBC(ADDR_immediate16Y); return 5; break;
	case 0xb7: SBC(ADDR_indirectY); return 6; break;
	case 0xb8: SBC(ADDR_immediate8, SPC700_readFromMem(SPC700_regs.PC++)); return 5; break;
	case 0xb9: SBC(ADDR_X, SPC700_readFromMem(ADDR_Y())); return 5; break;
	case 0xba: MOVW_YA_nn(); return 5; break;
	case 0xbb: INC_n(ADDR_immediate8X); return 5; break;
	case 0xbc: INC_A(); return 2; break;
	case 0xbd: SPC700_regs.SP = 0x100 | SPC700_regs.X; return 2; break;
	case 0xbe: DAS(); return 3; break;
	case 0xbf: MOV_A(ADDR_X_INC); return 4; break;
	case 0xc0: DI(); return 3; break;
	case 0xc1: TCALL(12); return 8; break;
	case 0xc2: SET(ADDR_immediate8, 6); return 4; break;
	case 0xc3: return BBS(6); break;
	case 0xc4: MOV_nn_WITH_READ(ADDR_immediate8, SPC700_regs.A); return 4; break;
	case 0xc5: MOV_nn_WITH_READ(ADDR_immediate16, SPC700_regs.A); return 5; break;
	case 0xc6: MOV_nn_WITH_READ(ADDR_X, SPC700_regs.A); return 4; break;
	case 0xc7: MOV_nn_WITH_READ(ADDR_indirectX, SPC700_regs.A); return 7; break;
	case 0xc8: CMP_X(ADDR_PC); return 2; break;
	case 0xc9: MOV_nn_WITH_READ(ADDR_immediate16, SPC700_regs.X); return 5; break;
	case 0xca: MOV_FROM_C(); return 6; break;
	case 0xcb: MOV_nn_WITH_READ(ADDR_immediate8, SPC700_regs.Y); return 4; break;
	case 0xcc: MOV_nn_WITH_READ(ADDR_immediate16, SPC700_regs.Y); return 5; break;
	case 0xcd: MOV_X(ADDR_PC); return 2; break;
	case 0xce: POP(SPC700_regs.X); return 4; break;
	case 0xcf: MUL(); return 9; break;
	case 0xd0: return BNE(); break;
	case 0xd1: TCALL(13); return 8; break;
	case 0xd2: CLR(ADDR_immediate8, 6); return 4; break;
	case 0xd3: return BBC(6); break;
	case 0xd4: MOV_nn_WITH_READ(ADDR_immediate8X, SPC700_regs.A); return 5; break;
	case 0xd5: MOV_nn_WITH_READ(ADDR_immediate16X, SPC700_regs.A); return 6; break;
	case 0xd6: MOV_nn_WITH_READ(ADDR_immediate16Y, SPC700_regs.A); return 6; break;
	case 0xd7: MOV_nn_WITH_READ(ADDR_indirectY, SPC700_regs.A); return 7; break;
	case 0xd8: MOV_nn_WITH_READ(ADDR_immediate8, SPC700_regs.X); return 4; break;
	case 0xd9: MOV_nn_WITH_READ(ADDR_immediate8Y, SPC700_regs.X); return 5; break;
	case 0xda: MOVW_nn_YA(); return 5; break;
	case 0xdb: MOV_nn_WITH_READ(ADDR_immediate8X, SPC700_regs.Y); return 5; break;
	case 0xdc: DEC_Y(); return 2; break;
	case 0xdd: MOV_A(SPC700_regs.Y); return 2; break;
	case 0xde: return CBNE(ADDR_immediate8X); break;
	case 0xdf: DAA(); return 3; break;
	case 0xe0: CLRV(); return 2; break;
	case 0xe1: TCALL(14); return 8; break;
	case 0xe2: SET(ADDR_immediate8, 7); return 4; break;
	case 0xe3: return BBS(7); break;
	case 0xe4: MOV_A(ADDR_immediate8); return 3; break;
	case 0xe5: MOV_A(ADDR_immediate16); return 4; break;
	case 0xe6: MOV_A(ADDR_X()); return 3; break;
	case 0xe7: MOV_A(ADDR_indirectX); return 6; break;
	case 0xe8: MOV_A(ADDR_PC); return 2; break;
	case 0xe9: MOV_X(ADDR_immediate16); return 4; break;
	case 0xea: NOT(); return 5; break;
	case 0xeb: MOV_Y(ADDR_immediate8); return 3; break;
	case 0xec: MOV_Y(ADDR_immediate16); return 4; break;
	case 0xed: NOTC(); return 3; break;
	case 0xee: POP(SPC700_regs.Y); return 4; break;
	case 0xef: printf("SLEEP\n"); return 1; break;
	case 0xf0: return BEQ(); break;
	case 0xf1: TCALL(15); return 8; break;
	case 0xf2: CLR(ADDR_immediate8, 7); return 4; break;
	case 0xf3: return BBC(7); break;
	case 0xf4: MOV_A(ADDR_immediate8X); return 4; break;
	case 0xf5: MOV_A(ADDR_immediate16X); return 5; break;
	case 0xf6: MOV_A(ADDR_immediate16Y); return 5; break;
	case 0xf7: MOV_A(ADDR_indirectY); return 6; break;
	case 0xf8: MOV_X(ADDR_immediate8); return 3; break;
	case 0xf9: MOV_X(ADDR_immediate8Y); return 4; break;
	case 0xfa: MOV_nn_NO_READ(ADDR_immediate8, SPC700_readFromMem(ADDR_immediate8())); return 5; break;
	case 0xfb: MOV_Y(ADDR_immediate8X); return 4; break;
	case 0xfc: INC_Y(); return 2; break;
	case 0xfd: MOV_Y(SPC700_regs.A); return 2; break;
	case 0xfe: return DBNZ_Y(); break;
	case 0xff: printf("STOP\n"); return 1; break;
	default:
		break;
	}
}