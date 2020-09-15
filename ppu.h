#ifndef LIB_PPU
#define LIB_PPU

#include <string>
#include "SDL2/include/SDL.h"
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

using namespace::std;
void PPU_init(string filename);
void PPU_step(u8 steps);
void PPU_writeVRAMhigh(u8 val, u16 adr);
void PPU_writeVRAMlow(u8 val, u16 adr);
u16 PPU_readVRAM(u16 adr);
void PPU_drawFrame();
void PPU_setTitle(string filename);
u16 PPU_readCGRAM(u8 adr);
void PPU_writeCGRAM(u8 val, u8 adr);
void PPU_reset();
bool PPU_getVBlankNMIFlag();

//	debug
void debug_drawBG(u8 id);

const enum class PPU_COLOR_DEPTH {
	CD_2BPP_4_COLORS,
	CD_4BPP_16_COLORS,
	CD_8BPP_256_COLORS,
	CD_OFFSET_PER_TILE,
	CD_EXTBG,
	CD_DISABLED
};

const PPU_COLOR_DEPTH PPU_BG_MODES[][4] = {
	{PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS },		//	mode 0	-	2bpp, 2bpp, 2bpp, 2bpp	-	Normal
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_DISABLED},			//	mode 1	-	4bpp, 4bpp, 2bpp, -		-	Normal
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_OFFSET_PER_TILE, PPU_COLOR_DEPTH::CD_DISABLED},		//	mode 2	-	4bpp, 4bpp, OPT, -		-	Offset-per-Tile
	{PPU_COLOR_DEPTH::CD_8BPP_256_COLORS, PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_DISABLED},				//	mode 3	-	8bpp, 4bpp, -, -		-	Normal
	{PPU_COLOR_DEPTH::CD_8BPP_256_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_OFFSET_PER_TILE, PPU_COLOR_DEPTH::CD_DISABLED},		//	mode 4	-	8bpp, 2bpp, OPT, -		-	Offset-per-Tile
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_DISABLED},				//	mode 5	-	4bpp, 2bpp, -, -		-	512-pix-hires
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_OFFSET_PER_TILE, PPU_COLOR_DEPTH::CD_DISABLED},				//	mode 6	-	4bpp, -, OPT, -			-	512-pix plus Offset-per-Tile
	{PPU_COLOR_DEPTH::CD_8BPP_256_COLORS, PPU_COLOR_DEPTH::CD_EXTBG, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_DISABLED}						//	mode 7	-	8bpp, EXTBG, -, -		-	Rotation/Scaling
};

#endif // !LIB_PPU