#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include <iostream>
using namespace::std;

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

struct Cartridge {

private:
	enum class ROM_chipset {
		ROM_only				= 0b0000,
		ROM_SRAM				= 0b0001,
		ROM_SRAM_BATTERY		= 0b0010,
		ROM_CO_CPU				= 0b0011,
		ROM_CO_CPU_SRAM			= 0b0100,
		ROM_CO_CPU_SRAM_BATTERY = 0b0101,
		ROM_CO_CPU_BATTERY		= 0b0110
	};
	map<u8, string> ROM_chipset_string = {
		{0b0000, "ROM only"},
		{0b0001, "ROM + SRAM"},
		{0b0010, "ROM + SRAM & Battery"},
		{0b0011, "ROM + Co-CPU"},
		{0b0100, "ROM + Co-CPU + SRAM"},
		{0b0101, "ROM + Co-CPU + SRAM & Battery"},
		{0b0110, "ROM + Co-CPU + Battery"}
	};

	enum class ROM_coprocessor {
		DSP						= 0b0000,
		SuperFX					= 0b0001,
		OBC1					= 0b0010,
		SA_1					= 0b0011,
		S_DD1					= 0b0100,
		S_RTC					= 0b0101,
		Other					= 0b1000,
		Custom					= 0b1111
	};
	map<u8, string> ROM_coprocessor_string = {
		{0b0000, "DSP"},
		{0b0001, "SuperFX"},
		{0b0010, "OBC1"},
		{0b0011, "SA-1"},
		{0b0100, "S-DD1"},
		{0b0101, "S-RTC"},
		{0b1000, "Other"},
		{0b1111, "Custom (v2/v3 header)"}
	};

	map<u8, string> Region = {
		{0x00, "Japan"},
		{0x01, "Most of North America"},
		{0x02, "Most of Europe"},
		{0x03, "Sweden"},
		{0x04, "Japan"},
		{0x05, "Denmark"},
		{0x06, "France"},
		{0x07, "Netherlands"},
		{0x08, "Spain"},
		{0x09, "Germany"},
		{0x0a, "Italy"},
		{0x0b, "China"},
		{0x0c, "Indonesia"},
		{0x0d, "South Korea"},
		{0x0e, "Common/International"},
		{0x0f, "Canada"},
		{0x10, "Brazil"},
		{0x11, "Australia"}
	};

	u8 header_version;
	string game_title;
	ROM_chipset rom_chipset;
	ROM_coprocessor rom_coprocessor;
	u8 rom_size;			//	size bits
	u8 sram_size;			//	size bits
	u32 rom_real_size;		//	size in bytes
	u32 sram_real_size;		//	size in bytes
	u8 flash_size_v3_header;
	u32 flash_real_size_v3_header;
	u8 xram_size_v3_header;
	u32 xram_real_size_v3_header;
	u8 region;
	u8 dev_id;
	string dev_id_v3_header;
	string game_code_v3_header;
	u8 version;
	u16 checksum_complement;
	u16 checksum;

public:

	bool isFastROM;
	bool isHiROM;

	//	vectors
	u16 native_cop_vector;
	u16 native_brk_vector;
	u16 native_abort_vector;
	u16 native_nmi_vector;
	u16 native_irq_vector;
	u16 emu_cop_vector;
	u16 emu_abort_vector;
	u16 emu_nmi_vector;
	u16 emu_reset_vector;
	u16 emu_irq_brk_vector;

	void initSNESHeader(u8 header[]) {
		header_version = 1;
		if (header[0x24] == 0x00) {
			header_version = 2;
		}
		if (header[0x2a] == 0x33) {
			header_version = 3;
		}
		for (u8 i = 0; i < 21; i++) {
			game_title += (char)header[i + 0x10];
		}
		isFastROM = ((header[0x25]) & 0b110000) == 0b110000;
		isHiROM = ((header[0x25]) & 1) == 1;
		rom_chipset = (ROM_chipset)(header[0x26] & 0b1111);
		rom_coprocessor = (ROM_coprocessor)(header[0x26] >> 8);
		rom_size = header[0x27]; 
		printf("This is rom size %d", rom_size);
		rom_real_size = 0x400 << rom_size;
		sram_size = header[0x28];
		sram_real_size = (sram_size != 0) ? 0x400 << sram_size : 0;
		region = header[0x29];
		dev_id = header[0x2a];
		dev_id_v3_header = (header_version == 3) ? to_string((char)header[0x00] + (char)header[0x01]) : "/";
		game_code_v3_header = (header_version == 3) ? to_string((char)header[0x02] + (char)header[0x03] + (char)header[0x04] + (char)header[0x05]) : "/";
		flash_size_v3_header = (header_version == 3) ? header[0x0c] : 0;
		flash_real_size_v3_header = (flash_size_v3_header != 0) ? 0x400 << flash_size_v3_header : 0;
		xram_size_v3_header = (header_version == 3) ? header[0x0c] : 0;
		xram_real_size_v3_header = (xram_size_v3_header != 0) ? 0x400 << xram_size_v3_header : 0;
		checksum_complement = (header[0x2d] << 8) | header[0x2c];
		checksum = (header[0x2f] << 8) | header[0x2e];

		//	vectors
		native_cop_vector = (header[0x35] << 8) | header[0x34];
		native_brk_vector = (header[0x37] << 8) | header[0x36];
		native_abort_vector = (header[0x39] << 8) | header[0x38];
		native_nmi_vector = (header[0x3b] << 8) | header[0x3a];
		native_irq_vector = (header[0x3f] << 8) | header[0x3e];
		emu_cop_vector = (header[0x45] << 8) | header[0x44];
		emu_abort_vector = (header[0x49] << 8) | header[0x48];
		emu_nmi_vector = (header[0x4b] << 8) | header[0x4a];
		emu_reset_vector = (header[0x4d] << 8) | header[0x4c];
		emu_irq_brk_vector = (header[0x4f] << 8) | header[0x4e];
	}
	string getTitleString() {
		return game_title;
	}
	string getROMChipsetString() {
		return ROM_chipset_string[(u8)rom_chipset];
	}
	string getROMCoprocessorString() {
		return ROM_coprocessor_string[(u8)rom_coprocessor];
	}
	string getROMSizeString() {
		return to_string(rom_real_size);
	}
	string getRAMSizeString() {
		return to_string(sram_real_size);
	}
	string getFlashSizeString() {
		return to_string(flash_real_size_v3_header);
	}
	string getExpansionRAMString() {
		return to_string(xram_real_size_v3_header);
	}
	string getRegionString() {
		return Region[region];
	}
	string getDevIDString() {
		if (header_version != 3)
			return to_string(dev_id);
		return dev_id_v3_header;
	}
	string getGameCodeString() {
		return game_code_v3_header;
	}
	string getVersionString() {
		return "1." + to_string(version);
	}
	string getChecksumString() {
		return to_string(checksum);
	}
	string getChecksumComplementString() {
		return to_string(checksum_complement);
	}
	string getChecksumOkay() {
		if ((checksum & 0xff) + (checksum >> 8) + (checksum_complement & 0xff) + (checksum_complement >> 8) != 0x1fe)
			return "Corrupted !";
		return "Correct";
	}
	string getHeaderVersionString() {
		return to_string(header_version);
	}

	////	vectors
	//u16 getCOPVectorNative() {
	//	return native_cop_vector;
	//}
	//u16 getBRKVectorNative() {
	//	return native_brk_vector;
	//}
	//u16 getABORTVectorNative() {
	//	return native_abort_vector;
	//}
	//u16 getNMIVectorNative() {
	//	return native_nmi_vector;
	//}
	//u16 getIRQVectorNative() {
	//	return native_irq_vector;
	//}
	//u16 getCOPVectorEmulation() {
	//	return emu_cop_vector;
	//}
	//u16 getABORTVectorEmulation() {
	//	return emu_abort_vector;
	//}
	//u16 getNMIVectorEmulation() {
	//	return emu_nmi_vector;
	//}
	//u16 getRESETVectorEmulation() {
	//	return emu_reset_vector;
	//}
	//u16 getIRQBRKVectorEmulation() {
	//	return emu_irq_brk_vector;
	//}
};

void reset();
void loadROM(string filename);
unsigned char readFromMem(u16 adr, u8 bank_nr = 0);
void writeToMem(u8 val, u16 adr, u8 bank_nr = 0);

//	Addressing modes (6502)
u16 getImmediate(u16 adr);
u16 getDirectpage(u16 adr);
u16 getDirectpageXIndex(u16 adr, u8 X);
u16 getDirectpageYIndex(u16 adr, u8 Y);
u16 getIndirect(u16 adr);
u16 getIndirectXIndex(u16 adr, u8 X);
u16 getIndirectYIndex(u16 adr, u8 Y);
u16 getAbsolute(u16 adr);
u16 getAbsoluteXIndex(u16 adr, u8 X);
u16 getAbsoluteYIndex(u16 adr, u8 Y);

//	Addressing modes (65816)
u16 getStackRelative();
u16 getStackRelativeIndirectIndexedWithY();
u16 getBlockMove();
u16 getAbsoluteLong();
u16 getAbsoluteLongIndexedWithX();
u16 getAbsoluteIndirectLong();
u16 getDirectPageIndirectLong();
u16 getDirectPageIndirectLongIndexedWithY();