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
vector<u8> cartridge_memory;

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
	cartridge_memory = readFile(filename.c_str());

	//	identify cartridge & post data to variable
	//	TODO
	u16 base_snes_header = 0xffff;
	bool isLoROM = true;
	bool fileHasHeader = (cartridge_memory.size() & 0x3ff) == 0x200;
	u16 filesizeInKb = ((cartridge_memory.size() - (fileHasHeader ? 0x200 : 0x000)) / 1024);
	if (isLoROM) {
		base_snes_header = 0x8000;

		//	map rom to memory
		u8 bank = 0x80;
		u8 shadow_bank = 0x00;
		u8 chunks = filesizeInKb / 32;		//	LoRAM stores 32kb chunks, so we want to know how many chunks it takes up before the first mirroring (has to) appear
		for (u32 i = 0; i < cartridge_memory.size(); i++) {

			//	write to all locations that mirror our ROM
			for (u8 mirror = 0; mirror < (0x80 / chunks); mirror++) {

				//	(Q3-Q4) 32k (0x8000) consecutive chunks to banks 0x80-0xff, upper halfs (0x8000-0xffff)
				if ((bank + (i / 0x8000) + (mirror * chunks)) < 0xff) {
					memory[((bank + (i / 0x8000) + (mirror * chunks)) << 16) | 0x8000 + (i % 0x8000)] = cartridge_memory[i + (fileHasHeader ? 0x200 : 0x000)];
				}

				//	(Q1-Q2) shadowing to banks 0x00-0x7d, except WRAM (bank 0x7e/0x7f)
				if ((shadow_bank + (i / 0x8000) + (mirror * chunks)) < 0x7e) {
					memory[((shadow_bank + (i / 0x8000) + (mirror * chunks)) << 16) | 0x8000 + (i % 0x8000)] = cartridge_memory[i + (fileHasHeader ? 0x200 : 0x000)];
				}

			}
			
		}

	}

	/*cout << "Check Mirroring:\n";
	cout << hex << memory[0x808000] << " " << memory[0x908000] << " " << memory[0xa08000] << " " << memory[0xb08000] << " \n";
	cout << hex << memory[0x818000] << " " << memory[0x948000] << " " << memory[0xa58000] << " " << memory[0xbb8000] << " \n";
	cout << hex << memory[0xf18000] << " " << memory[0xf48000] << " " << memory[0xf58000] << " " << memory[0xfb8000] << " \n";
	cout << hex << memory[0x008000] << " " << memory[0x208000] << " " << memory[0x408000] << " " << memory[0x508000] << " \n";
	cout << "<done\n";*/

	u8 header[0x50];
	for (u8 i = 0; i < 0x50; i++) {
		header[i] = cartridge_memory[base_snes_header - 0x50 + i];
	}
	cartridge.initSNESHeader(header);
	
	/*cout << "Loaded '" << filename << "' - " << filesizeInKb << " kbytes..\n";
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
	cout << "ExpRAM size:\t\t" << cartridge.getExpansionRAMString() << "\n\n";*/

	resetCPU( cartridge.emu_reset_vector );
}

uint8_t readFromMem(u16 adr, u8 bank_nr) {
	switch (bank_nr)
	{
		
		case 0x7e:		//	WRAM (work RAM)
		case 0x7f:
			break;
		default:
			return memory[(bank_nr << 16) | adr];
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