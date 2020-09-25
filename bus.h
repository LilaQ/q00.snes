#ifndef LIB_MMU
#define LIB_MMU


#include <stdint.h>
#include <string>
#include <map>
#include <iostream>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;


struct DMA {

public:
	bool enabled = false;
	bool do_transfer = false;
	bool repeat = false;
	bool terminated = false;
	u8 directon = 0;
	u8 addressing_mode = 0;
	u8 dma_mode = 0;
	u8 IO = 0;
	u32 indirect_address = 0;
	u8 line_counter = 0;
	u32 aaddress = 0;
	u32 address = 0;

	DMA() {};
	DMA(bool e) {
		enabled = e;
	}
};


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
	std::map<u8, std::string> ROM_chipset_string = {
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
	std::map<u8, std::string> ROM_coprocessor_string = {
		{0b0000, "DSP"},
		{0b0001, "SuperFX"},
		{0b0010, "OBC1"},
		{0b0011, "SA-1"},
		{0b0100, "S-DD1"},
		{0b0101, "S-RTC"},
		{0b1000, "Other"},
		{0b1111, "Custom (v2/v3 header)"}
	};

	std::map<u8, std::string> Region = {
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
	std::string game_title;
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
	std::string dev_id_v3_header;
	std::string game_code_v3_header;
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
		game_title = "";
		for (u8 i = 0; i < 21; i++) {
			game_title += (char)header[i + 0x10];
		}
		isFastROM = ((header[0x25]) & 0b110000) == 0b110000;
		isHiROM = ((header[0x25]) & 1) == 1;
		rom_chipset = (ROM_chipset)(header[0x26] & 0b1111);
		rom_coprocessor = (ROM_coprocessor)(header[0x26] >> 4);
		rom_size = header[0x27]; 
		rom_real_size = 0x400 << rom_size;
		sram_size = header[0x28];
		sram_real_size = (sram_size != 0) ? 0x400 << sram_size : 0;
		region = header[0x29];
		dev_id = header[0x2a];
		dev_id_v3_header = (header_version == 3) ? std::to_string((char)header[0x00] + (char)header[0x01]) : "/";
		game_code_v3_header = (header_version == 3) ? std::to_string((char)header[0x02] + (char)header[0x03] + (char)header[0x04] + (char)header[0x05]) : "/";
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
	std::string getTitleString() {
		return game_title;
	}
	std::string getROMChipsetString() {
		return ROM_chipset_string[(u8)rom_chipset];
	}
	std::string getROMCoprocessorString() {
		return ROM_coprocessor_string[(u8)rom_coprocessor];
	}
	std::string getROMSizeString() {
		return std::to_string(rom_real_size);
	}
	std::string getRAMSizeString() {
		return std::to_string(sram_real_size);
	}
	std::string getFlashSizeString() {
		return std::to_string(flash_real_size_v3_header);
	}
	std::string getExpansionRAMString() {
		return std::to_string(xram_real_size_v3_header);
	}
	std::string getRegionString() {
		return Region[region];
	}
	std::string getDevIDString() {
		if (header_version != 3)
			return std::to_string(dev_id);
		return dev_id_v3_header;
	}
	std::string getGameCodeString() {
		return game_code_v3_header;
	}
	std::string getVersionString() {
		return "1." + std::to_string(version);
	}
	std::string getChecksumString() {
		return std::to_string(checksum);
	}
	std::string getChecksumComplementString() {
		return std::to_string(checksum_complement);
	}
	std::string getChecksumOkay() {
		if ((checksum & 0xff) + (checksum >> 8) + (checksum_complement & 0xff) + (checksum_complement >> 8) != 0x1fe)
			return "Corrupted !";
		return "Correct";
	}
	std::string getHeaderVersionString() {
		return std::to_string(header_version);
	}

};

//	vectors
//	TODO set by cartridge header
static const u16 VECTOR_NATIVE_COP = 0xffe4;
static const u16 VECTOR_NATIVE_BRK = 0xffe6;
static const u16 VECTOR_NATIVE_ABORT = 0xffe8;
static const u16 VECTOR_NATIVE_NMI = 0xffea;
static const u16 VECTOR_NATIVE_IRQ = 0xffee;
static const u16 VECTOR_EMU_COP = 0xfff4;
static const u16 VECTOR_EMU_ABORT = 0xfff8;
static const u16 VECTOR_EMU_NMI = 0xfffa;
static const u16 VECTOR_EMU_RESET = 0xfffc;
static const u16 VECTOR_EMU_IRQBRK = 0xfffe;

//	base methods
void BUS_reset(std::string filename);
void BUS_reset();
void BUS_loadROM(std::string filename);
u8 BUS_readFromMem(u32 adr);
void BUS_writeToMem(u8 val, u32 adr);
void BUS_startDMA();
void BUS_startHDMA();
void BUS_resetHDMA();

#endif // !LIB_MMU