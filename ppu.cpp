#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <algorithm>
#include "SDL2/include/SDL.h"
#include "bus.h"
#include "ppu.h"
#include "wmu.h"
#include "cpu.h"
#include "input.h"
#ifdef _WIN32
	#include <Windows.h>
	#include <WinUser.h>
	#include "SDL2/include/SDL_syswm.h"
#endif // _WIN32

//	init PPU
u16 VRAM[0x8000];	//	64 kbytes (16bit * 0x8000 [ 32768 ] )
u16 CGRAM[0x100];	//	512 bytes (16bit * 0x100 [ 256 ] ) 
u8 CGRAM_Lsb;
bool CGRAM_Flipflop = false;
const int FB_SIZE = 256 * 256;
SDL_Renderer* renderer;
SDL_Window* sdl_window;
u16 RENDER_X = 0, RENDER_Y = 0;
bool VBlankNMIFlag = 0;

//	textures
SDL_Texture* FULL_CALC_TEX;

//	buffers
u32 DEBUG[FB_SIZE * 4];			//	Debug window can display up to 4 screens (4 * usual size)
u32 FULL_CALC[FB_SIZE];

//	BG3 Priority flag
bool BG3_PRIORITY = false;

//	BG & SubScreen Enabled Status
PPU_COLOR_DEPTH *BG_MODE = new PPU_COLOR_DEPTH[4]();
u8 BG_MODE_ID = 0;

//	BG Scrolling
bool BGSCROLLX_Flipflop[4] = { false, false, false, false };
bool BGSCROLLY_Flipflop[4] = { false, false, false, false };
u16 BGSCROLLX[4] = { 0, 0, 0, 0 };
u16 BGSCROLLY[4] = { 0, 0, 0, 0 };

//	BG Tilebase
u16 BG_TILEBASE[5] = { 0, 0, 0, 0 };

//	BG Screen base
u16 BG_BASE[5] = { 0, 0, 0, 0 };

//	BG Tilesizes
u8 BG_TILES_H[5] = { 32, 32, 32, 32 };
u8 BG_TILES_V[5] = { 32, 32, 32, 32 };

//	BG Palette Offset (really only necessary for Mode 0 for BG2 BG3 BG4)
const u8 BG_PALETTE_OFFSET[][4] = {
	{0, 0x20, 0x40, 0x60},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
};

//	Window
COLOR_MATH window;
PIXEL backdrop_pixel;
PIXEL fixedcolor_pixel;

//	Mosaic
bool MOSAIC_ENABLED[4] = { false, false, false, false };
u8 MOSAIC_SIZE = 0;

//	INIDISP
double MASTER_BRIGHTNESS = 0;
bool FORCED_BLANKING = false;

const u8 FIVEBIT_TO_EIGHTBIT_LUT[0x20] = { 0, 8, 16, 24, 32, 41, 49, 57, 65, 74, 82, 90, 98, 106, 115, 123, 131, 139, 148, 156, 164, 172, 180, 189, 197, 205, 213, 222, 230, 238, 246, 255 };

PIXEL src_pixel_bg1, src_pixel_bg2, src_pixel_bg3, src_pixel_bg4, src_pixel_obj;
PIXEL main_pixel_bg1, main_pixel_bg2, main_pixel_bg3, main_pixel_bg4, main_pixel_obj;
PIXEL sub_pixel_bg1, sub_pixel_bg2, sub_pixel_bg3, sub_pixel_bg4, sub_pixel_obj;

void PPU_init(std::string filename) {

	/*
		MAIN WINDOW
	*/
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
	SDL_Init(SDL_INIT_VIDEO);
	/*window = SDL_CreateWindow("poop", SDL_WINDOWPOS_CENTERED, 100, 256, 239, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);*/
	SDL_CreateWindowAndRenderer(256, 239, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC, &sdl_window, &renderer);
	SDL_SetWindowSize(sdl_window, 256 * 2, 239 * 2);
	//	init and create window and renderer
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	//SDL_SetWindowSize(window, 512, 478);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(sdl_window, SDL_TRUE);

	//	for fast rendering, create a texture
	FULL_CALC_TEX = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	initWindow(sdl_window, filename);

}

void PPU_setTitle(std::string filename) {
	SDL_SetWindowTitle(sdl_window, filename.c_str());
}

void PPU_reset() {
	window.winlog[0] = _or;
	window.winlog[1] = _or;
	window.winlog[2] = _or;
	window.winlog[3] = _or;
	window.winlog[4] = _or;
	window.winlog[5] = _or;
	src_pixel_bg1.id = 0;
	src_pixel_bg2.id = 1;
	src_pixel_bg3.id = 2;
	src_pixel_bg4.id = 3;
	src_pixel_obj.id = 4;
	backdrop_pixel.id = 5;
	fixedcolor_pixel.id = 5;
	window.mainSW = _never;
	window.subSW = _always;
	backdrop_pixel.color = 0x0001;
	fixedcolor_pixel.color = 0x0001;
	BGSCROLLX_Flipflop[0] = false;
	BGSCROLLX_Flipflop[1] = false;
	BGSCROLLX_Flipflop[2] = false;
	BGSCROLLX_Flipflop[3] = false;
	BGSCROLLY_Flipflop[0] = false;
	BGSCROLLY_Flipflop[1] = false;
	BGSCROLLY_Flipflop[2] = false;
	BGSCROLLY_Flipflop[3] = false;
	BGSCROLLX[0] = 0;
	BGSCROLLX[1] = 0;
	BGSCROLLX[2] = 0;
	BGSCROLLX[3] = 0;
	BGSCROLLY[0] = 0;
	BGSCROLLY[1] = 0;
	BGSCROLLY[2] = 0;
	BGSCROLLY[3] = 0;
	BG3_PRIORITY = 0;
	BG_MODE_ID = 0;
	RENDER_X = 0, RENDER_Y = 0;
	VBlankNMIFlag = 0;
	CGRAM_Lsb = 0;
	CGRAM_Flipflop = false;
	memset(FULL_CALC, 0, sizeof(FULL_CALC));
	FULL_CALC_TEX = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
}

void processMosaic(u32 *BG) {
	for (u16 x = 0; x < 256; x += MOSAIC_SIZE) {
		for (u16 y = 0; y < 256; y += MOSAIC_SIZE) {
			u16 pos = y * 256 + x;
			u16 col = BG[pos];
			for (u8 a = 0; a < MOSAIC_SIZE; a++) {
				for (u8 b = 0; b < MOSAIC_SIZE; b++) {
					BG[min(y + b, 255) * 256 + min(x + a, 255)] = col;
				}
			}
		}
	}
}

void PPU_drawFrame() {

	//	apply mosaic if set
	/*for (u8 i = 0; i < 4; i++)
		if (MOSAIC_ENABLED[i])
			processMosaic(MAIN_BGS[i]);*/

	SDL_UpdateTexture(FULL_CALC_TEX, NULL, FULL_CALC, 1024);
	SDL_RenderCopy(renderer, FULL_CALC_TEX, NULL, NULL);
	SDL_RenderPresent(renderer);
	
}
const u8 ALPHA_LUT[2] = {0, 255};
void writeToFB(u32 *BG, const u16 x, const u16 y, const u16 width, const u16 v) {
	const u8 r = FIVEBIT_TO_EIGHTBIT_LUT[(v >> 1) & 0b11111];
	const u8 g = FIVEBIT_TO_EIGHTBIT_LUT[(v >> 6) & 0b11111];
	const u8 b = FIVEBIT_TO_EIGHTBIT_LUT[(v >> 11) & 0b11111];
	const u8 a = ALPHA_LUT[(v & 1)];
	BG[y * width + x] = r | (g << 8) | (b << 16) | (a << 24);
}

inline u16 getRGBAFromCGRAM(u32 id, u8 palette, u8 bpp, u8 palette_offset) {
	if (id == 0) return 0x00000000;	//	id 0 = transparency
	return (u16)(CGRAM[(id + palette * (bpp * bpp)) + palette_offset] * MASTER_BRIGHTNESS) << 1 | 1;
}

void renderBackdrop(u16 srcx, u16 srcy, u32* BG) {
	writeToFB(BG, srcx, srcy, 256, ((u16)(CGRAM[0x00] * MASTER_BRIGHTNESS) << 1 | 1));
};

void renderBGat2BPP(u16 scrx, u16 scry, u32 *BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u16 texture_width, u8 palette_offset) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset =
		(((bg_size_w == 64) ? (scry % 256) : (scry)) / 8) * 32 +
		((scrx % 256) / 8) +
		(scrx / 256) * 0x400 +
		(bg_size_w / 64) * ((scry / 256) * 0x800);
	const u16 tile_id = VRAM[bg_base + offset] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[bg_base + offset] >> 10) & 0b111;
	const u8 b_priority = (VRAM[bg_base + offset] >> 13) & 1;			//	0 - lower, 1 - higher
	const u8 b_flip_x = (VRAM[bg_base + offset] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[bg_base + offset] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 v_shift = i + (-i + 7 - i) * b_flip_y;
	const u8 h_shift = (7 - j) + (2 * j - 7) * b_flip_x;
	const u16 tile_address = tile_id * 8 + tile_base + v_shift;						//	this doesn't have tile_base like 8bpp, fix?
	const u8 b_hi = VRAM[tile_address] >> 8;
	const u8 b_lo = VRAM[tile_address] & 0xff;
	const u8 v = ((b_lo >> h_shift) & 1) + (2 * ((b_hi >> h_shift) & 1));
	writeToFB(BG, orgx, orgy, texture_width, getRGBAFromCGRAM(v, b_palette_nr, 2, palette_offset));
}

void renderBGat4BPP(u16 scrx, u16 scry, u32* BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u16 texture_width, u8 palette_offset) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset =
		(((bg_size_w == 64) ? (scry % 256) : (scry)) / 8) * 32 +
		((scrx % 256) / 8) +
		(scrx / 256) * 0x400 +
		(bg_size_w / 64) * ((scry / 256) * 0x800);
	const u16 tile_id = VRAM[bg_base + offset] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[bg_base + offset] >> 10) & 0b111;
	const u8 b_priority = (VRAM[bg_base + offset] >> 13) & 1;			//	0 - lower, 1 - higher
	const u8 b_flip_x = (VRAM[bg_base + offset] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[bg_base + offset] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 v_shift = i + (-i + 7 - i) * b_flip_y;
	const u8 h_shift = (7 - j) + (2 * j - 7) * b_flip_x;
	const u16 tile_address = tile_id * 16 + tile_base + v_shift;		//	this doesn't have tile_base like 8bpp, fix?
	const u8 b_1 = VRAM[tile_address] & 0xff;
	const u8 b_2 = VRAM[tile_address] >> 8;
	const u8 b_3 = VRAM[tile_address + 8] & 0xff;
	const u8 b_4 = VRAM[tile_address + 8] >> 8;
	const u16 v =	((b_1 >> h_shift) & 1) + 
					(2 * ((b_2 >> h_shift) & 1)) +
					(4 * ((b_3 >> h_shift) & 1)) +
					(8 * ((b_4 >> h_shift) & 1));
	writeToFB(BG, orgx, orgy, texture_width, getRGBAFromCGRAM(v, b_palette_nr, 4, palette_offset));
}

void renderBGat8BPP(u16 scrx, u16 scry, u32* BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u16 texture_width, u8 palette_offset) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset = 
		(((bg_size_w == 64) ? (scry % 256) : (scry)) / 8) * 32 +
		((scrx % 256) / 8) +
		(scrx / 256) * 0x400 +
		(bg_size_w / 64) * ((scry / 256) * 0x800);
	const u16 tile_base_adr = bg_base + offset;
	const u16 tile_id = VRAM[tile_base_adr] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[tile_base_adr] >> 10) & 0b111;
	const u8 b_priority = (VRAM[tile_base_adr] >> 13) & 1;				//	0 - lower, 1 - higher
	const u8 b_flip_x = (VRAM[tile_base_adr] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[tile_base_adr] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 v_shift = i + (-i + 7 - i) * b_flip_y;
	const u8 h_shift = (7 - j) + (2 * j - 7) * b_flip_x;
	const u16 tile_address = tile_id * 32 + tile_base + v_shift;
	const u8 b_1 = VRAM[tile_address] & 0xff;
	const u8 b_2 = VRAM[tile_address] >> 8;
	const u8 b_3 = VRAM[tile_address + 8] & 0xff;
	const u8 b_4 = VRAM[tile_address + 8] >> 8;
	const u8 b_5 = VRAM[tile_address + 16] & 0xff;
	const u8 b_6 = VRAM[tile_address + 16] >> 8;
	const u8 b_7 = VRAM[tile_address + 24] & 0xff;
	const u8 b_8 = VRAM[tile_address + 24] >> 8;
	const u16 v =	(		(b_1 >> h_shift) & 1) + 
					(2 *	((b_2 >> h_shift) & 1)) + 
					(4 *	((b_3 >> h_shift) & 1)) + 
					(8 *	((b_4 >> h_shift) & 1)) + 
					(16 *	((b_5 >> h_shift) & 1)) + 
					(32 *	((b_6 >> h_shift) & 1)) +
					(64 *	((b_7 >> h_shift) & 1)) +
					(128 *	((b_8 >> h_shift) & 1));
	writeToFB(BG, orgx, orgy, texture_width, getRGBAFromCGRAM(v, b_palette_nr, 8, palette_offset));
}


void getPixelEXTBG(u16 scrx, u16 scry, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u8 palette_offset, PIXEL& pixel) {
}
void getPixelOPT(u16 scrx, u16 scry, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u8 palette_offset, PIXEL& pixel) {
}
void getPixelDISABLED(u16 scrx, u16 scry, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u8 palette_offset, PIXEL& pixel) {
}
void getPixel2BPP(u16 scrx, u16 scry, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u8 palette_offset, PIXEL& pixel) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset =
		(((bg_size_w == 64) ? (scry % 256) : (scry)) / 8) * 32 +
		((scrx % 256) / 8) +
		(scrx / 256) * 0x400 +
		(bg_size_w / 64) * ((scry / 256) * 0x800);
	const u16 tile_id = VRAM[bg_base + offset] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[bg_base + offset] >> 10) & 0b111;
	const u8 b_flip_x = (VRAM[bg_base + offset] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[bg_base + offset] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 v_shift = i + (-i + 7 - i) * b_flip_y;
	const u8 h_shift = (7 - j) + (2 * j - 7) * b_flip_x;
	const u16 tile_address = tile_id * 8 + tile_base + v_shift;	
	const u8 v = (((VRAM[tile_address] >> 8) >> h_shift) & 1) + (2 * (((VRAM[tile_address] & 0xff) >> h_shift) & 1));
	pixel.color = getRGBAFromCGRAM(v, b_palette_nr, 2, palette_offset);
	pixel.priority = (VRAM[bg_base + offset] >> 13) & 1;			//	0 - lower, 1 - higher
}
void getPixel8BPP(u16 scrx, u16 scry, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u8 palette_offset, PIXEL &pixel) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset =
		(((bg_size_w == 64) ? (scry % 256) : (scry)) / 8) * 32 +
		((scrx % 256) / 8) +
		(scrx / 256) * 0x400 +
		(bg_size_w / 64) * ((scry / 256) * 0x800);
	const u16 tile_base_adr = bg_base + offset;
	const u16 tile_id = VRAM[tile_base_adr] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[tile_base_adr] >> 10) & 0b111;
	const u8 b_flip_x = (VRAM[tile_base_adr] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[tile_base_adr] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 v_shift = i + (-i + 7 - i) * b_flip_y;
	const u8 h_shift = (7 - j) + (2 * j - 7) * b_flip_x;
	const u16 tile_address = tile_id * 32 + tile_base + v_shift;
	const u16 v = (((VRAM[tile_address] & 0xff)			>> h_shift) & 1) +
		(2		* (((VRAM[tile_address] >> 8)			>> h_shift) & 1)) +
		(4		* (((VRAM[tile_address + 8] & 0xff)		>> h_shift) & 1)) +
		(8		* (((VRAM[tile_address + 8] >> 8)		>> h_shift) & 1)) +
		(16		* (((VRAM[tile_address + 16] & 0xff)	>> h_shift) & 1)) +
		(32		* (((VRAM[tile_address + 16] >> 8)		>> h_shift) & 1)) +
		(64		* (((VRAM[tile_address + 24] & 0xff)	>> h_shift) & 1)) +
		(128	* (((VRAM[tile_address + 24] >> 8)		>> h_shift) & 1));
	pixel.color = getRGBAFromCGRAM(v, b_palette_nr, 8, palette_offset);
	pixel.priority = (VRAM[tile_base_adr] >> 13) & 1;				//	0 - lower, 1 - higher
}
void getPixel4BPP(u16 scrx, u16 scry, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u8 palette_offset, PIXEL &pixel) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset =
		(((bg_size_w == 64) ? (scry % 256) : (scry)) / 8) * 32 +
		((scrx % 256) / 8) +
		(scrx / 256) * 0x400 +
		(bg_size_w / 64) * ((scry / 256) * 0x800);
	const u16 tile_id = VRAM[bg_base + offset] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[bg_base + offset] >> 10) & 0b111;
	const u8 b_flip_x = (VRAM[bg_base + offset] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[bg_base + offset] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 v_shift = i + (-i + 7 - i) * b_flip_y;
	const u8 h_shift = (7 - j) + (2 * j - 7) * b_flip_x;
	const u16 tile_address = tile_id * 16 + tile_base + v_shift;		//	this doesn't have tile_base like 8bpp, fix?
	const u16 v =	(((VRAM[tile_address] & 0xff)		>> h_shift) & 1) +
		(2 *		(((VRAM[tile_address] >> 8)			>> h_shift) & 1)) +
		(4 *		(((VRAM[tile_address + 8] & 0xff)	>> h_shift) & 1)) +
		(8 *		(((VRAM[tile_address + 8] >> 8)		>> h_shift) & 1));
	pixel.color = getRGBAFromCGRAM(v, b_palette_nr, 4, palette_offset);
	pixel.priority = (VRAM[bg_base + offset] >> 13) & 1;			//	0 - lower, 1 - higher
}

u16 vbl = 0;
void PPU_step(u8 steps) {

	while (steps--) {
		if (++RENDER_X == 256) {					//	H-Blank starts
			BUS_startHDMA();
		}		
		else if (RENDER_X == 341) {				//	PAL Line, usually takes up 341 dot cycles (unless interlace=on, field=1, line=311 it will be one additional dot cycle)
			RENDER_X = 0;
			if (++RENDER_Y == 241) {				//	V-Blank starts
				VBlankNMIFlag = true;
				Interrupts::set(Interrupts::NMI);
				printf("VBlank %d\n", ++vbl);
			}
			else if (RENDER_Y == 312) {			//	PAL System has 312 lines
				RENDER_Y = 0;
				VBlankNMIFlag = false;
				Interrupts::clear(Interrupts::NMI);
				BUS_resetHDMA();				//	A new frame will begin, we can safely reset our HDMAs now
			}
		}
		if (RENDER_X < 256 && RENDER_Y < 241) {	//	Only render current pixel(s) if we're not in any blanking period
			//PPU_render();

			const bool in_W1 = window.W1_LEFT <= RENDER_X && RENDER_X <= window.W1_RIGHT;
			const bool in_W2 = window.W2_LEFT <= RENDER_X && RENDER_X <= window.W2_RIGHT;

			const bool BG1Mux = window.winlog[0]((in_W1 && window.w1en[0] ^ window.w1IO[0]), (in_W2 && window.w2en[0] ^ window.w2IO[0]));
			const bool BG2Mux = window.winlog[1]((in_W1 && window.w1en[1] ^ window.w1IO[1]), (in_W2 && window.w2en[1] ^ window.w2IO[1]));
			const bool BG3Mux = window.winlog[2]((in_W1 && window.w1en[2] ^ window.w1IO[2]), (in_W2 && window.w2en[2] ^ window.w2IO[2]));
			const bool BG4Mux = window.winlog[3]((in_W1 && window.w1en[3] ^ window.w1IO[3]), (in_W2 && window.w2en[3] ^ window.w2IO[3]));
			const bool OBJMux = window.winlog[4]((in_W1 && window.w1en[4] ^ window.w1IO[4]), (in_W2 && window.w2en[4] ^ window.w2IO[4]));
			const bool SELMux = window.winlog[5]((in_W1 && window.w1en[5] ^ window.w1IO[5]), (in_W2 && window.w2en[5] ^ window.w2IO[5]));

			const bool MainWinBG1 = BG1Mux && window.tmw[0];
			const bool MainWinBG2 = BG2Mux && window.tmw[1];
			const bool MainWinBG3 = BG3Mux && window.tmw[2];
			const bool MainWinBG4 = BG4Mux && window.tmw[3];
			const bool MainWinOBJ = OBJMux && window.tmw[4];
			const bool SubWinBG1 = BG1Mux && window.tsw[0];
			const bool SubWinBG2 = BG2Mux && window.tsw[1];
			const bool SubWinBG3 = BG3Mux && window.tsw[2];
			const bool SubWinBG4 = BG4Mux && window.tsw[3];
			const bool SubWinOBJ = OBJMux && window.tsw[4];

			//	get pixel (incl. priority) from current x/y
			src_pixel_obj.color = 0;
			PPU_BG_MODES_FUNCTION[BG_MODE_ID][0](RENDER_X, RENDER_Y, BG_BASE[0], BG_TILES_H[0], BG_TILES_V[0], BG_TILEBASE[0], BGSCROLLX[0], BGSCROLLY[0] + 1, BG_PALETTE_OFFSET[BG_MODE_ID][0], src_pixel_bg1);
			PPU_BG_MODES_FUNCTION[BG_MODE_ID][1](RENDER_X, RENDER_Y, BG_BASE[1], BG_TILES_H[1], BG_TILES_V[1], BG_TILEBASE[1], BGSCROLLX[1], BGSCROLLY[1] + 1, BG_PALETTE_OFFSET[BG_MODE_ID][1], src_pixel_bg2);
			PPU_BG_MODES_FUNCTION[BG_MODE_ID][2](RENDER_X, RENDER_Y, BG_BASE[2], BG_TILES_H[2], BG_TILES_V[2], BG_TILEBASE[2], BGSCROLLX[2], BGSCROLLY[2] + 1, BG_PALETTE_OFFSET[BG_MODE_ID][2], src_pixel_bg3);
			PPU_BG_MODES_FUNCTION[BG_MODE_ID][3](RENDER_X, RENDER_Y, BG_BASE[3], BG_TILES_H[3], BG_TILES_V[3], BG_TILEBASE[3], BGSCROLLX[3], BGSCROLLY[3] + 1, BG_PALETTE_OFFSET[BG_MODE_ID][3], src_pixel_bg4);
			//PPU_BG_MODES_FUNCTION[BG_MODE_ID][4](RENDER_X, RENDER_Y, BG_BASE[4], BG_TILES_H[4], BG_TILES_V[4], BG_TILEBASE[4], BGSCROLLX[4], BGSCROLLY[4] + 1, BG_PALETTE_OFFSET[BG_MODE_ID][4], src_pixel_obj);
				
			//	split source pixel to main screen and sub screen
			main_pixel_bg1 = src_pixel_bg1;
			main_pixel_bg2 = src_pixel_bg2;
			main_pixel_bg3 = src_pixel_bg3;
			main_pixel_bg4 = src_pixel_bg4;
			main_pixel_obj = src_pixel_obj;
			sub_pixel_bg1 = src_pixel_bg1;
			sub_pixel_bg2 = src_pixel_bg2;
			sub_pixel_bg3 = src_pixel_bg3;
			sub_pixel_bg4 = src_pixel_bg4;
			sub_pixel_obj = src_pixel_obj;

			//	if not enabled, make pixel transparent
			main_pixel_bg1.color &= ~window.tm[0];
			main_pixel_bg2.color &= ~window.tm[1];
			main_pixel_bg3.color &= ~window.tm[2];
			main_pixel_bg4.color &= ~window.tm[3];
			main_pixel_obj.color &= ~window.tm[4];
			sub_pixel_bg1.color &= ~window.ts[0];
			sub_pixel_bg2.color &= ~window.ts[1];
			sub_pixel_bg3.color &= ~window.ts[2];
			sub_pixel_bg4.color &= ~window.ts[3];
			sub_pixel_obj.color &= ~window.ts[4];

			//	pipe the window in, pixel again will become transparent of not enabled at this point
			main_pixel_bg1.color &= ~MainWinBG1;
			main_pixel_bg2.color &= ~MainWinBG2;
			main_pixel_bg3.color &= ~MainWinBG3;
			main_pixel_bg4.color &= ~MainWinBG4;
			main_pixel_obj.color &= ~MainWinOBJ;
			sub_pixel_bg1.color &= ~SubWinBG1;
			sub_pixel_bg2.color &= ~SubWinBG2;
			sub_pixel_bg3.color &= ~SubWinBG3;
			sub_pixel_bg4.color &= ~SubWinBG4;
			sub_pixel_obj.color &= ~SubWinOBJ;

			//	main priority circuit
			PIXEL p_main = getPixelByPriority(BG_MODE_ID, main_pixel_bg1, main_pixel_bg2, main_pixel_bg3, main_pixel_bg4, main_pixel_obj, backdrop_pixel, BG3_PRIORITY);
			PIXEL p_sub = (window.fixSub == 1) ? fixedcolor_pixel : getPixelByPriority(BG_MODE_ID, sub_pixel_bg1, sub_pixel_bg2, sub_pixel_bg3, sub_pixel_bg4, sub_pixel_obj, fixedcolor_pixel, BG3_PRIORITY);
		
			p_main.color &= ~window.mainSW(SELMux);
			p_sub.color &= ~window.subSW(SELMux);

			u8 sr = (p_main.color >> 1) & 0b11111;
			u8 sg = (p_main.color >> 6) & 0b11111;
			u8 sb = (p_main.color >> 11) & 0b11111;
			u8 dr = (p_sub.color >> 1) & 0b11111;
			u8 dg = (p_sub.color >> 6) & 0b11111;
			u8 db = (p_sub.color >> 11) & 0b11111;
			if (window.mathEn[p_main.id]) {
				if (window.add_sub) {					//	Subtract
					sr = max(sr - dr, 0);
					sg = max(sg - dg, 0);
					sb = max(sb - db, 0);
				}
				else {									//	Add
					sr = max(sr + dr, 0x1f);
					sg = max(sg + dg, 0x1f);
					sb = max(sb + db, 0x1f);
				}
				if (window.halfEn) {					//	Divide result by 2
					sr >>= 1;
					sg >>= 1;
					sb >>= 1;
				}
			}

			//	write to framebuffer
			writeToFB(FULL_CALC, RENDER_X, RENDER_Y, 256, ((sb << 11) | (sg << 6) | (sr << 1) | 1));
		}
		else if (RENDER_X == 0 && RENDER_Y == 241) {	//	Exclude drawing mechanism so every X/Y modification is done by this point
			INPUT_stepJoypadAutoread();
			PPU_drawFrame();
			//printf("Scroll x : %x  y: %x\n", BGSCROLLX[0], BGSCROLLY[0]);
		}
		
	}

}

void PPU_writeVRAMlow(u8 val, u16 adr) {
	VRAM[adr & 0x7fff] = (VRAM[adr & 0x7fff] & 0xff00) | val;
}
void PPU_writeVRAMhigh(u8 val, u16 adr) {
	VRAM[adr & 0x7fff] = (VRAM[adr & 0x7fff] & 0xff) | (val << 8);
}

u16 PPU_readVRAM(u16 adr) {
	return VRAM[adr & 0x7fff];
}

void PPU_writeCGRAM(u8 val, u8 adr) {
	//	if address is even, we just remember the current value
	if (!CGRAM_Flipflop) {
		CGRAM_Lsb = val;
		CGRAM_Flipflop = true;
	}
	//	else concat the remembered value with the current one, and store it to adr-1
	else {
		CGRAM[adr] = (val << 8) | CGRAM_Lsb;
		CGRAM_Flipflop = false;
		BUS_writeToMem(BUS_readFromMem(0x2121) + 1, 0x2121);
		if (adr == 0x0)	//	store backdrop separately for color math
			backdrop_pixel.color = (CGRAM[0x00] << 1) | 1;
	}
	//printf("Write CGRAM: %x\n", val);
}

u16 PPU_readCGRAM(u8 adr) {
	return CGRAM[adr];
}

void PPU_writeWindow1PositionLeft(u8 val) {
	window.W1_LEFT = val;
}
void PPU_writeWindow1PositionRight(u8 val) {
	window.W1_RIGHT = val;
}
void PPU_writeWindow2PositionLeft(u8 val) {
	window.W2_LEFT = val;
}
void PPU_writeWindow2PositionRight(u8 val) {
	window.W2_RIGHT = val;
}

void PPU_writeWindowEnableBG12(u8 val) {
	//	window enabled?
	window.w1en[0] = (val >> 1) & 1;
	window.w2en[0] = (val >> 3) & 1;
	window.w1en[1] = (val >> 5) & 1;
	window.w2en[1] = (val >> 7) & 1;
	//	invert signal? (this is used to determine inside/outside window setting)
	window.w1IO[0] = (val >> 0) & 1;
	window.w2IO[0] = (val >> 2) & 1;
	window.w1IO[1] = (val >> 4) & 1;
	window.w2IO[1] = (val >> 6) & 1;
}
void PPU_writeWindowEnableBG34(u8 val) {
	//	window enabled?
	window.w1en[2] = (val >> 1) & 1;
	window.w2en[2] = (val >> 3) & 1;
	window.w1en[3] = (val >> 5) & 1;
	window.w2en[3] = (val >> 7) & 1;
	//	invert signal? (this is used to determine inside/outside window setting)
	window.w1IO[2] = (val >> 0) & 1;
	window.w2IO[2] = (val >> 2) & 1;
	window.w1IO[3] = (val >> 4) & 1;
	window.w2IO[3] = (val >> 6) & 1;
}
void PPU_writeWindowEnableBGOBJSEL(u8 val) {
	//	window enabled?
	window.w1en[4] = (val >> 1) & 1;
	window.w2en[4] = (val >> 3) & 1;
	window.w1en[5] = (val >> 5) & 1;
	window.w2en[5] = (val >> 7) & 1;
	//	invert signal? (this is used to determine inside/outside window setting)
	window.w1IO[4] = (val >> 0) & 1;
	window.w2IO[4] = (val >> 2) & 1;
	window.w1IO[5] = (val >> 4) & 1;
	window.w2IO[5] = (val >> 6) & 1;
}

void PPU_writeWindowMaskLogicBGs(u8 val) {
	//	BG1
	switch (val & 0b11)	{
	case 0b00: window.winlog[0] = _or; break;
	case 0b01: window.winlog[0] = _and; break;
	case 0b10: window.winlog[0] = _xor; break;
	case 0b11: window.winlog[0] = _xnor; break;
	default: break;
	}
	//	BG2
	switch ((val >> 2) & 0b11) {
	case 0b00: window.winlog[1] = _or; break;
	case 0b01: window.winlog[1] = _and; break;
	case 0b10: window.winlog[1] = _xor; break;
	case 0b11: window.winlog[1] = _xnor; break;
	default: break;
	}
	//	BG3
	switch ((val >> 4) & 0b11) {
	case 0b00: window.winlog[2] = _or; break;
	case 0b01: window.winlog[2] = _and; break;
	case 0b10: window.winlog[2] = _xor; break;
	case 0b11: window.winlog[2] = _xnor; break;
	default: break;
	}
	//	BG4
	switch ((val >> 6) & 0b11) {
	case 0b00: window.winlog[3] = _or; break;
	case 0b01: window.winlog[3] = _and; break;
	case 0b10: window.winlog[3] = _xor; break;
	case 0b11: window.winlog[3] = _xnor; break;
	default: break;
	}
}
void PPU_writeWindowMaskLogicOBJSEL(u8 val) {
	//	OBJ
	switch ((val >> 0) & 0b11) {
	case 0b00: window.winlog[4] = _or; break;
	case 0b01: window.winlog[4] = _and; break;
	case 0b10: window.winlog[4] = _xor; break;
	case 0b11: window.winlog[4] = _xnor; break;
	default: break;
	}
	//	SEL/Math/Color
	switch ((val >> 2) & 0b11) {
	case 0b00: window.winlog[5] = _or; break;
	case 0b01: window.winlog[5] = _and; break;
	case 0b10: window.winlog[5] = _xor; break;
	case 0b11: window.winlog[5] = _xnor; break;
	default: break;
	}
}

void PPU_writeMainscreenDisable(u8 val) {
	window.tmw[0] = (val >> 0) & 1;
	window.tmw[1] = (val >> 1) & 1;
	window.tmw[2] = (val >> 2) & 1;
	window.tmw[3] = (val >> 3) & 1;
	window.tmw[4] = (val >> 4) & 1;
}
void PPU_writeSubscreenDisable(u8 val) {
	window.tsw[0] = (val >> 0) & 1;
	window.tsw[1] = (val >> 1) & 1;
	window.tsw[2] = (val >> 2) & 1;
	window.tsw[3] = (val >> 3) & 1;
	window.tsw[4] = (val >> 4) & 1;
}

void PPU_writeSubscreenFixedColor(u8 val) {
	bool r = ((val >> 5) & 1) == 1;
	bool g = ((val >> 6) & 1) == 1;
	bool b = ((val >> 7) & 1) == 1;
	u8 intensity = val & 0b11111;
	if (r) fixedcolor_pixel.color = (fixedcolor_pixel.color & 0b11111'11111'00000'1) | (intensity << 1);
	if (g) fixedcolor_pixel.color = (fixedcolor_pixel.color & 0b11111'00000'11111'1) | (intensity << 6);
	if (b) fixedcolor_pixel.color = (fixedcolor_pixel.color & 0b00000'11111'11111'1) | (intensity << 11);
}

void PPU_writeBGScreenSizeAndBase(u8 bg_id, u8 val) {
	BG_BASE[bg_id] = ((val >> 2) << 10) & 0x7fff;
	switch (val & 0b11) {					//	0 - 32x32, 1 - 64x32, 2 - 32x64, 3 - 64x64
	case 0b00: BG_TILES_H[bg_id] = 32; BG_TILES_V[bg_id] = 32; break;
	case 0b01: BG_TILES_H[bg_id] = 64; BG_TILES_V[bg_id] = 32; break;
	case 0b10: BG_TILES_H[bg_id] = 32; BG_TILES_V[bg_id] = 64; break;
	case 0b11: BG_TILES_H[bg_id] = 64; BG_TILES_V[bg_id] = 64; break;
	}
}

void PPU_writeBGTilebase(u8 bg_id, u8 val) {
	BG_TILEBASE[bg_id] = (val * 0x2000) % 0x10000 / 2;
}

void PPU_writeBGScrollX(u8 bg_id, u8 val) {
	if (!BGSCROLLX_Flipflop[bg_id]) {
		BGSCROLLX[bg_id] = (BGSCROLLX[bg_id] & 0xff00) | val;
		BGSCROLLX_Flipflop[bg_id] = true;
		//printf("Low %x Write BG ScrollX: %x\n", val, BGSCROLLX[bg_id]);
	}
	else {
		BGSCROLLX[bg_id] = (BGSCROLLX[bg_id] & 0xff) | (val << 8);
		BGSCROLLX_Flipflop[bg_id] = false;
		//printf("High %x Write BG ScrollX: %x\n", val, BGSCROLLX[bg_id]);
	}
	//printf("%d\n", BGSCROLLX[bg_id]);
}

void PPU_writeBGScrollY(u8 bg_id, u8 val) {
	if (!BGSCROLLY_Flipflop[bg_id]) {
		BGSCROLLY[bg_id] = (BGSCROLLY[bg_id] & 0xff00) | val;
		BGSCROLLY_Flipflop[bg_id] = true;
	}
	else {
		BGSCROLLY[bg_id] = (BGSCROLLY[bg_id] & 0xff) | (val << 8);
		BGSCROLLY_Flipflop[bg_id] = false;
		//printf("Write BG ScrollY: %x\n", BGSCROLLY[bg_id]);
	}
}

void PPU_setMosaic(u8 val) {
	MOSAIC_ENABLED[0] = val & 1;
	MOSAIC_ENABLED[1] = (val >> 1) & 1;
	MOSAIC_ENABLED[2] = (val >> 2) & 1;
	MOSAIC_ENABLED[3] = (val >> 3) & 1;
	MOSAIC_SIZE = (val >> 4) + 1;
}

void PPU_writeINIDISP(u8 val) {
	MASTER_BRIGHTNESS = (val & 0b1111) / (double)0xf;
	FORCED_BLANKING = val >> 7;
}

void PPU_writeColorMathControlRegisterA(u8 val) {
	switch ((val >> 6) & 0b11) {
	case 0b00: window.mainSW = _never; break;
	case 0b01: window.mainSW = _invert; break;
	case 0b10: window.mainSW = _pass; break;
	case 0b11: window.mainSW = _always; break;
	}
	switch ((val >> 4) & 0b11) {
	case 0b00: window.subSW = _always; break;
	case 0b01: window.subSW = _pass; break;
	case 0b10: window.subSW = _invert; break;
	case 0b11: window.subSW = _never; break;
	}
	window.fixSub = (val >> 1) & 1;
	window.directColor = val & 1;
}

void PPU_writeColorMathControlRegisterB(u8 val) {
	window.add_sub = ((val >> 7) & 1) == 1;
	window.halfEn = ((val >> 6) & 1) == 1;
	window.mathEn[0] = (val & 1) == 1;
	window.mathEn[1] = ((val >> 1) & 1) == 1;
	window.mathEn[2] = ((val >> 2) & 1) == 1;
	window.mathEn[3] = ((val >> 3) & 1) == 1;
	window.mathEn[4] = ((val >> 4) & 1) == 1;
	window.mathEn[5] = ((val >> 5) & 1) == 1;
}

void PPU_writeMainScreenEnabled(u8 val) {
	window.tm[0] = (val >> 0) & 1;
	window.tm[1] = (val >> 1) & 1;
	window.tm[2] = (val >> 2) & 1;
	window.tm[3] = (val >> 3) & 1;
	window.tm[4] = (val >> 4) & 1;
}

void PPU_writeSubScreenEnabled(u8 val) {
	window.ts[0] = (val >> 0) & 1;
	window.ts[1] = (val >> 1) & 1;
	window.ts[2] = (val >> 2) & 1;
	window.ts[3] = (val >> 3) & 1;
	window.ts[4] = (val >> 4) & 1;
}

void PPU_writeBGMode(u8 val) {
	BG_MODE_ID = (val & 0b111);
	BG_MODE = PPU_BG_MODES[BG_MODE_ID];
	BG3_PRIORITY = ((val >> 3) & 1) == 1;
}

//	reading the VBlank NMI Flag automatically aknowledges it
bool PPU_getVBlankNMIFlag() {
	bool res = VBlankNMIFlag;
	VBlankNMIFlag = false;
	return res;
}


//		DEBUG

void debug_drawBG(u8 bg_id) {

	//	find out size
	memset(DEBUG, 0, sizeof(DEBUG));
	SDL_Window* tWindows;
	SDL_Renderer* tRenderer;
	SDL_Texture* tTexture;
	u16 tex_w = (8 * BG_TILES_H[bg_id]);
	u16 tex_h = (8 * BG_TILES_V[bg_id]);

	//	render full size
	if (BG_MODE[bg_id] != PPU_COLOR_DEPTH::CD_DISABLED) {
		for (u16 y = 0; y < tex_h; y++) {
			for (u16 x = 0; x < tex_w; x++) {
				if (BG_MODE[bg_id] == PPU_COLOR_DEPTH::CD_2BPP_4_COLORS)
					renderBGat2BPP(x, y, DEBUG, BG_BASE[bg_id], BG_TILES_H[bg_id], BG_TILES_V[bg_id], BG_TILEBASE[bg_id], 0, 0, BG_TILES_H[bg_id] * 8, BG_PALETTE_OFFSET[BG_MODE_ID][bg_id]);
				else if (BG_MODE[bg_id] == PPU_COLOR_DEPTH::CD_4BPP_16_COLORS)
					renderBGat4BPP(x, y, DEBUG, BG_BASE[bg_id], BG_TILES_H[bg_id], BG_TILES_V[bg_id], BG_TILEBASE[bg_id], 0, 0, BG_TILES_H[bg_id] * 8, BG_PALETTE_OFFSET[BG_MODE_ID][bg_id]);
				else if (BG_MODE[bg_id] == PPU_COLOR_DEPTH::CD_8BPP_256_COLORS)
					renderBGat8BPP(x, y, DEBUG, BG_BASE[bg_id], BG_TILES_H[bg_id], BG_TILES_V[bg_id], BG_TILEBASE[bg_id], 0, 0, BG_TILES_H[bg_id] * 8, BG_PALETTE_OFFSET[BG_MODE_ID][bg_id]);
			}
		}
	}

	//	create window
	tWindows = SDL_CreateWindow("poop", SDL_WINDOWPOS_CENTERED, 100, tex_w, tex_h, SDL_WINDOW_SHOWN);
	tRenderer = SDL_CreateRenderer(tWindows, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	SDL_SetWindowSize(tWindows, tex_w * 2, tex_h * 2);

	//	create texture
	tTexture = SDL_CreateTexture(tRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, tex_w, tex_h);

	//	window decorations
	char title[70];
	u8 bpp = 0;
	if (BG_MODE[bg_id] == PPU_COLOR_DEPTH::CD_2BPP_4_COLORS)
		bpp = 2;
	else if (BG_MODE[bg_id] == PPU_COLOR_DEPTH::CD_4BPP_16_COLORS)
		bpp = 4;
	else if (BG_MODE[bg_id] == PPU_COLOR_DEPTH::CD_8BPP_256_COLORS)
		bpp = 8;
	snprintf(title, sizeof title, "[ BG%d | %dx%d | %dBPP | base 0x%04x | tilebase 0x%04x ]", bg_id+1, tex_w, tex_h, bpp, BG_BASE[bg_id], BG_TILEBASE[bg_id] );
	SDL_SetWindowTitle(tWindows, title);

	//	draw texture to renderer
	SDL_UpdateTexture(tTexture, NULL, DEBUG, tex_w * sizeof(u32));
	SDL_RenderCopy(tRenderer, tTexture, NULL, NULL);
	SDL_RenderPresent(tRenderer);

}