#pragma once
#ifndef LIB_SPC700
#define LIB_SPC700

#include <stdint.h>
#include <stdio.h>
#include "spc700_bus.h"
#include <string>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef int8_t		i8;

struct SPC700_Registers
{
private:

	struct SPC700_Status
	{
	public:
		bool N = false;			//	Negative
		bool V = false;			//	Overflow
		bool B = false;			//	Break Flag (0 = Reset, 1 = BRK opcode; set <after> BRK opcode)
		bool H = false;			//	Half-carry
		bool I = false;			//	IRQ Enable (0 = Disable, 1 = Enable) [no function in SNES APU]
		bool Z = true;			//	Zero
		bool C = false;			//	Carry
		u16 zeropage = 0x0000;

		u8 getByte() {
			return  (N << 7) | (V << 6) | ((zeropage >> 2) << 5) | (B << 4) | (H << 3) | (I << 2) | (Z << 1) | (u8)C;
		}
		void setByte(u8 val) {
			C = val & 1;
			Z = (val >> 1) & 1;
			I = (val >> 2) & 1;
			H = (val >> 3) & 1;
			B = (val >> 4) & 1;
			zeropage = ((val >> 5) & 1) << 2;
			V = (val >> 6) & 1;
			N = (val >> 7) & 1;
		}
	};

public:

	u8 A;					//	Accumulator
	u8 X;					//	X-Register
	u8 Y;					//	Y-Register
	u16 SP = 0x01ef;		//	Stack Pointer
	u16 YA;					//	Y-MSB, A-LSB
	u16 PC = 0xFFC0;		//	Program Counter
	SPC700_Status PSW;		//	Program Status
};

void SPC700_RESET();
u8 SPC700_TICK();

#endif // !LIB_SPC700