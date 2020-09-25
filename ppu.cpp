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
const int FB_SIZE = 256 * 256 * 4;
SDL_Renderer* renderer;
SDL_Window* window;
u16 RENDER_X = 0, RENDER_Y = 0;
bool VBlankNMIFlag = 0;

//	textures
SDL_Texture* TEXTURE[4];
SDL_Texture* BACKDROP_TEX;

//	buffers
u16 MAIN_BGS[4][FB_SIZE];		//	BG1, BG2, BG3, BG4 - Main Screen
u16 SUB_BGS[4][FB_SIZE];		//	BG1, BG2, BG3, BG4 - Sub Screen
u16 MAIN_BACKDROP[FB_SIZE];
u16 SUB_BACKDROP[FB_SIZE];
u16 DEBUG[FB_SIZE];
u16 framebuffer[FB_SIZE];

//	BG & SubScreen Enabled Status
bool BG_ENABLED[5] = { false, false, false, false, false };
bool SUB_ENABLED[5] = { false, false, false, false, false };
bool SUB_COLMATH_ENABLED[5] = { false, false, false, false, false };

//	BG Scrolling
bool BGSCROLLX_Flipflop[4] = { false, false, false, false };
bool BGSCROLLY_Flipflop[4] = { false, false, false, false };
u16 BGSCROLLX[4] = { 0, 0, 0, 0 };
u16 BGSCROLLY[4] = { 0, 0, 0, 0 };

//	BG Tilebase
u16 BG_TILEBASE[4] = { 0, 0, 0, 0 };

//	BG Screen base
u16 BG_BASE[4] = { 0, 0, 0, 0 };

//	BG Tilesizes
u8 BG_TILES_H[4] = { 32, 32, 32, 32 };
u8 BG_TILES_V[4] = { 32, 32, 32, 32 };

//	Mosaic
bool MOSAIC_ENABLED[4] = { false, false, false, false };
u8 MOSAIC_SIZE = 0;

//	INIDISP
double MASTER_BRIGHTNESS = 0;
bool FORCED_BLANKING = false;

//	Color Math (REG B)
SDL_BlendMode COLOR_MATH_REG_B_ADD_SUB = SDL_BLENDMODE_ADD;
u8 COLOR_MATH_REG_A_FORCE_MAIN_SCREEN_BLACK = 0;		//	0 - Never, 1 - NotMathWindow, 2 - MathWindow, 3 - Always
u8 COLOR_MATH_REG_A_COLOR_MATH_ENABLE = 0;				//	0 - Always, 1 - MathWindow, 2 - NotMathWindow, 3 - Never
bool COLOR_MATH_REG_A_ENABLE_SUBSCREENS = false;		//	0 - No/Backdrop only, 1 - Yes/Backdrop+BG+OBJ
bool COLOR_MATH_REG_A_DIRECT_COLOR = false;				//	0 - Use Palette, 1 - Direct Color
bool COLOR_MATH_REG_B_HALF_RESULT = false;				//	0 - Don't, 1 - divide result by 2
bool COLOR_MATH_REG_B_ENABLE_BACKDROP = false;			//	0 - Off, 1 - On

void PPU_init(std::string filename) {

	/*
		MAIN WINDOW
	*/

	//	init and create window and renderer
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	SDL_CreateWindowAndRenderer(256, 239, 0, &window, &renderer);
	SDL_SetWindowSize(window, 512, 478);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	TEXTURE[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[2] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[3] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	BACKDROP_TEX = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	SDL_SetTextureBlendMode(TEXTURE[0], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[1], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[2], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[3], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(BACKDROP_TEX, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	initWindow(window, filename);

}

void PPU_setTitle(std::string filename) {
	SDL_SetWindowTitle(window, filename.c_str());
}

void PPU_reset() {
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
	RENDER_X = 0, RENDER_Y = 0;
	VBlankNMIFlag = 0;
	CGRAM_Lsb = 0;
	CGRAM_Flipflop = false;
	memset(framebuffer, 0, sizeof(framebuffer));
	memset(MAIN_BGS[0], 0, sizeof(MAIN_BGS[0]));
	memset(MAIN_BGS[1], 0, sizeof(MAIN_BGS[1]));
	memset(MAIN_BGS[2], 0, sizeof(MAIN_BGS[2]));
	memset(MAIN_BGS[3], 0, sizeof(MAIN_BGS[3]));
	memset(MAIN_BACKDROP, 0, sizeof(MAIN_BACKDROP));
	TEXTURE[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[2] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[3] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	BACKDROP_TEX = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	SDL_SetTextureBlendMode(TEXTURE[0], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[1], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[2], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[3], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(BACKDROP_TEX, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void processMosaic(u16 *BG) {
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
	for (u8 i = 0; i < 4; i++)
		if (MOSAIC_ENABLED[i])
			processMosaic(MAIN_BGS[i]);

	//	Prep Textures
	SDL_UpdateTexture(TEXTURE[0], NULL, MAIN_BGS[0], 256 * sizeof(u16));
	SDL_UpdateTexture(TEXTURE[1], NULL, MAIN_BGS[1], 256 * sizeof(u16));
	SDL_UpdateTexture(TEXTURE[2], NULL, MAIN_BGS[2], 256 * sizeof(u16));
	SDL_UpdateTexture(TEXTURE[3], NULL, MAIN_BGS[3], 256 * sizeof(u16));
	SDL_UpdateTexture(BACKDROP_TEX, NULL, MAIN_BACKDROP, 256 * sizeof(u16));

	//	Backdrop
	SDL_RenderCopy(renderer, BACKDROP_TEX, NULL, NULL);


	//	BGs
	if (BG_ENABLED[3]) {
		SDL_RenderCopy(renderer, TEXTURE[3], NULL, NULL);
		//	SubScreens
		if (COLOR_MATH_REG_A_COLOR_MATH_ENABLE == 0) {	//	TODO - This still has modes "MathWindow" (1) and "NotMathWin" (2) and "Never" (3)
			if (SUB_ENABLED[3] && SUB_COLMATH_ENABLED[3]) {
				SDL_SetTextureBlendMode(TEXTURE[3], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[3], NULL, NULL);
			}
			if (SUB_ENABLED[2] && SUB_COLMATH_ENABLED[3]) {
				SDL_SetTextureBlendMode(TEXTURE[2], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[2], NULL, NULL);
			}
			if (SUB_ENABLED[1] && SUB_COLMATH_ENABLED[3]) {
				SDL_SetTextureBlendMode(TEXTURE[1], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[1], NULL, NULL);
			}
			if (SUB_ENABLED[0] && SUB_COLMATH_ENABLED[3]) {
				SDL_SetTextureBlendMode(TEXTURE[0], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[0], NULL, NULL);
			}
		}
	}
	if (BG_ENABLED[2]) {
		SDL_RenderCopy(renderer, TEXTURE[2], NULL, NULL);
		//	SubScreens
		if (COLOR_MATH_REG_A_COLOR_MATH_ENABLE == 0) {	//	TODO - This still has modes "MathWindow" (1) and "NotMathWin" (2) and "Never" (3)
			if (SUB_ENABLED[3] && SUB_COLMATH_ENABLED[2]) {
				SDL_SetTextureBlendMode(TEXTURE[3], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[3], NULL, NULL);
			}
			if (SUB_ENABLED[2] && SUB_COLMATH_ENABLED[2]) {
				SDL_SetTextureBlendMode(TEXTURE[2], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[2], NULL, NULL);
			}
			if (SUB_ENABLED[1] && SUB_COLMATH_ENABLED[2]) {
				SDL_SetTextureBlendMode(TEXTURE[1], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[1], NULL, NULL);
			}
			if (SUB_ENABLED[0] && SUB_COLMATH_ENABLED[2]) {
				SDL_SetTextureBlendMode(TEXTURE[0], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[0], NULL, NULL);
			}
		}
	}
	if (BG_ENABLED[1]) {
		SDL_RenderCopy(renderer, TEXTURE[1], NULL, NULL);
		//	SubScreens
		if (COLOR_MATH_REG_A_COLOR_MATH_ENABLE == 0) {	//	TODO - This still has modes "MathWindow" (1) and "NotMathWin" (2) and "Never" (3)
			if (SUB_ENABLED[3] && SUB_COLMATH_ENABLED[1]) {
				SDL_SetTextureBlendMode(TEXTURE[3], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[3], NULL, NULL);
			}
			if (SUB_ENABLED[2] && SUB_COLMATH_ENABLED[1]) {
				SDL_SetTextureBlendMode(TEXTURE[2], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[2], NULL, NULL);
			}
			if (SUB_ENABLED[1] && SUB_COLMATH_ENABLED[1]) {
				SDL_SetTextureBlendMode(TEXTURE[1], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[1], NULL, NULL);
			}
			if (SUB_ENABLED[0] && SUB_COLMATH_ENABLED[1]) {
				SDL_SetTextureBlendMode(TEXTURE[0], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[0], NULL, NULL);
			}
		}
	}
	if (BG_ENABLED[0]) {
		SDL_RenderCopy(renderer, TEXTURE[0], NULL, NULL);
		//	SubScreens
		if (COLOR_MATH_REG_A_COLOR_MATH_ENABLE == 0) {	//	TODO - This still has modes "MathWindow" (1) and "NotMathWin" (2) and "Never" (3)
			if (SUB_ENABLED[3] && SUB_COLMATH_ENABLED[0]) {
				SDL_SetTextureBlendMode(TEXTURE[3], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[3], NULL, NULL);
			}
			if (SUB_ENABLED[2] && SUB_COLMATH_ENABLED[0]) {
				SDL_SetTextureBlendMode(TEXTURE[2], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[2], NULL, NULL);
			}
			if (SUB_ENABLED[1] && SUB_COLMATH_ENABLED[0]) {
				SDL_SetTextureBlendMode(TEXTURE[1], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[1], NULL, NULL);
			}
			if (SUB_ENABLED[0] && SUB_COLMATH_ENABLED[0]) {
				SDL_SetTextureBlendMode(TEXTURE[0], COLOR_MATH_REG_B_ADD_SUB);
				SDL_RenderCopy(renderer, TEXTURE[0], NULL, NULL);
			}
		}
	}

	SDL_RenderPresent(renderer);
	
}

void writeToFB(u16 *BG, u16 x, u16 y, u16 width, u16 v) {
	BG[y * width + x] = v;
}

u16 getRGBAFromCGRAM(u32 id, u8 palette, u8 bpp) {
	if (id == 0) return 0x00000000;	//	id 0 = transparency
	return (u16)(CGRAM[(id + palette * (bpp * bpp))] * MASTER_BRIGHTNESS) << 1 | 1;
}

void renderBackdrop(u16 srcx, u16 srcy, u16* BG) {
	writeToFB(BG, srcx, srcy, 256, ((u16)(CGRAM[0x00] * MASTER_BRIGHTNESS) << 1 | 1));
};

void renderBGat2BPP(u16 scrx, u16 scry, u16 *BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u16 texture_width) {
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
	writeToFB(BG, orgx, orgy, texture_width, getRGBAFromCGRAM(v, b_palette_nr, 2));
}

void renderBGat4BPP(u16 scrx, u16 scry, u16* BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u16 texture_width) {
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
	writeToFB(BG, orgx, orgy, texture_width, getRGBAFromCGRAM(v, b_palette_nr, 4));
}

void renderBGat8BPP(u16 scrx, u16 scry, u16* BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u16 tile_base, u16 scroll_x, u16 scroll_y, u16 texture_width) {
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
	writeToFB(BG, orgx, orgy, texture_width, getRGBAFromCGRAM(v, b_palette_nr, 8));
}

void PPU_render() {
	u8 bg_palette_base = 0;

	//	iterate all BGs
	for (u8 bg_id = 0; bg_id < 4; bg_id++) {
		if (BG_ENABLED[bg_id] || SUB_ENABLED[bg_id]) {								//	Check if the BG (1/2/3/4) is enabled
			u8 bg_mode = BUS_readFromMem(0x2105) & 0b111;
			if (PPU_BG_MODES[bg_mode][bg_id] != PPU_COLOR_DEPTH::CD_DISABLED) {
				

				if (PPU_BG_MODES[bg_mode][bg_id] == PPU_COLOR_DEPTH::CD_2BPP_4_COLORS)
					renderBGat2BPP(RENDER_X, RENDER_Y, MAIN_BGS[bg_id], BG_BASE[bg_id], BG_TILES_H[bg_id], BG_TILES_V[bg_id], BG_TILEBASE[bg_id], BGSCROLLX[bg_id], BGSCROLLY[bg_id] + 1, BG_TILES_H[bg_id] * 8);
				else if (PPU_BG_MODES[bg_mode][bg_id] == PPU_COLOR_DEPTH::CD_4BPP_16_COLORS)
					renderBGat4BPP(RENDER_X, RENDER_Y, MAIN_BGS[bg_id], BG_BASE[bg_id], BG_TILES_H[bg_id], BG_TILES_V[bg_id], BG_TILEBASE[bg_id], BGSCROLLX[bg_id], BGSCROLLY[bg_id] + 1, BG_TILES_H[bg_id] * 8);
				else if (PPU_BG_MODES[bg_mode][bg_id] == PPU_COLOR_DEPTH::CD_8BPP_256_COLORS)
					renderBGat8BPP(RENDER_X, RENDER_Y, MAIN_BGS[bg_id], BG_BASE[bg_id], BG_TILES_H[bg_id], BG_TILES_V[bg_id], BG_TILEBASE[bg_id], BGSCROLLX[bg_id], BGSCROLLY[bg_id] + 1, BG_TILES_H[bg_id] * 8);
			}
		}
	}
	renderBackdrop(RENDER_X, RENDER_Y, MAIN_BACKDROP);
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
				printf("VBlank %d\n", vbl++);
			}
			else if (RENDER_Y == 312) {			//	PAL System has 312 lines
				RENDER_Y = 0;
				VBlankNMIFlag = false;
				Interrupts::clear(Interrupts::NMI);
				BUS_resetHDMA();				//	A new frame will begin, we can safely reset our HDMAs now
			}
		}
		if (RENDER_X < 256 && RENDER_Y < 241) {	//	Only render current pixel(s) if we're not in any blanking period
			PPU_render();
		}
		else if (RENDER_X == 0 && RENDER_Y == 241) {	//	Exclude drawing mechanism so every X/Y modification is done by this point
			INPUT_stepJoypadAutoread();
			handleWindowEvents();
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
	}
	//printf("Write CGRAM: %x\n", val);
	
}

u16 PPU_readCGRAM(u8 adr) {
	return CGRAM[adr];
}

void PPU_writeBGScreenSizeAndBase(u8 bg_id, u8 val) {
	BG_BASE[bg_id] = ((val >> 2) << 10) & 0x7fff;
	switch (val & 0b11) {					//	0 - 32x32, 1 - 64x32, 2 - 32x64, 3 - 64x64
	case 0b00: BG_TILES_V[bg_id] = 32; BG_TILES_H[bg_id] = 32; break;
	case 0b01: BG_TILES_V[bg_id] = 64; BG_TILES_H[bg_id] = 32; break;
	case 0b10: BG_TILES_V[bg_id] = 32; BG_TILES_H[bg_id] = 64; break;
	case 0b11: BG_TILES_V[bg_id] = 64; BG_TILES_H[bg_id] = 64; break;
	}
}

void PPU_writeBGTilebase(u8 bg_id, u8 val) {
	BG_TILEBASE[bg_id] = val * 0x1000;
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
	//	TODO
	//	Force Blanking & Brightness
	MASTER_BRIGHTNESS = (val & 0b1111) / (double)0xf;
	FORCED_BLANKING = val >> 7;
}

void PPU_writeColorMathControlRegisterA(u8 val) {
	COLOR_MATH_REG_A_FORCE_MAIN_SCREEN_BLACK = (val >> 6) & 0b11;
	COLOR_MATH_REG_A_COLOR_MATH_ENABLE = (val >> 4) & 0b11;
	COLOR_MATH_REG_A_ENABLE_SUBSCREENS = (val >> 1) & 1;
	COLOR_MATH_REG_A_DIRECT_COLOR = val & 1;
}

void PPU_writeColorMathControlRegisterB(u8 val) {
	COLOR_MATH_REG_B_ADD_SUB = ((val >> 7) & 1) ? SDL_BLENDMODE_SUB : SDL_BLENDMODE_ADD;
	COLOR_MATH_REG_B_HALF_RESULT = (val >> 6) & 1;
	COLOR_MATH_REG_B_ENABLE_BACKDROP = (val >> 5) & 1;

	SUB_COLMATH_ENABLED[0] = val & 1;
	SUB_COLMATH_ENABLED[1] = (val >> 1) & 1;
	SUB_COLMATH_ENABLED[2] = (val >> 2) & 1;
	SUB_COLMATH_ENABLED[3] = (val >> 3) & 1;
	SUB_COLMATH_ENABLED[4] = (val >> 4) & 1;
}

void PPU_writeBGEnabled(u8 val) {
	for(u8 f = 0; f <  5; f++)
		BG_ENABLED[f] = (val >> f) & 1;
}

void PPU_writeSUBEnabled(u8 val) {
	for (u8 f = 0; f < 5; f++)
		SUB_ENABLED[f] = (val >> f) & 1;
}

//	reading the VBlank NMI Flag automatically aknowledges it
bool PPU_getVBlankNMIFlag() {
	bool res = VBlankNMIFlag;
	VBlankNMIFlag = false;
	return res;
}


//		DEBUG

void debug_drawBG(u8 id) {

	//	find out size
	memset(DEBUG, 0, sizeof(DEBUG));
	SDL_Window* tWindows;
	SDL_Renderer* tRenderer;
	SDL_Texture* tTexture;
	const u8 tiles_x = 32 + (BUS_readFromMem(0x2107 + id) & 1) * 32;
	const u8 tiles_y = 32 + ((BUS_readFromMem(0x2107 + id) >> 1) & 1) * 32;
	u16 tex_w = (8 * tiles_x);
	u16 tex_h = (8 * tiles_y);

	//	render full size
	u16 bg_base;
	u8 bg_size_w, bg_size_h;
	u16 tile_base[4] = {
		(u16)((BUS_readFromMem(0x210b) & 0xf) * 0x1000),
		(u16)((BUS_readFromMem(0x210b) >> 4) * 0x1000),
		(u16)((BUS_readFromMem(0x210c) & 0xf) * 0x1000),
		(u16)((BUS_readFromMem(0x210c) >> 4) * 0x1000)
	};
	u8 bg_mode = BUS_readFromMem(0x2105) & 0b111;
	if (PPU_BG_MODES[bg_mode][id] != PPU_COLOR_DEPTH::CD_DISABLED) {
		bg_base = ((BUS_readFromMem(0x2107 + id) >> 2) << 10) & 0x7fff;	//	VRAM start address for rendering
		switch (BUS_readFromMem(0x2107 + id) & 0b11) {					//	0 - 32x32, 1 - 64x32, 2 - 32x64, 3 - 64x64
		case 0b00: bg_size_w = 32; bg_size_h = 32; break;
		case 0b01: bg_size_w = 64; bg_size_h = 32; break;
		case 0b10: bg_size_w = 32; bg_size_h = 64; break;
		case 0b11: bg_size_w = 64; bg_size_h = 64; break;
		}
		for (u16 y = 0; y < tex_h; y++) {
			for (u16 x = 0; x < tex_w; x++) {
				if (PPU_BG_MODES[bg_mode][id] == PPU_COLOR_DEPTH::CD_2BPP_4_COLORS)
					renderBGat2BPP(x, y, DEBUG, bg_base, bg_size_w, bg_size_h, tile_base[id], 0, 0, bg_size_w * 8);
				else if (PPU_BG_MODES[bg_mode][id] == PPU_COLOR_DEPTH::CD_4BPP_16_COLORS)
					renderBGat4BPP(x, y, DEBUG, bg_base, bg_size_w, bg_size_h, tile_base[id], 0, 0, bg_size_w * 8);
				else if (PPU_BG_MODES[bg_mode][id] == PPU_COLOR_DEPTH::CD_8BPP_256_COLORS)
					renderBGat8BPP(x, y, DEBUG, bg_base, bg_size_w, bg_size_h, tile_base[id], 0, 0, bg_size_w * 8);
			}
		}
	}

	//	create window
	tWindows = SDL_CreateWindow("poop", SDL_WINDOWPOS_CENTERED, 100, tex_w, tex_h, SDL_WINDOW_SHOWN);
	tRenderer = SDL_CreateRenderer(tWindows, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	SDL_SetWindowSize(tWindows, tex_w * 2, tex_h * 2);

	//	create texture
	tTexture = SDL_CreateTexture(tRenderer, SDL_PIXELFORMAT_BGRA5551, SDL_TEXTUREACCESS_STREAMING, tex_w, tex_h);

	//	window decorations
	char title[70];
	u8 bpp = 0;
	if (PPU_BG_MODES[bg_mode][id] == PPU_COLOR_DEPTH::CD_2BPP_4_COLORS)
		bpp = 2;
	else if (PPU_BG_MODES[bg_mode][id] == PPU_COLOR_DEPTH::CD_4BPP_16_COLORS)
		bpp = 4;
	else if (PPU_BG_MODES[bg_mode][id] == PPU_COLOR_DEPTH::CD_8BPP_256_COLORS)
		bpp = 8;
	snprintf(title, sizeof title, "[ BG%d | %dx%d | %dBPP | tilebase 0x%04x | palettebase 0x%04x ]", id+1, tex_w, tex_h, bpp, tile_base[id], (0x20 * id) );
	SDL_SetWindowTitle(tWindows, title);

	//	draw texture to renderer
	SDL_UpdateTexture(tTexture, NULL, DEBUG, tex_w * sizeof(u16));
	SDL_RenderCopy(tRenderer, tTexture, NULL, NULL);
	SDL_RenderPresent(tRenderer);

}