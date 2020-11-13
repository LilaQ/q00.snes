#include "bus.h"

typedef uint8_t		u8;
typedef uint16_t	u16;

u8 pbc = 0;
std::vector<u8> memory(0xffffff);
std::vector<u8> cartridge_memory;

//	vectors
//	TODO set by cartridge header
const u8* P_VECTOR_NATIVE_COP	= &memory[0xffe4];
const u8* P_VECTOR_NATIVE_BRK	= &memory[0xffe6];
const u8* P_VECTOR_NATIVE_ABORT	= &memory[0xffe8];
const u8* P_VECTOR_NATIVE_NMI	= &memory[0xffea];
const u8* P_VECTOR_NATIVE_IRQ	= &memory[0xffee];
const u8* P_VECTOR_EMU_COP		= &memory[0xfff4];
const u8* P_VECTOR_EMU_ABORT	= &memory[0xfff8];
const u8* P_VECTOR_EMU_NMI		= &memory[0xfffa];
const u8* P_VECTOR_EMU_RESET	= &memory[0xfffc];
const u8* P_VECTOR_EMU_IRQBRK	= &memory[0xfffe];

Cartridge cartridge;
Interrupts interrupts;
DMA HDMAS[8];

void BUS_reset(std::string filename) {
	setTitle(filename);
	BUS_reset();
}

void BUS_reset() {
	// experimental - wipe first 32k of memory, but only if CPU is not halted by STP instruction
	if (!CPU_isStopped()) {
		for (auto i = 0; i < 0x8000; i++)
			BUS_writeToMem(0x00, i);
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
		std::istream_iterator<u8>(file),
		std::istream_iterator<u8>());

	return vec;
}

//	copy cartridge to memory
void BUS_loadROM(std::string filename) {

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
		for (u32 i = 0; i < filesizeInKb * 1024; i++) {

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

	std::cout << "Check Mirroring:\n";
	std::cout << std::hex << memory[0x808000] << " " << memory[0x908000] << " " << memory[0xa08000] << " " << memory[0xb08000] << " \n";
	std::cout << std::hex << memory[0x818000] << " " << memory[0x948000] << " " << memory[0xa58000] << " " << memory[0xbb8000] << " \n";
	std::cout << std::hex << memory[0xf18000] << " " << memory[0xf48000] << " " << memory[0xf58000] << " " << memory[0xfb8000] << " \n";
	std::cout << std::hex << memory[0x008000] << " " << memory[0x208000] << " " << memory[0x408000] << " " << memory[0x508000] << " \n";
	std::cout << "<done\n";

	u8 header[0x50];
	for (u8 i = 0; i < 0x50; i++) {
		header[i] = cartridge_memory[base_snes_header - 0x50 + i];
	}
	cartridge.initSNESHeader(header);
	
	std::cout << "Loaded '" << filename << "' - " << filesizeInKb << " kbytes..\n";
	std::cout << "------------------------------------------------------\n";
	std::cout << "SNES Header version:\t" << cartridge.getHeaderVersionString() << "\n";
	std::cout << "ROM Name:\t\t" << cartridge.getTitleString() << "\n";
	std::cout << "Region:\t\t\t" << cartridge.getRegionString() << "\n";
	std::cout << "GameCode:\t\t" << cartridge.getGameCodeString() << "\n";
	std::cout << "ROM speed:\t\t" << ((cartridge.isFastROM) ? "FastROM (3.58 MHz)" : "SlowROM (2.68 MHz)") << "\n";
	std::cout << "ROM type:\t\t" << (cartridge.isHiROM ? "HiROM" : "LoROM") << "\n";
	std::cout << "ROM size:\t\t" << cartridge.getRAMSizeString() << "\n";
	//std::cout << "SRAM Size:\t\t" << cartridge.getSRAMSizeString() << "\n";
	std::cout << "ROM chipset:\t\t" << cartridge.getROMChipsetString() << "\n";
	std::cout << "ROM coprocessor:\t" << cartridge.getROMCoprocessorString() << "\n";
	std::cout << "Version:\t\t" << cartridge.getVersionString() << "\n";
	std::cout << "Checksum:\t\t" << cartridge.getChecksumString() << "\n";
	std::cout << "Checksum complement:\t" << cartridge.getChecksumComplementString() << "\n";
	std::cout << "Checksum okay? \t\t" << cartridge.getChecksumOkay() << "\n";
	std::cout << "Dev-ID:\t\t\t" << cartridge.getDevIDString() << "\n";
	std::cout << "Flash size:\t\t" << cartridge.getFlashSizeString() << "\n";
	std::cout << "ExpRAM size:\t\t" << cartridge.getExpansionRAMString() << "\n\n";

	BUS_reset(filename);
}

u16 NMI = 0xc2;

u8 BUS_readFromMem(u32 fulladr) {
	u8 bank_nr = fulladr >> 16;
	u16 adr = fulladr & 0xffff;

	/*if (adr >= 0x4300 && adr <= 0x430A)
		printf("Read From %x\n", adr);*/

	//	CPU / MMIO / PPU area (lower Q1)
	if (adr < 0x8000 && ((bank_nr < 0x40) || (bank_nr >= 0x80 && bank_nr < 0xc0))) {

		//	WRAM
		if (adr < 0x2000) {
			return memory[0x7e0000 + adr];
		}

		switch (adr) {
		case 0x2138:			//	PPU - RDOAM - Read OAM Data (R)
			PPU_readOAM();
			break;
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

		case 0x2140: return CPU_readFromSCP700(0); break;	//	APU - Main communication registers
		case 0x2141: return CPU_readFromSCP700(1); break;
		case 0x2142: return CPU_readFromSCP700(2); break;
		case 0x2143: return CPU_readFromSCP700(3); break;

		case 0x4200: {
			//	TODO - This register also does Joypad Enable
			return (interrupts.isNMIEnabled() << 7) | (interrupts.isVEnabled() << 5) | (interrupts.isHEnabled() << 4);
			break;
		}

		case 0x4210: {			//	PPU Interrupts - V-Blank NMI Flag and CPU Version Number (R) [Read/Ack]
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
			bool res = interrupts.isNMIFlagged();
			interrupts.clearNMIFlag();
			return (res << 7) | 0x62;
			break;
		}
		case 0x4211:			//	PPU Interrupts - H/V-Timer IRQ Flag (R) [Read/Ack]
			return 0;
			break;
		case 0x4212:			//	PPU Interrupts - H/V-Blank Flag and Joypad Busy Flag (R)
			return 0;
			break;
		default:
			return memory[adr];
			break;
		}

	} 
	else {
		return memory[fulladr];
	}
}

void BUS_writeToMem(u8 val, u32 fulladr) {
	u8 bank_nr = fulladr >> 16;
	u16 adr = fulladr & 0xffff;

	//	CPU / MMIO / PPU area (lower Q1)
	if (adr < 0x8000 && ((bank_nr < 0x40) || (bank_nr >= 0x80 && bank_nr < 0xc0))) {

		//	WRAM
		if (adr < 0x2000) {
			memory[0x7e0000 + adr] = val;
		}

		switch (adr) {

		case 0x2100:				//	PPU - Forced Blanking	/ Master Brightness
			PPU_writeINIDISP(val);
			break;
		case 0x2101:				//	PPU	- OBSEL - Object Size & Object Base
			PPU_writeOAMOBSEL(val);
			break;
		case 0x2102:				//	PPU - OAMADDL / OAM Address Low
			PPU_writeOAMAddressLow(val);
			break;
		case 0x2103:				//	PPU - OAMADDL / OAM Address High
			PPU_writeOAMAddressHigh(val);
			break;
		case 0x2104:				//	PPU - OAMDATA / Write data to OAM
			PPU_writeOAM(val);
			break;
		case 0x2105:				//	PPU - BGMODE - BG Mode and Character Size (W)
			PPU_writeBGMode(val);
			break;
		case 0x2106:				//	PPU - Set Mosaic
			PPU_setMosaic(val);
			break;
		case 0x2107:				//	PPU - BG1SC - BG1 Screen size and Screen Base (W)
			PPU_writeBGScreenSizeAndBase(0, val);
			break;
		case 0x2108:				//	PPU - BG2SC - BG2 Screen size and Screen Base (W)
			PPU_writeBGScreenSizeAndBase(1, val);
			break;
		case 0x2109:				//	PPU - BG3SC - BG3 Screen size and Screen Base (W)
			PPU_writeBGScreenSizeAndBase(2, val);
			break;
		case 0x210a:				//	PPU - BG4SC - BG4 Screen size and Screen Base (W)
			PPU_writeBGScreenSizeAndBase(3, val);
			break;
		case 0x210b:				//	PPU - BG12NBA - BG1/BG2 Tilebase
			PPU_writeBGTilebase(0, (val & 0xf));
			PPU_writeBGTilebase(1, (val >> 4));
			break;
		case 0x210c:				//	PPU - BG34BA - BG3/BG4 Tilebase
			PPU_writeBGTilebase(2, (val & 0xf));
			PPU_writeBGTilebase(3, (val >> 4));
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

		case 0x2123:			//	PPU - W12SEL - Window BG1 / BG2 Mask Settings (W)
			PPU_writeWindowEnableBG12(val);
			break;
		case 0x2124:			//	PPU - W34SEL - Window BG3 / BG4 Mask Settings (W)
			PPU_writeWindowEnableBG34(val);
			break;
		case 0x2125:			//	PPU - WOBJSEL - Window OBJ / Math(Color) Mask Settings (W)
			PPU_writeWindowEnableBGOBJSEL(val);
			break;

		case 0x2126:			//	PPU - WH0 - Window 1 Left Position
			PPU_writeWindow1PositionLeft(val);
			break;
		case 0x2127:			//	PPU - WH1 - Window 1 Right Position
			PPU_writeWindow1PositionRight(val);
			break;
		case 0x2128:			//	PPU - WH2 - Window 2 Left Position
			PPU_writeWindow2PositionLeft(val);
			break;
		case 0x2129:			//	PPU - WH3 - Window 2 Right Position
			PPU_writeWindow2PositionRight(val);
			break;

		case 0x212a:			//	PPU - WBGLOG - Window 1 / 2 Mask Logic (W)
			PPU_writeWindowMaskLogicBGs(val);
			break;
		case 0x212b:			//	PPU - WOBJLOG - Window 1 / 2 Mask Logic (W)
			PPU_writeWindowMaskLogicOBJSEL(val);
			break;

		case 0x212c:			//	PPU - TM - MainScreen Enable / Disable
			PPU_writeMainScreenEnabled(val);
			break;
		case 0x212d:			//	PPU - TS - SubScreen Enable / Disable
			PPU_writeSubScreenEnabled(val);
			break;

		case 0x212e:			//	PPU - TMW - Window area Main screen Disable (W)
			PPU_writeMainscreenDisable(val);
			break;
		case 0x212f:			//	PPU - TSW - Window area Sub screen Disable (W)
			PPU_writeSubscreenDisable(val);
			break;

		case 0x2130:			//	PPU - CGWSEL - Color Math Control Register A (W)
			PPU_writeColorMathControlRegisterA(val);
			break;
		case 0x2131:			//	PPU - CGADSUB - Color Math Control Registers B (W)
			PPU_writeColorMathControlRegisterB(val);
			break;

		case 0x2132:			//	PPU - COLDATA - Color Math Sub Screen Backdrop (Fixed Color) (W)
			PPU_writeSubscreenFixedColor(val);
			break;

		case 0x2140: CPU_writeToSCP700(0, val);	break; //	APU - Main communication registers
		case 0x2141: CPU_writeToSCP700(1, val); break;
		case 0x2142: CPU_writeToSCP700(2, val); break;
		case 0x2143: CPU_writeToSCP700(3, val); break;
			break;

		case 0x4200:			//	PPU Interrupts - Interrupt Enable and Joypad Requests (W)
			interrupts.setNMI(val >> 7);
			interrupts.setV(val & 0x100000);
			interrupts.setH(val & 0x010000);
			break;

		//	CPU MATH
		case 0x4202:			//	CPU MATH - WRMPYA - Multiplicand
			memory[adr] = val;
			break;
		case 0x4203:			//	CPU MATH - WRMPYB - Multiplier & Start Multiplication
			memory[adr] = val;
			memory[0x4216] = (val * memory[0x4202]) & 0xff;
			memory[0x4217] = (val * memory[0x4202]) >> 8;
			//	Quirks
			memory[0x4214] = val;
			memory[0x4215] = 0x00;
			break;
		case 0x4204:			//	CPU MATH - WRDIVL - 16 Bit Dividend (lower 8 Bit)
			memory[adr] = val;
			break;
		case 0x4205:			//	CPU MATH - WRDIVH - 16 Bit Dividend (upper 8 Bit)
			memory[adr] = val;
			break;
		case 0x4206:			//	CPU MATH - WRDIVB - 8 Bit Divisor & Start Division
			memory[adr] = val;
			if (!val) {			//	Division by zero, return 0xFFFF as Result, Remainder is Dividend
				memory[0x4214] = 0xff;
				memory[0x4215] = 0xff;
				memory[0x4216] = memory[0x4204];
				memory[0x4217] = memory[0x4205];
				return;
				break;
			}
			memory[0x4214] = (u16)(((memory[0x4205] << 8) | memory[0x4204]) / val) & 0xff;
			memory[0x4215] = (u16)(((memory[0x4205] << 8) | memory[0x4204]) / val) >> 8;
			memory[0x4216] = (u16)(((memory[0x4205] << 8) | memory[0x4204]) % val) & 0xff;
			memory[0x4217] = (u16)(((memory[0x4205] << 8) | memory[0x4204]) % val) >> 8;
			break;

		//	MODE 7 COLOR MATH
		case 0x211b:
			//printf("Unimplemented M7 Color Math!\n");
			break;
		case 0x211c:
			//printf("Unimplemented M7 Color Math!\n");
			break;

			//	TODO Writes to the following registers
		case 0x4207:			//	PPU Interrupts - H-Count Timer Setting - Low (W)
			break;
		case 0x4208:			//	PPU Interrupts - H-Count Timer Setting - High (W)
			break;
		case 0x4209:			//	PPU Interrupts - V-Count Timer Setting - Low (W)
			break;
		case 0x420a:			//	PPU Interrupts - V-Count Timer Setting - High (W)
			break;

		case 0x420b:			//	DMA - MDMAEN - Select general purpose DMA Channel(s) and start Transfer (W)
			memory[adr] = val;
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
		memory[adr] = val;
	}
	else {
		memory[fulladr] = val;
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
			if(bytes)
				while ((bytes = BUS_DMAtransfer(dma_id, dma_mode, dma_dir, dma_step, cpu_address, io_address, bytes))) {}
			//	set byte counting values to 0 (since this memory location acts as a counter)
			memory[0x4306 + (dma_id * 0x10)] = 0;
			memory[0x4305 + (dma_id * 0x10)] = 0;
		}
	}
	//	reset DMA-start register
	memory[0x420b] = 0x00;
}

void BUS_startHDMA() {
	for (u8 dma_id = 0; dma_id < 8; dma_id++) {
		if (HDMAS[dma_id].enabled && !HDMAS[dma_id].terminated) {

			if (HDMAS[dma_id].line_counter-- == 0) {
				if (memory[HDMAS[dma_id].address] == 0x00) {
					HDMAS[dma_id].terminated = true;
					return;
				}
				HDMAS[dma_id].repeat = memory[HDMAS[dma_id].address] >> 7;
				HDMAS[dma_id].line_counter = memory[HDMAS[dma_id].address] & 0x7f;
				HDMAS[dma_id].address++;
				if (HDMAS[dma_id].addressing_mode) {		//	0 - Direct, 1 - Indirect
					HDMAS[dma_id].indirect_address = (memory[HDMAS[dma_id].address + 1] << 8) | memory[HDMAS[dma_id].address];
					HDMAS[dma_id].address += 2;
				}
				if (HDMAS[dma_id].addressing_mode) {		//	0 - Direct, 1 - Indirect
					BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].direction, 0, HDMAS[dma_id].indirect_address, HDMAS[dma_id].IO, 100);
					return;
				}
				else {
					BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].direction, 0, HDMAS[dma_id].address, HDMAS[dma_id].IO, 100);
					return;
				}
			}
			
			if (HDMAS[dma_id].repeat) {
				if (HDMAS[dma_id].addressing_mode) {		//	0 - Direct, 1 - Indirect
					BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].direction, 0, HDMAS[dma_id].indirect_address, HDMAS[dma_id].IO, 100);
				}
				else {
					BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].direction, 0, HDMAS[dma_id].address, HDMAS[dma_id].IO, 100);
				}
			}

		}
	}
}

void BUS_resetHDMA() {
	for (u8 dma_id = 0; dma_id < 8; dma_id++) {
		if (HDMAS[dma_id].enabled) {
			HDMAS[dma_id].terminated = false;
			HDMAS[dma_id].IO = memory[0x4301 + (dma_id * 0x10)];
			HDMAS[dma_id].addressing_mode = (memory[0x4300 + (dma_id * 0x10)] >> 6) & 1;
			HDMAS[dma_id].direction = memory[0x4300 + (dma_id * 0x10)] >> 7;
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
				BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].direction, 0, HDMAS[dma_id].indirect_address, HDMAS[dma_id].IO, 100);
			}
			else {
				BUS_DMAtransfer(dma_id, HDMAS[dma_id].dma_mode, HDMAS[dma_id].direction, 0, HDMAS[dma_id].address, HDMAS[dma_id].IO, 100);
			}
		}
	}
}