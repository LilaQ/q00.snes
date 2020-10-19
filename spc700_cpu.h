#ifndef LIB_CPU
#define LIB_CPU

#include <stdint.h>
#include <stdio.h>

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
		bool P = false;			//	Zero Page Location (0 = 00xxh, 1 = 01xxh)
	public:
		bool N = false;			//	Negative
		bool V = false;			//	Overflow
		bool B = false;			//	Break Flag (0 = Reset, 1 = BRK opcode; set <after> BRK opcode)
		bool H = false;			//	Half-carry
		bool I = true;			//	IRQ Enable (0 = Disable, 1 = Enable) [no function in SNES APU]
		bool Z = false;			//	Zero
		bool C = false;			//	Carry
		u16 zeropage = 0x0000;

		u8 getByte() {
			return  (N << 7) | (V << 6) | (P << 5) | (B << 4) | (H << 3) | (I << 2) | (Z << 1) | (u8)C;
		}
		void setByte(u8 val) {
			C = val & 1;
			Z = (val >> 1) & 1;
			I = (val >> 2) & 1;
			H = (val >> 3) & 1;
			B = (val >> 4) & 1;
			P = (val >> 5) & 1;
			V = (val >> 6) & 1;
			N = (val >> 7) & 1;
		}
	};

public:

	u8 A;			//	Accumulator
	u8 X;			//	X-Register
	u8 Y;			//	Y-Register
	u16 SP;			//	Stack Pointer
	u16 YA;			//	Y-MSB, A-LSB
	u16 PC;			//	Program Counter
	Status PSW;		//	Program Status
};


u8 CPU_step();
void resetCPU();
void togglePause();
u16 CPU_getPC();
bool CPU_isStopped();

#endif // !LIB_CPU