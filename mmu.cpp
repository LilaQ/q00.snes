#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"

void powerUp() {
	//	TODO
}

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

uint8_t readFromMem(uint16_t adr) {
	switch (adr)
	{
		default:
			return 0xff;
			break;
	}
}

void writeToMem(uint16_t adr, uint8_t val) {

	switch (adr) {
	default:
		break;
	}

}