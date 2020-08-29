#pragma once
#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint16_t u16;

struct Status
{

	u8 N;	//	Negative
	u8 V;	//	Overflow
	u8 M;	//	Accumulator register size	-	0 = 16-bit, 1 = 8-bit	(only native mode)
	u8 X;	//	Index register size			-	0 = 16-bit, 1 = 8-bit	(only native mode)
	u8 D;	//	Decimal
	u8 I;	//	IRQ Disable
	u8 Z;	//	Zero
	u8 C;	//	Carry
	u8 E;	//	6502 Emulation mode
	u8 B;	//	Break (emulation mode only)

};

struct Registers
{

	private: 
		u8 A_lo;	//	Accumulator - low-byte
		u8 A_hi;	//	Accumulator - high-byte
		u8 DBR;		//	Data Bank Register
		u16 D;		//	Direct Page Register
		u8 PB;		//	Program Bank Register
		Status P;	//	Program Bank
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
};


int stepCPU();
void resetCPU();
void togglePause();