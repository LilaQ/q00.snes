#pragma once
#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;

struct Status
{
	private:
		bool N;	//	Negative
		bool V;	//	Overflow
		bool M;	//	Memory / Accumulator Select register size	-	0 = 16-bit, 1 = 8-bit	(only native mode)
		bool X;	//	Index register Select size					-	0 = 16-bit, 1 = 8-bit	(only native mode)
		bool D;	//	Decimal
		bool I;	//	IRQ Disable
		bool Z;	//	Zero
		bool C;	//	Carry

		//	side bits
		bool B;	//	Break (emulation mode only)
		bool E;	//	6502 Emulation mode	

	public:
		u8 getByte() {
			(N << 7) | (V << 6) | (M << 5) | (X << 4) | (D << 3) | (I << 2) | (Z << 1) | C;
		}
		bool getNegative() {
			return N;
		}
		bool getOverflow() {
			return V;
		}
		bool getAccuMemSize() {
			return M;
		}
		bool getIndexSize() {
			return X;
		}
		bool getDecimal() {
			return D;
		}
		bool getIRQDisable() {
			return I;
		}
		bool getZero() {
			return Z;
		}
		bool getCarry() {
			return C;
		}
		bool getBreak() {
			return B;
		}
		bool getEmulation() {
			return E;
		}
		void setByte(u8 val) {
			C = val & 1;
			Z = (val >> 1) & 1;
			I = (val >> 2) & 1;
			D = (val >> 3) & 1;
			X = (val >> 4) & 1;
			M = (val >> 5) & 1;
			V = (val >> 6) & 1;
			N = (val >> 7) & 1;
		}
		void setNegative(u8 val) {
			N = val & 1;
		}
		void setOverflow(u8 val) {
			V = val & 1;
		}
		void setAccuMemSize(u8 val) {
			M = val & 1;
		}
		void setIndexSize(u8 val, Registers &regs) {
			//	high bytes get reset on change from 8-bit to 16-bit or vice versa
			X = val & 1;
			regs.clearXYhighBytesOnModeChange();
		}
		void setDecimal(u8 val) {
			D = val & 1;
		}
		void setIRQDisable(u8 val) {
			I = val & 1;
		}
		void setZero(u8 val) {
			Z = val & 1;
		}
		void setCarry(u8 val) {
			C = val & 1;
		}
		void setBreak(u8 val) {
			B = val & 1;
		}
		void setEmulation(u8 val) {
			E = val & 1;
		}

};

struct Registers
{

	private: 
		u8 A_lo;	//	Accumulator - low-byte
		u8 A_hi;	//	Accumulator - high-byte
		u8 DBR;		//	Data Bank Register
		u16 D;		//	Direct Page Register
		u8 PB;		//	Program Bank Register
		Status P;	//	Program Status
		u16 SP;		//	Stack Pointer
		u16 X_lo;	//	index X - low byte
		u16 X_hi;	//	index X - high byte
		u16 Y_lo;	//	index Y - low byte
		u16 Y_hi;	//	index Y - high byte
		u16 PC;		//	Program Counter

	public:
		//	8-bit / 16-bit wide Accumulator templates (getter / setter)
		template <typename T>
		void setAccumulator(T val) {
			A_lo = val & 0xff;
			A_hi = val >> 8;
		}
		template <>
		void setAccumulator(u8 val) {
			A_lo = val;
		}
		template <typename T> T getAccumulator() {
			if (!P.M) {
				return (A_hi << 8) | A_lo;
			}
			return A_lo;
		}
		//	8-bit / 16-bit wide X-Index templates (getter / setter)
		template <typename T>
		void setX(T val) {
			X_lo = val & 0xff;
			X_hi = val >> 8;
		}
		template <>
		void setX(u8 val) {
			X_lo = val;
		}
		template <typename T> T getX() {
			if (!P.M) {
				return (X_hi << 8) | X_lo;
			}
			return X_lo;
		}
		//	8-bit / 16-bit wide Y-Index templates (getter / setter)
		template <typename T>
		void setY(T val) {
			Y_lo = val & 0xff;
			Y_hi = val >> 8;
		}
		template <>
		void setY(u8 val) {
			Y_lo = val;
		}
		template <typename T> T getY() {
			if (!P.M) {
				return (Y_hi << 8) | Y_lo;
			}
			return Y_lo;
		}
		//	reset the high bytes of X and Y registers when the sizes of XY got changed from 8-bit to 16-bit or vice versa
		void clearXYhighBytesOnModeChange() {	
			Y_hi = 0x00;
			X_hi = 0x00;
		}
		u8 getProgramBankRegister() {
			if (P.getEmulation()) {
				return 0x00;
			}
			return PB;
		}
		u8 getDataBankRegister() {
			if (P.getEmulation()) {
				return 0x00;
			}
			return DBR;
		}
		u16 getDirectPageRegister() {
			return D;
		}
		u16 getStackPointer() {
			return SP;
		}
		u16 getProgramCounter() {
			return PC;
		}
		Status getStatus() {
			return P;
		}
		void setProgramBankRegister(u8 val) {
			PB = val;
		}
		void setDataBankRegister(u8 val) {
			DBR = val;
		}
		void setDirectPageRegister(u8 val) {
			D = val;
		}
		void setStackPointer(u16 val) {
			SP = val;
		}
		void setProgramCounter(u16 val) {
			PC = val;
		}
};


int stepCPU();
void resetCPU();
void togglePause();