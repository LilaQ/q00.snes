#pragma once
#include <stdint.h>
#include <stdio.h>
#include "mmu.h"

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

struct Status;

struct Registers
{
	private:

		struct Status
		{
			private:
				bool N;			//	Negative
				bool V;			//	Overflow
				bool M;			//	Memory / Accumulator Select register size	-	0 = 16-bit, 1 = 8-bit	(only native mode)
				bool X;			//	Index register Select size					-	0 = 16-bit, 1 = 8-bit	(only native mode)
				bool D;			//	Decimal
				bool I = true;	//	IRQ Disable
				bool Z;			//	Zero
				bool C;			//	Carry

				//	side bits
				bool B = true;	//	Break (emulation mode only)
				bool E = true;	//	6502 Emulation mode	

			public:
				u8 getByte() {
					//	NOT emulation mode
					if (!E) {
						return (N << 7) | (V << 6) | (M << 5) | (X << 4) | (D << 3) | (I << 2) | (Z << 1) | C;
					}
					//	emulation mode
					return  (N << 7) | (V << 6) | (1 << 5) | (B << 4) | (D << 3) | (I << 2) | (Z << 1) | C;
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
				void setIndexSize(u8 val, Registers* regs) {
					//	high bytes get reset on change from 8-bit to 16-bit or vice versa
					X = val & 1;
					if (X) {
						regs->clearXYhighBytesOnModeChange();
					}
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

				//	helper, to reduce clutter
				bool isMReset() {
					return M == 0;
				}
				bool isXReset() {
					return X == 0;
				}
		};

		u8 A_lo;	//	Accumulator - low-byte
		u8 A_hi;	//	Accumulator - high-byte
		u8 DBR;		//	Data Bank Register
		u16 D;		//	Direct Page Register
		u8 PB;		//	Program Bank Register
		u8 X_lo;	//	index X - low byte
		u8 X_hi;	//	index X - high byte
		u8 Y_lo;	//	index Y - low byte
		u8 Y_hi;	//	index Y - high byte
		u16 SP;		//	Stack Pointer

	public:

		Status P;	//	Program Status
		u16 PC;		//	Program Counter

		//	8-bit / 16-bit wide Accumulator templates (getter / setter)
		void setAccumulator(u16 val) {
			A_lo = val & 0xff;
			A_hi = val >> 8;
		}
		void setAccumulator(u8 val) {
			A_lo = val;
		}
		u16 getAccumulator() {
			//if (!P.getAccuMemSize()) {
				return (A_hi << 8) | A_lo;
			//}
			//return A_lo;
		}
		//	8-bit / 16-bit wide X-Index templates (getter / setter)
		void setX(u16 val) {
			X_lo = val & 0xff;
			X_hi = val >> 8;
		}
		void setX(u8 val) {
			X_lo = val;
		}
		u16 getX() {
			if (!P.getIndexSize()) {
				return (X_hi << 8) | X_lo;
			}
			return X_lo;
		}
		//	8-bit / 16-bit wide Y-Index templates (getter / setter)
		void setY(u16 val) {
			Y_lo = val & 0xff;
			Y_hi = val >> 8;
		}
		void setY(u8 val) {
			Y_lo = val;
		}
		u16 getY() {
			if (!P.getIndexSize()) {
				return (Y_hi << 8) | Y_lo;
			}
			return Y_lo;
		}
		//	8-bit / 16-bit wide Y-Index templates (getter / setter)
		template <typename T>
		void setSP(T val) {
			SP = val;
		}
		u16 getSP() {
			if (P.getEmulation()) {
				return 0x100 | (SP & 0xff);
			}
			return SP;
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
		void setProgramBankRegister(u8 val) {
			PB = val;
		}
		void setDataBankRegister(u8 val) {
			DBR = val;
		}
		void setDirectPageRegister(u16 val) {
			D = val;
		}
		void resetStackPointer() {
			if (P.getEmulation()) {
				SP = 0x1ff;
			}
			else {
				SP = 0x1ff;
			}
		}
		bool isDPLowNotZero() {
			return (D & 0xff) != 0x00;
		}
};



u8 stepCPU();
void resetCPU(u16 reset_vector);
void togglePause();