#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include <map>

//	set up vars
uint16_t PC = 0x0000;
uint8_t SP_ = 0x0000;
Registers registers;
Status status;

void resetCPU() {
	PC = readFromMem(0xfffd) << 8 | readFromMem(0xfffc);
	printf("Reset CPU, starting at PC: %x\n", PC);
}

int stepCPU() {

	switch (readFromMem(PC)) {
		default:
			printf("ERROR! Unimplemented opcode 0x%02x at address 0x%04x !\n", readFromMem(PC), PC);
			std::exit(1);
			break;
	}
	
	return 0;
}