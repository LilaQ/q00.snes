#define _CRT_SECURE_NO_DEPRECATE
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>
#include "cpu.h"
#include "bus.h"
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

DMA HDMAS[8];

void BUS_reset(string filename) {
	PPU_setTitle(filename);
	BUS_reset();
}

void BUS_reset() {
	// experimental - wipe first 32k of memory, but only if CPU is not halted by STP instruction
	if (!CPU_isStopped()) {
		for (auto i = 0; i < 0x8000; i++)
			memory[i] = 0x00;
	}
	PPU_reset();
	resetCPU();
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
void BUS_loadROM(string filename) {

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

	BUS_reset(filename);
}

u16 NMI = 0xc2;

u8 BUS_readFromMem(u32 fulladr) {
	u8 bank_nr = fulladr >> 16;
	u16 adr = fulladr & 0xffff;

	if (adr >= 0x4300 && adr <= 0x430A)
		printf("Read From %x\n", adr);

	switch (bank_nr)
	{
		case 0x00:
			switch (adr) {
			case 0x2139:			//	PPU - VMDATAL - VRAM Write - Low (R)
			case 0x213a: {			//	PPU - VMDATAH - VRAM Write - High (R)
				u16 adr = (memory[0x2117] << 8) | memory[0x2116];
				u8 _v_hi_lo = memory[0x2115] >> 7;
				u8 _v_trans = (memory[0x2115] & 0b1100) >> 2;
				u8 _v_step = memory[0x2115] & 0b11;
				u16 _t_st, _t_off, _t_in;
				switch (_v_trans) { //	PPU - Apply address translation if necessary (leftshift thrice lower 8, 9 or 10 bits)
				case 0b00:
					break;
				case 0b01:		//	8 bit, aaaaaaaYYYxxxxx becomes aaaaaaaxxxxxYYY
					_t_st = (adr & 0b1111111100000000);
					_t_off = (adr & 0b11100000) >> 5;
					_t_in = (adr & 0b11111) << 3;
					adr = _t_st | _t_off | _t_in;
					break;
				case 0b10:		//	9 bit, aaaaaaYYYxxxxxP becomes aaaaaaxxxxxPYYY
					_t_st = (adr & 0b1111111000000000);
					_t_off = (adr & 0b111000000) >> 6;
					_t_in = (adr & 0b111111) << 3;
					adr = _t_st | _t_off | _t_in;
					break;
				case 0b11:		//	10 bit, aaaaaYYYxxxxxPP becomes aaaaaxxxxxPPYYY
					_t_st = (adr & 0b1111110000000000);
					_t_off = (adr & 0b1110000000) >> 7;
					_t_in = (adr & 0b1111111) << 3;
					adr = _t_st | _t_off | _t_in;
					break;
				}
				if (((adr == 0x2139 && !_v_hi_lo) || (adr == 0x213a && _v_hi_lo)) && _v_trans != 0) {
					u16 _t = (memory[0x2117] << 8) | memory[0x2116];
					switch (_v_step)
					{
						case 0b00: _t += 1;	break;
						case 0b01: _t += 32; break;
						case 0b10: _t += 128; break;
						case 0b11: _t += 128; break;
						default: break;
					}
					memory[0x2116] = _t & 0xff;
					memory[0x2117] = _t >> 8;
				}
				return (adr == 0x2139) ? PPU_readVRAM(adr) & 0xff : PPU_readVRAM(adr) >> 8;
				break;
			}
			case 0x213b:			//	PPU - CGDATA - Palette CGRAM Data Read (R)
				return PPU_readCGRAM(memory[0x2121]);
				memory[0x2121]++;
				break;

			case 0x2140:			//	APU - Main communication registers
			case 0x2141:
			case 0x2142:
			case 0x2143:
				break;

			case 0x4210:			//	PPU Interrupts - V-Blank NMI Flag and CPU Version Number (R) [Read/Ack]
				//	TODO there is NMI interrupt enable at $4200 that needs to be included somewhere around the flag
				//	TDOD test with WAI instruction like here : https://wiki.superfamicom.org/using-the-nmi-vblank#toc-1
				/*if (NMI == 0x42) {
					NMI = 0xc2;
					return NMI;
				}
				if (NMI == 0xc2) {
					NMI = 0x42;
					return NMI;
				}*/
				return (PPU_getVBlankNMIFlag() << 7) | 0x02;
				break;
			case 0x4211:			//	PPU Interrupts - H/V-Timer IRQ Flag (R) [Read/Ack]
				return 0;
				break;
			case 0x4212:			//	PPU Interrupts - H/V-Blank Flag and Joypad Busy Flag (R)
				return 0;
				break;
			default:
				return memory[(bank_nr << 16) | adr];
				break;
			}
		case 0x7e:					//	WRAM (work RAM)
		case 0x7f:
			return 0;
			break;
		default:
			return memory[(bank_nr << 16) | adr];
			break;
	}
}

void BUS_writeToMem(u8 val, u32 fulladr) {
	u8 bank_nr = fulladr >> 16;
	u16 adr = fulladr & 0xffff;
 	switch (bank_nr) {
	case 0x00:
		switch (adr)
		{

		case 0x2100:				//	Forced Blanking	/ Master Brightness
			PPU_writeINIDISP(val);
			break;

		case 0x2106:				//	Set Mosaic
			PPU_setMosaic(val);
			break;

		case 0x210d:				//	BG1 Horizontal Scroll
			PPU_writeBGScrollX(0, val);
			break;	
		case 0x210e:				//	BG1 Vertical Scroll
			PPU_writeBGScrollY(0, val);
			break;
		case 0x210f:				//	BG2 Horizontal Scroll
			PPU_writeBGScrollX(1, val);
			break;
		case 0x2110:				//	BG2 Vertical Scroll
			PPU_writeBGScrollY(1, val);
			break;
		case 0x2111:				//	BG3 Horizontal Scroll
			PPU_writeBGScrollX(2, val);
			break;
		case 0x2112:				//	BG3 Vertical Scroll
			PPU_writeBGScrollY(2, val);
			break;
		case 0x2113:				//	BG4 Horizontal Scroll
			PPU_writeBGScrollX(3, val);
			break;
		case 0x2114:				//	BG4 Vertical Scroll
			PPU_writeBGScrollY(3, val);
			break;

		case 0x2118:				//	PPU	Data Write
		case 0x2119: {
			u16 _adr = (memory[0x2117] << 8) | memory[0x2116];
			u8 _v_hi_lo = memory[0x2115] >> 7;
			u8 _v_trans = (memory[0x2115] & 0b1100) >> 2;
			u8 _v_step = memory[0x2115] & 0b11;
			switch (_v_trans) {		//	PPU - Apply address translation if necessary (leftshift thrice lower 8, 9 or 10 bits)
				case 0b00:
					break;
				case 0b01: {		//	8 bit, aaaaaaaYYYxxxxx becomes aaaaaaaxxxxxYYY
					u16 _t_st = (_adr & 0b1111111100000000);
					u16 _t_off = (_adr & 0b11100000) >> 5;
					u16 _t_in = (_adr & 0b11111) << 3;
					_adr = _t_st | _t_off | _t_in;
					break;
				}
				case 0b10: {		//	9 bit, aaaaaaYYYxxxxxP becomes aaaaaaxxxxxPYYY
					u16 _t_st = (_adr & 0b1111111000000000);
					u16 _t_off = (_adr & 0b111000000) >> 6;
					u16 _t_in = (_adr & 0b111111) << 3;
					_adr = _t_st | _t_off | _t_in;
					break;
				}
				case 0b11: {		//	10 bit, aaaaaYYYxxxxxPP becomes aaaaaxxxxxPPYYY
					u16 _t_st = (_adr & 0b1111110000000000);
					u16 _t_off = (_adr & 0b1110000000) >> 7;
					u16 _t_in = (_adr & 0b1111111) << 3;
					_adr = _t_st | _t_off | _t_in;
					break;
				}
			}
			if ((adr == 0x2118 && !_v_hi_lo) || (adr == 0x2119 && _v_hi_lo)) {
				u16 _t = _adr;
				switch (_v_step)
				{
					case 0b00: _t += 1;	break;
					case 0b01: _t += 32; break;
					case 0b10: _t += 128; break;
					case 0b11: _t += 128; break;
					default: break;
				}
				memory[0x2116] = _t & 0xff;
				memory[0x2117] = _t >> 8;
			}
			if (adr == 0x2118) {
				PPU_writeVRAMlow(val, _adr);
			}
			else {
				PPU_writeVRAMhigh(val, _adr);
			}
			break;
		}
		case 0x2122:			//	PPU - CGDATA - Palette CGRAM Data Write (W)
			PPU_writeCGRAM(val, memory[0x2121]);
			break;

		case 0x212c:			//	PPU - TM - MainScreen Enable / Disable
			PPU_writeBGEnabled(val);
			break;
		case 0x212d:			//	PPU - TS - SubScreen Enable / Disable
			PPU_writeSUBEnabled(val);
			break;

		case 0x2140:			//	APU - Main communication registers
		case 0x2141:
		case 0x2142:
		case 0x2143:
			break;

		case 0x4200:			//	PPU Interrupts - Interrupt Enable and Joypad Requests (W)
			break;
		case 0x4207:			//	PPU Interrupts - H-Count Timer Setting - Low (W)
			break;
		case 0x4208:			//	PPU Interrupts - H-Count Timer Setting - High (W)
			break;
		case 0x4209:			//	PPU Interrupts - V-Count Timer Setting - Low (W)
			break;
		case 0x420a:			//	PPU Interrupts - V-Count Timer Setting - High (W)
			break;

		case 0x420b:			//	DMA - MDMAEN - Select general purpose DMA Channel(s) and start Transfer (W)
			if (val > 0)
				BUS_startDMA();
			break;
		case 0x420c:			//	DMA - HDMAEN - Select H-Blank DMA (HDMA) Channel(s) and start Transfer (W)
			for (u8 i = 0; i < 8; i++) {
				HDMAS[i] = DMA((val >> i) & 1);
			}
			break;

		default:
			break;
		}
		memory[fulladr] = val;
	default:
		memory[fulladr] = val;
		break;
	}
}

u16 BUS_DMAtransfer(u8 dma_id, u8 dma_mode, u8 dma_dir, u8 dma_step, u32 &cpu_address, u8 io_address, u16 bytes_left) {
	switch (dma_mode) {
	case 0: {						//	transfer 1 byte (e.g. WRAM)
		if (!dma_dir)
			BUS_writeToMem(memory[cpu_address], 0x2100 + io_address);
		else
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address), cpu_address);
		if (!--bytes_left) return 0;
		cpu_address += (dma_step == 0) ? 1 : ((dma_step == 2) ? -1 : 0);
		break;
	}
	case 1:							//	transfer 2 bytes (xx, xx + 1) (e.g. VRAM)
		if (!dma_dir) {
			BUS_writeToMem(memory[cpu_address], 0x2100 + io_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 1], 0x2100 + io_address + 1);
			if (!--bytes_left) return 0;
		}
		else {
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address), cpu_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 1), cpu_address + 1);
			if (!--bytes_left) return 0;
		}
		cpu_address += (dma_step == 0) ? 2 : ((dma_step == 2) ? -2 : 0);
		break;
	case 2:							//	transfer 2 bytes (xx, xx) (e.g. OAM / CGRAM)
		if (!dma_dir) {
			BUS_writeToMem(memory[cpu_address], 0x2100 + io_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 1], 0x2100 + io_address);
			if (!--bytes_left) return 0;
		}
		else {
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address), cpu_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 1), cpu_address);
			if (!--bytes_left) return 0;
		}
		cpu_address += (dma_step == 0) ? 2 : ((dma_step == 2) ? -2 : 0);
		break;
	case 3:							//	transfer 4 bytes (xx, xx, xx + 1, xx + 1) (e.g. BGnxOFX, M7x)
		if (!dma_dir) {
			BUS_writeToMem(memory[cpu_address], 0x2100 + io_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 1], 0x2100 + io_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 2], 0x2100 + io_address + 1);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 3], 0x2100 + io_address + 1);
			if (!--bytes_left) return 0;
		}
		else {
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address), cpu_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 1), cpu_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 2), cpu_address + 1);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 3), cpu_address + 1);
			if (!--bytes_left) return 0;
		}
		cpu_address += (dma_step == 0) ? 4 : ((dma_step == 2) ? -4 : 0);
		break;
	case 4:							//	transfer 4 bytes (xx, xx + 1, xx + 2, xx + 3) (e.g. BGnSC, Window, APU...)
		if (!dma_dir) {
			BUS_writeToMem(memory[cpu_address], 0x2100 + io_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 1], 0x2100 + io_address + 1);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 2], 0x2100 + io_address + 2);
			if (!--bytes_left) return 0;
			BUS_writeToMem(memory[cpu_address + 3], 0x2100 + io_address + 3);
			if (!--bytes_left) return 0;
		}
		else {
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address), cpu_address);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 1), cpu_address + 1);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 2), cpu_address + 2);
			if (!--bytes_left) return 0;
			BUS_writeToMem(BUS_readFromMem(0x2100 + io_address + 3), cpu_address + 2);
			if (!--bytes_left) return 0;
		}
		cpu_address += (dma_step == 0) ? 4 : ((dma_step == 2) ? -4 : 0);
		break;
	case 5:					//	transfer 4 bytes (xx, xx + 1, xx, xx + 1) - RESERVED
		break;
	case 6:					//	same as mode 2 - RESERVED
		break;
	case 7:					//	same as mode 3 - RESERVED
		break;
	default:
		break;
	}
	return bytes_left;
}

void BUS_startDMA() {
	for (u8 dma_id = 0; dma_id < 8; dma_id++) {
		if ((memory[0x420b] & (1 << dma_id)) == (1 << dma_id)) {
			u8 io_address = memory[0x4301 + (dma_id * 0x10)];
			u8 val = memory[0x4300 + (dma_id * 0x10)];
			u8 dma_dir = val >> 7;					//	0 - write to IO, 1 - read from IO
			u8 dma_step = (val >> 3) & 0b11;		//	0 - increment, 2 - decrement, 1/3 = none
			u8 dma_mode = (val & 0b111);
			u32 cpu_address = (memory[0x4304 + (dma_id * 0x10)] << 16) | (memory[0x4303 + (dma_id * 0x10)] << 8) | memory[0x4302 + (dma_id * 0x10)];
			u16 bytes = (memory[0x4306 + (dma_id * 0x10)] << 8) | memory[0x4305 + (dma_id * 0x10)];
			while (bytes = BUS_DMAtransfer(dma_id, dma_mode, dma_dir, dma_step, cpu_address, io_address, bytes)) {}
		}
	}
	//	reset DMA-start register
	memory[0x420b] = 0x00;
}

void BUS_startHDMA() {
	u8 dma_id = 0;
	for (u8 dma_id = 0; dma_id < 8; dma_id++) {
		if (HDMAS[dma_id].enabled && !HDMAS[dma_id].terminated) {
			if (--HDMAS[dma_id].line_counter == 0) {
				HDMAS[dma_id].repeat = memory[HDMAS[dma_id].address] >> 7;
				HDMAS[dma_id].line_counter = memory[HDMAS[dma_id].address] & 0x7f;
				HDMAS[dma_id].address++;
				if (HDMAS[dma_id].addressing_mode) {		//	0 - Direct, 1 - Indirect
					HDMAS[dma_id].indirect_address = (memory[HDMAS[dma_id].address + 1] << 8) | memory[HDMAS[dma_id].address];
					HDMAS[dma_id].address += 2;
					BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].directon, 0, HDMAS[dma_id].indirect_address, HDMAS[dma_id].IO, 100);
				}
				else {
					BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].directon, 0, HDMAS[dma_id].address, HDMAS[dma_id].IO, 100);
				}
				if (!memory[HDMAS[dma_id].address]) {
					HDMAS[dma_id].terminated = true;
				}
			}
		}
	}
}

void BUS_resetHDMA() {
	u8 dma_id = 0;
	for (u8 dma_id = 0; dma_id < 8; dma_id++) {
		if (HDMAS[dma_id].enabled) {
			HDMAS[dma_id].terminated = false;
			HDMAS[dma_id].IO = memory[0x4301 + (dma_id * 0x10)];
			HDMAS[dma_id].addressing_mode = (memory[0x4300 + (dma_id * 0x10)] >> 6) & 1;
			HDMAS[dma_id].directon = memory[0x4300 + (dma_id * 0x10)] >> 7;
			HDMAS[dma_id].dma_mode = memory[0x4300 + (dma_id * 0x10)] & 0b111;
			HDMAS[dma_id].indirect_address = (memory[0x4307 + (dma_id * 0x10)] << 16) | (memory[0x4306 + (dma_id * 0x10)] << 8) | memory[0x4305 + (dma_id * 0x10)];
			HDMAS[dma_id].aaddress = (memory[0x4304 + (dma_id * 0x10)] << 16) | (memory[0x4303 + (dma_id * 0x10)] << 8) | memory[0x4302 + (dma_id * 0x10)];
			HDMAS[dma_id].address = HDMAS[dma_id].aaddress;
			HDMAS[dma_id].repeat = memory[HDMAS[dma_id].address] >> 7;
			HDMAS[dma_id].line_counter = memory[HDMAS[dma_id].address] & 0x7f;
			//	initial transfer
			HDMAS[dma_id].address++;
			if (HDMAS[dma_id].addressing_mode) {		//	0 - Direct, 1 - Indirect
				HDMAS[dma_id].indirect_address = (memory[HDMAS[dma_id].address + 1] << 8) | memory[HDMAS[dma_id].address];
				HDMAS[dma_id].address += 2;
				BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].directon, 0, HDMAS[dma_id].indirect_address, HDMAS[dma_id].IO, 100);
			}
			else {
				BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].directon, 0, HDMAS[dma_id].address, HDMAS[dma_id].IO, 100);
			}
		}
	}
}