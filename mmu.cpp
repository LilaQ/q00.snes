#define _CRT_SECURE_NO_DEPRECATE
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>
#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "apu.h"
#include "input.h"
using namespace::std;

typedef uint8_t		u8;
typedef uint16_t	u16;

u8 pbc = 0;

Cartridge cartridge;
vector<u8> memory(0xffffff);

void reset() {
	resetCPU( cartridge.emu_reset_vector );
}

std::vector<u8> readFile(const char* filename)
{
	// open the file:
	std::ifstream file(filename, std::ios::binary);

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	std::vector<u8> vec;
	vec.reserve(fileSize);

	// read the data:
	vec.insert(vec.begin(),
		istream_iterator<u8>(file),
		istream_iterator<u8>());

	return vec;
}

//	copy cartridge to memory
void loadROM(string filename) {

	//	load cartridge to memory
	memory = readFile(filename.c_str());

	//	identify cartridge & post data to variable
	//	TODO
	uint16_t base_snes_header = 0xffff;
	bool isLoROM = true;
	if (isLoROM) {
		base_snes_header = 0x8000;
	}
	u8 header[0x50];
	for (u8 i = 0; i < 0x50; i++) {
		header[i] = memory[base_snes_header - 0x50 + i];
	}
	cartridge.initSNESHeader(header);
	
	cout << "Loaded '" << filename << "' - " << (memory.size() / 1024) << " kbytes..\n";
	cout << "------------------------------------------------------\n";
	cout << "SNES Header version:\t" << cartridge.getHeaderVersionString() << "\n";
	cout << "ROM Name:\t\t" << cartridge.getTitleString() << "\n";
	cout << "Region:\t\t\t" << cartridge.getRegionString() << "\n";
	cout << "GameCode:\t\t" << cartridge.getGameCodeString() << "\n";
	cout << "ROM speed:\t\t" << ((cartridge.isFastROM) ? "FastROM (3.58 MHz)" : "SlowROM (2.68 MHz)") << "\n";
	cout << "ROM type:\t\t" << (cartridge.isHiROM ? "HiROM" : "LoROM") << "\n";
	cout << "ROM size:\t\t" << cartridge.getRAMSizeString() << "\n";
	cout << "SRAM Size:\t\t" << cartridge.getRAMSizeString() << "\n";
	cout << "ROM chipset:\t\t" << cartridge.getROMChipsetString() << "\n";
	cout << "ROM coprocessor:\t" << cartridge.getROMCoprocessorString() << "\n";
	cout << "Version:\t\t" << cartridge.getVersionString() << "\n";
	cout << "Checksum:\t\t" << cartridge.getChecksumString() << "\n";
	cout << "Checksum complement:\t" << cartridge.getChecksumComplementString() << "\n";
 	cout << "Checksum okay? \t\t" << cartridge.getChecksumOkay() << "\n";
	cout << "Dev-ID:\t\t\t" << cartridge.getDevIDString() << "\n";
	cout << "Flash size:\t\t" << cartridge.getFlashSizeString() << "\n";
	cout << "ExpRAM size:\t\t" << cartridge.getExpansionRAMString() << "\n\n";

	resetCPU( cartridge.emu_reset_vector );
}

uint8_t readFromMem(u16 adr, u8 bank_nr) {
	switch ((bank_nr << 16) | adr)
	{
		default:
			return 0xff;
			break;
	}
}

void writeToMem(u8 val, u16 adr, u8 bank_nr) {

	switch ((bank_nr << 16) | adr) {
	default:
		break;
	}

}

bool pageBoundaryCrossed() {
	return pbc;
}

//	addressing modes
uint16_t a;
uint16_t getImmediate(uint16_t adr) {
	return adr;
}
uint16_t getAbsolute(uint16_t adr) {
	return (memory[adr + 1] << 8) | memory[adr];
}
uint16_t getAbsoluteXIndex(uint16_t adr, uint8_t X) {
	a = ((memory[adr + 1] << 8) | memory[adr]);
	pbc = (a & 0xff00) != ((a + X) & 0xff00);
	return (a + X) % 0x10000;
}
uint16_t getAbsoluteYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[adr + 1] << 8) | memory[adr]);
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}
uint16_t getZeropage(uint16_t adr) {
	return memory[adr];
}
uint16_t getZeropageXIndex(uint16_t adr, uint8_t X) {
	return (memory[adr] + X) % 0x100;
}
uint16_t getZeropageYIndex(uint16_t adr, uint8_t Y) {
	return (memory[adr] + Y) % 0x100;
}
uint16_t getIndirect(uint16_t adr) {
	return (memory[memory[adr + 1] << 8 | ((memory[adr] + 1) % 0x100)]) << 8 | memory[memory[adr + 1] << 8 | memory[adr]];
}
uint16_t getIndirectXIndex(uint16_t adr, uint8_t X) {
	a = (memory[(memory[adr] + X + 1) % 0x100] << 8) | memory[(memory[adr] + X) % 0x100];
	return a % 0x10000;
}
uint16_t getIndirectYIndex(uint16_t adr, uint8_t Y) {
	a = ((memory[(memory[adr] + 1) % 0x100] << 8) | (memory[memory[adr]]));
	pbc = (a & 0xff00) != ((a + Y) & 0xff00);
	return (a + Y) % 0x10000;
}