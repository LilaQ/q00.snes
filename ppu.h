#ifndef LIB_PPU
#define LIB_PPU

#include <string>
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

using namespace::std;
void initPPU(string filename);
void stepPPU();
void writeToVRAMhigh(u8 val, u16 adr);
void writeToVRAMlow(u8 val, u16 adr);
u16 readFromVRAM(u16 adr);
void drawFrame();
void setTitle(string filename);
u16 readFromCGRAM(u8 adr);
void writeToCGRAM(u8 val, u8 adr);
void resetPPU();

const enum class COLOR_DEPTH {
	CD_2BPP_4_COLORS,
	CD_4BPP_16_COLORS,
	CD_8BPP_256_COLORS,
	CD_OFFSET_PER_TILE,
	CD_EXTBG,
	CD_DISABLED
};

const COLOR_DEPTH BG_MODES[][4] = {
	{COLOR_DEPTH::CD_2BPP_4_COLORS, COLOR_DEPTH::CD_2BPP_4_COLORS, COLOR_DEPTH::CD_2BPP_4_COLORS, COLOR_DEPTH::CD_2BPP_4_COLORS },		//	mode 0	-	2bpp, 2bpp, 2bpp, 2bpp	-	Normal
	{COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_2BPP_4_COLORS, COLOR_DEPTH::CD_DISABLED},			//	mode 1	-	4bpp, 4bpp, 2bpp, -		-	Normal
	{COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_OFFSET_PER_TILE, COLOR_DEPTH::CD_DISABLED},		//	mode 2	-	4bpp, 4bpp, OPT, -		-	Offset-per-Tile
	{COLOR_DEPTH::CD_8BPP_256_COLORS, COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_DISABLED, COLOR_DEPTH::CD_DISABLED},				//	mode 3	-	8bpp, 4bpp, -, -		-	Normal
	{COLOR_DEPTH::CD_8BPP_256_COLORS, COLOR_DEPTH::CD_2BPP_4_COLORS, COLOR_DEPTH::CD_OFFSET_PER_TILE, COLOR_DEPTH::CD_DISABLED},		//	mode 4	-	8bpp, 2bpp, OPT, -		-	Offset-per-Tile
	{COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_2BPP_4_COLORS, COLOR_DEPTH::CD_DISABLED, COLOR_DEPTH::CD_DISABLED},				//	mode 5	-	4bpp, 2bpp, -, -		-	512-pix-hires
	{COLOR_DEPTH::CD_4BPP_16_COLORS, COLOR_DEPTH::CD_DISABLED, COLOR_DEPTH::CD_OFFSET_PER_TILE, COLOR_DEPTH::CD_DISABLED},				//	mode 6	-	4bpp, -, OPT, -			-	512-pix plus Offset-per-Tile
	{COLOR_DEPTH::CD_8BPP_256_COLORS, COLOR_DEPTH::CD_EXTBG, COLOR_DEPTH::CD_DISABLED, COLOR_DEPTH::CD_DISABLED}						//	mode 7	-	8bpp, EXTBG, -, -		-	Rotation/Scaling
};

#endif // !LIB_PPU