#ifndef LIB_PPU
#define LIB_PPU

#include <string>
#include <vector>
#include <functional>
#include "SDL2/include/SDL.h"
typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

//	Functions
void PPU_init(std::string filename);
void PPU_step(u8 steps);
void PPU_writeVRAMhigh(u8 val, u16 adr);
void PPU_writeVRAMlow(u8 val, u16 adr);
u16 PPU_readVRAM(u16 adr);
void PPU_drawFrame();
void PPU_setTitle(std::string filename);
u16 PPU_readCGRAM(u8 adr);
void PPU_writeCGRAM(u8 val, u8 adr);
void PPU_reset();
bool PPU_getVBlankNMIFlag();
void PPU_writeBGScrollX(u8 bg_id, u8 val);
void PPU_writeBGScrollY(u8 bg_id, u8 val);
void PPU_setMosaic(u8 val);
void PPU_writeINIDISP(u8 val);
void PPU_writeColorMathControlRegisterA(u8 val);
void PPU_writeColorMathControlRegisterB(u8 val);
void PPU_writeMainScreenEnabled(u8 val);
void PPU_writeSubScreenEnabled(u8 val);
void PPU_writeBGTilebase(u8 bg_id, u8 val);
void PPU_writeBGScreenSizeAndBase(u8 bg_id, u8 val);
void PPU_writeBGMode(u8 val);
void PPU_writeWindow1PositionLeft(u8 val);
void PPU_writeWindow1PositionRight(u8 val);
void PPU_writeWindow2PositionLeft(u8 val);
void PPU_writeWindow2PositionRight(u8 val);
void PPU_writeWindowEnableBG12(u8 val);
void PPU_writeWindowEnableBG34(u8 val);
void PPU_writeWindowEnableBGOBJSEL(u8 val);
void PPU_writeWindowMaskLogicBGs(u8 val);
void PPU_writeWindowMaskLogicOBJSEL(u8 val);
void PPU_writeMainscreenDisable(u8 val);
void PPU_writeSubscreenDisable(u8 val);
void PPU_writeSubscreenFixedColor(u8 val);

//	debug
void debug_drawBG(u8 id);

static inline bool _or(const bool a, const bool b) {
	return a || b;
}
static inline bool _and(const bool a, const bool b) {
	return a && b;
}
static inline bool _xor(const bool a, const bool b) {
	return a != b;
}
static inline bool _xnor(const bool a, const bool b) {
	return !(a != b);
}

static inline bool _always(const bool a) {
	return true;
}
static inline bool _pass(const bool a) {
	return a;
}
static inline bool _invert(const bool a) {
	return !a;
}
static inline bool _never(const bool a) {
	return false;
}

struct PIXEL {
	u8 id = 0;				//	layer ID, necessary for MathEn
	u16 color = 0x0000;		//	15-bit color value + 1-bit alpha
	u8 priority = 0;		//	1 - high priority, 0 - low priority; OBJ have 2-bit priority, from 0 (lowest) to 3 (highest)
};

enum class PPU_COLOR_DEPTH {
	CD_2BPP_4_COLORS,
	CD_4BPP_16_COLORS,
	CD_8BPP_256_COLORS,
	CD_OFFSET_PER_TILE,
	CD_EXTBG,
	CD_DISABLED
};

static PPU_COLOR_DEPTH PPU_BG_MODES[][4] = {
	{PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS },		//	mode 0	-	2bpp, 2bpp, 2bpp, 2bpp	-	Normal
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_DISABLED},			//	mode 1	-	4bpp, 4bpp, 2bpp, -		-	Normal
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_OFFSET_PER_TILE, PPU_COLOR_DEPTH::CD_DISABLED},		//	mode 2	-	4bpp, 4bpp, OPT, -		-	Offset-per-Tile
	{PPU_COLOR_DEPTH::CD_8BPP_256_COLORS, PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_DISABLED},				//	mode 3	-	8bpp, 4bpp, -, -		-	Normal
	{PPU_COLOR_DEPTH::CD_8BPP_256_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_OFFSET_PER_TILE, PPU_COLOR_DEPTH::CD_DISABLED},		//	mode 4	-	8bpp, 2bpp, OPT, -		-	Offset-per-Tile
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_2BPP_4_COLORS, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_DISABLED},				//	mode 5	-	4bpp, 2bpp, -, -		-	512-pix-hires
	{PPU_COLOR_DEPTH::CD_4BPP_16_COLORS, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_OFFSET_PER_TILE, PPU_COLOR_DEPTH::CD_DISABLED},				//	mode 6	-	4bpp, -, OPT, -			-	512-pix plus Offset-per-Tile
	{PPU_COLOR_DEPTH::CD_8BPP_256_COLORS, PPU_COLOR_DEPTH::CD_EXTBG, PPU_COLOR_DEPTH::CD_DISABLED, PPU_COLOR_DEPTH::CD_DISABLED}						//	mode 7	-	8bpp, EXTBG, -, -		-	Rotation/Scaling
};

struct COLOR_MATH {

	u8 W1_LEFT;
	u8 W1_RIGHT;
	u8 W2_LEFT;
	u8 W2_RIGHT;
	//	BG1
	//	BG2
	//	BG2
	//	BG3
	//	OBJ
	//	Color
	bool w1en[6];
	bool w2en[6];
	bool w1IO[6];
	bool w2IO[6];
	bool (*winlog[6])(const bool , const bool);	//	function pointers to the muxing of W1 & W2 lines
	bool tsw[5];								//	only 5 because the SEL/Math/Color line goes another way
	bool tmw[5];								//	only 5 because the SEL/Math/Color line goes another way
	bool tm[5];
	bool ts[5];

	//	color math control reg A
	bool (*mainSW)(const bool);					//	0 - Never, 1 - NotMathWindow, 2 - MathWindow, 3 - Always
	bool (*subSW)(const bool);					//	0 - Always, 1 - MathWindow, 2 - NotMathWindow, 3 - Never
	bool fixSub;								//	0 - No/Backdrop only, 1 - Yes/Backdrop+BG+OBJ
	bool directColor;							//	0 - Use Palette, 1 - Direct Color

	//	color math control reg B
	bool mathEn[6];
	bool add_sub;
	bool halfEn;
	
};

/*
		Mode0    Mode1    Mode2    Mode3    Mode4    Mode5    Mode6    Mode7
		-        BG3.1a   -        -        -        -        -        -
		OBJ.3    OBJ.3    OBJ.3    OBJ.3    OBJ.3    OBJ.3    OBJ.3    OBJ.3
		BG1.1    BG1.1    BG1.1    BG1.1    BG1.1    BG1.1    BG1.1    -
		BG2.1    BG2.1    -        -        -        -        -        -
		OBJ.2    OBJ.2    OBJ.2    OBJ.2    OBJ.2    OBJ.2    OBJ.2    OBJ.2
		BG1.0    BG1.0    BG2.1    BG2.1    BG2.1    BG2.1    -        BG2.1p
		BG2.0    BG2.0    -        -        -        -        -        -
		OBJ.1    OBJ.1    OBJ.1    OBJ.1    OBJ.1    OBJ.1    OBJ.1    OBJ.1
		BG3.1    BG3.1b   BG1.0    BG1.0    BG1.0    BG1.0    BG1.0    BG1
		BG4.1    -        -        -        -        -        -        -
		OBJ.0    OBJ.0    OBJ.0    OBJ.0    OBJ.0    OBJ.0    OBJ.0    OBJ.0
		BG3.0    BG3.0a   BG2.0    BG2.0    BG2.0    BG2.0    -        BG2.0p
		BG4.0    BG3.0b   -        -        -        -        -        -
		Backdrop Backdrop Backdrop Backdrop Backdrop Backdrop Backdrop Backdrop
*/
static PIXEL& getPixelByPriority(u8 bg_mode, PIXEL& bg1, PIXEL& bg2, PIXEL& bg3, PIXEL& bg4, PIXEL& obj, PIXEL& backdrop_fixedcolor, bool bg3_priority = false) {
	switch (bg_mode)
	{
		//	Pushing reverse order to be able to pop them out of the vector
	case 0:
		if (obj.priority == 3 && (obj.color & 1) == 1)	return obj;
		else if (bg1.priority == 1 && (bg1.color & 1) == 1)	return bg1;
		else if (bg2.priority == 1 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 2 && (obj.color & 1) == 1) return obj;
		else if (bg1.priority == 0 && (bg1.color & 1) == 1)	return bg1;
		else if (bg2.priority == 0 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 1 && (obj.color & 1) == 1) return obj;
		else if (bg3.priority == 1 && (bg3.color & 1) == 1)	return bg3;
		else if (bg4.priority == 1 && (bg4.color & 1) == 1)	return bg4;
		else if (obj.priority == 0 && (obj.color & 1) == 1) return obj;
		else if (bg3.priority == 0 && (bg3.color & 1) == 1)	return bg3;
		else if (bg4.priority == 0 && (bg4.color & 1) == 1)	return bg4;
		break;
	case 1:
		if (bg3.priority == 1 && bg3_priority && (bg3.color & 1) == 1) return bg3;
		else if (obj.priority == 3 && (obj.color & 1) == 1)	return obj;
		else if (bg1.priority == 1 && (bg1.color & 1) == 1)	return bg1;
		else if (bg2.priority == 1 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 2 && (obj.color & 1) == 1) return obj;
		else if (bg1.priority == 0 && (bg1.color & 1) == 1)	return bg1;
		else if (bg2.priority == 0 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 1 && (obj.color & 1) == 1) return obj;
		else if (bg3.priority == 1 && !bg3_priority && (bg3.color & 1) == 1) return bg3;
		else if (obj.priority == 0 && (obj.color & 1) == 1) return obj;
		else if (bg3.priority == 0 && (bg3.color & 1) == 1)	return bg3;
		break;
	case 2:
		if (obj.priority == 3 && (obj.color & 1) == 1)	return obj;
		else if (bg1.priority == 1 && (bg1.color & 1) == 1)	return bg1;
		else if (obj.priority == 2 && (obj.color & 1) == 1) return obj;
		else if (bg2.priority == 1 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 1 && (obj.color & 1) == 1) return obj;
		else if (bg1.priority == 0 && (bg1.color & 1) == 1)	return bg1;
		else if (obj.priority == 0 && (obj.color & 1) == 1) return obj;
		else if (bg2.priority == 0 && (bg2.color & 1) == 1)	return bg2;
		break;
	case 3:
	case 4:
	case 5:
		if (obj.priority == 3 && (obj.color & 1) == 1) return obj;
		else if (bg1.priority == 1 && (bg1.color & 1) == 1)	return bg1;
		else if (obj.priority == 2 && (obj.color & 1) == 1) return obj;
		else if (bg2.priority == 1 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 1 && (obj.color & 1) == 1) return obj;
		else if (bg1.priority == 0 && (bg1.color & 1) == 1)	return bg1;
		else if (obj.priority == 0 && (obj.color & 1) == 1) return obj;
		else if (bg2.priority == 0 && (bg2.color & 1) == 1)	return bg2;
		break;
	case 6:
		if (obj.priority == 3 && (obj.color & 1) == 1)	return obj;
		else if (bg1.priority == 1 && (bg1.color & 1) == 1)	return bg1;
		else if (obj.priority == 2 && (obj.color & 1) == 1) return obj;
		else if (obj.priority == 1 && (obj.color & 1) == 1) return obj;
		else if (bg1.priority == 0 && (bg1.color & 1) == 1)	return bg1;
		else if (obj.priority == 0 && (obj.color & 1) == 1) return obj;
		break;
	case 7:
		if (obj.priority == 3 && (obj.color & 1) == 1)	return obj;
		else if (obj.priority == 2 && (obj.color & 1) == 1) return obj;
		else if (bg2.priority == 1 && (bg2.color & 1) == 1)	return bg2;
		else if (obj.priority == 1 && (obj.color & 1) == 1) return obj;
		else if ((bg1.color & 1) == 1) return bg1;
		else if (obj.priority == 0 && (obj.color & 1) == 1) return obj;
		else if (bg2.priority == 0 && (bg2.color & 1) == 1)	return bg2;
		break;
	default: break;
	}
	return backdrop_fixedcolor;
}


#endif // !LIB_PPU