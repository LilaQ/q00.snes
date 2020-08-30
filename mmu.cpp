#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"

typedef uint8_t		u8;
typedef uint16_t	u16;

u8 memory[0xffffff];

void reset() {
	
}

//	copy cartridge to memory
void loadROM(string filename) {

	unsigned char cartridge[0xf0000];
	FILE* file = fopen(filename.c_str(), "rb");
	int pos = 0;
	while (fread(&cartridge[pos], 1, 1, file)) {
		pos++;
	}
	fclose(file);

	resetCPU();
}

uint8_t readFromMem(u16 adr, u8 data_bank = 0) {
	switch (adr)
	{
		default:
			return 0xff;
			break;
	}
}

void writeToMem(u8 val, u16 adr, u8 data_bank = 0) {

	switch (adr) {
	default:
		break;
	}

}