#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <algorithm>
#include "SDL2/include/SDL.h"
#include "mmu.h"
#include "ppu.h"
#include "wmu.h"
#include "cpu.h"
#ifdef _WIN32
	#include <Windows.h>
	#include <WinUser.h>
	#include "SDL2/include/SDL_syswm.h"
#endif // _WIN32

using namespace::std;

//	init PPU
u16 VRAM[0x8000];	//	64 kbytes (16bit * 0x8000 [ 32768 ] )
u16 CGRAM[0x100];	//	512 bytes (16bit * 0x100 [ 256 ] ) 
u8 CGRAM_Lsb;
bool CGRAM_Flipflip = false;
const int FB_SIZE = 256 * 256 * 4 * 4;
SDL_Renderer *renderer;
SDL_Window *window;
SDL_Texture *TEXTURE[4];
u8 BGS[4][FB_SIZE];
u8 DEBUG[FB_SIZE];
u8 framebuffer[FB_SIZE];

void initPPU(string filename) {

	/*
		MAIN WINDOW
	*/

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	SDL_CreateWindowAndRenderer(256, 239, 0, &window, &renderer);
	SDL_SetWindowSize(window, 512, 478);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	TEXTURE[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[2] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[3] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	SDL_SetTextureBlendMode(TEXTURE[0], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[1], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[2], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[3], SDL_BLENDMODE_BLEND);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	

	/*
		INIT WINDOW
	*/
	initWindow(window, filename);

}

void setTitle(string filename) {
	SDL_SetWindowTitle(window, filename.c_str());
}

void resetPPU() {
	memset(BGS[0], 0, sizeof(BGS[0]));
	memset(BGS[1], 0, sizeof(BGS[1]));
	memset(BGS[2], 0, sizeof(BGS[2]));
	memset(BGS[3], 0, sizeof(BGS[3]));
	TEXTURE[0] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[1] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[2] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	TEXTURE[3] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);
	SDL_SetTextureBlendMode(TEXTURE[0], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[1], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[2], SDL_BLENDMODE_BLEND);
	SDL_SetTextureBlendMode(TEXTURE[3], SDL_BLENDMODE_BLEND);
}

/*
	DRAW FRAME
*/
void drawFrame() {

	SDL_UpdateTexture(TEXTURE[0], NULL, BGS[0], 256 * sizeof(u8) * 4);
	SDL_UpdateTexture(TEXTURE[1], NULL, BGS[1], 256 * sizeof(u8) * 4);
	SDL_UpdateTexture(TEXTURE[2], NULL, BGS[2], 256 * sizeof(u8) * 4);
	SDL_UpdateTexture(TEXTURE[3], NULL, BGS[3], 256 * sizeof(u8) * 4);

	SDL_RenderCopy(renderer, TEXTURE[0], NULL, NULL);
	SDL_RenderCopy(renderer, TEXTURE[1], NULL, NULL);
	SDL_RenderCopy(renderer, TEXTURE[2], NULL, NULL);
	SDL_RenderCopy(renderer, TEXTURE[3], NULL, NULL);
	SDL_RenderPresent(renderer);
	
}

void writeToFB(u8 *BG, u16 x, u16 y, u32 v) {
	BG[y * 4 * 256 + x * 4] = v >> 24;
	BG[y * 4 * 256 + x * 4 + 1] = v >> 16 & 0xff;
	BG[y * 4 * 256 + x * 4 + 2] = v >> 8 & 0xff;
	BG[y * 4 * 256 + x * 4 + 3] = v & 0xff;
}

u32 getRGBAFromCGRAM(u32 id, u8 palette, u8 palette_base, u8 bpp) {
	u16 color = CGRAM[(id + palette * (bpp*bpp)) + palette_base];
	if (!id)	//	all bits on zero results in using backdrop color
		color = CGRAM[0x00];
	const u8 r = (color & 0b11111) * 255 / 31;
	const u8 g = ((color >> 5) & 0b11111) * 255 / 31;
	const u8 b = ((color >> 10) & 0b11111) * 255 / 31;
	return (r << 24) | (g << 16) | (b << 8) | 0xff;
}

void renderBGat2BPP(u16 scrx, u16 scry, u8 *BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u8 bg_palette_base, u16 scroll_x, u16 scroll_y) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset = (scry / 8) * 32 + (scrx / 8);
	const u16 tile_id = VRAM[bg_base + offset] & 0x3ff;					//	mask bits that are for index
	const u16 tile_address = tile_id * 8;
	const u8 b_palette_nr = (VRAM[bg_base + offset] >> 10) & 0b111;
	const u8 b_priority = (VRAM[bg_base + offset] >> 13) & 1;			//	0 - lower, 1 - higher
	const u8 b_flip_x = (VRAM[bg_base + offset] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[bg_base + offset] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 b_hi = VRAM[tile_address + i] >> 8;
	const u8 b_lo = VRAM[tile_address + i] & 0xff;
	const u8 v = ((b_lo >> (7 - j)) & 1) + (2 * ((b_hi >> (7 - j)) & 1));
	writeToFB(BG, orgx, orgy, getRGBAFromCGRAM(v, b_palette_nr, bg_palette_base, 2));
}

void renderBGat4BPP(u16 scrx, u16 scry, u8* BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u8 bg_palette_base, u16 scroll_x, u16 scroll_y) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset = (scry / 8) * 32 + (scrx / 8);
	const u16 tile_id = VRAM[bg_base + offset] & 0x3ff;					//	mask bits that are for index
	const u16 tile_address = tile_id * 16;
	const u8 b_palette_nr = (VRAM[bg_base + offset] >> 10) & 0b111;
	const u8 b_priority = (VRAM[bg_base + offset] >> 13) & 1;			//	0 - lower, 1 - higher
	const u8 b_flip_x = (VRAM[bg_base + offset] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[bg_base + offset] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u8 b_1 = VRAM[tile_address + i] & 0xff;
	const u8 b_2 = VRAM[tile_address + i] >> 8;
	const u8 b_3 = VRAM[tile_address + i + 8] & 0xff;
	const u8 b_4 = VRAM[tile_address + i + 8] >> 8;
	const u16 v = ((b_1 >> (7 - j)) & 1) + (2 * ((b_2 >> (7 - j)) & 1)) + (4 * ((b_3 >> (7 - j)) & 1)) + (8 * ((b_4 >> (7 - j)) & 1));
	writeToFB(BG, orgx, orgy, getRGBAFromCGRAM(v, b_palette_nr, 0, 4));
}

void renderBGat8BPP(u16 scrx, u16 scry, u8* BG, u16 bg_base, u8 bg_size_w, u8 bg_size_h, u8 bg_palette_base, u16 tile_base, u16 scroll_x, u16 scroll_y) {
	const u16 orgx = scrx;												//	store original x/y position, so we can draw in the FB to it
	const u16 orgy = scry;
	scry = (scry + scroll_y) % (8 * bg_size_h);							//	scroll x and y, and adjust for line/column jumps
	scrx = (scrx + scroll_x) % (8 * bg_size_w);

	const u16 offset = (scry / 8) * 32 + (scrx / 8);
	const u16 tile_base_adr = bg_base + offset;
	const u16 tile_id = VRAM[tile_base_adr] & 0x3ff;					//	mask bits that are for index
	const u8 b_palette_nr = (VRAM[tile_base_adr] >> 10) & 0b111;
	const u8 b_priority = (VRAM[tile_base_adr] >> 13) & 1;				//	0 - lower, 1 - higher
	const u8 b_flip_x = (VRAM[tile_base_adr] >> 14) & 1;				//	0 - normal, 1 - mirror horizontally
	const u8 b_flip_y = (VRAM[tile_base_adr] >> 15) & 1;				//	0 - normal, 1 - mirror vertically
	const u8 i = scry % 8;
	const u8 j = scrx % 8;
	const u16 tile_address = tile_id * 32 + tile_base;
	const u8 b_1 = VRAM[tile_address + i] & 0xff;
	const u8 b_2 = VRAM[tile_address + i] >> 8;
	const u8 b_3 = VRAM[tile_address + i + 8] & 0xff;
	const u8 b_4 = VRAM[tile_address + i + 8] >> 8;
	const u8 b_5 = VRAM[tile_address + i + 16] & 0xff;
	const u8 b_6 = VRAM[tile_address + i + 16] >> 8;
	const u8 b_7 = VRAM[tile_address + i + 24] & 0xff;
	const u8 b_8 = VRAM[tile_address + i + 24] >> 8;
	const u16 v = ((b_1 >> (7 - j)) & 1) + (2 * ((b_2 >> (7 - j)) & 1)) + (4 * ((b_3 >> (7 - j)) & 1)) + (8 * ((b_4 >> (7 - j)) & 1)) + (16 * ((b_5 >> (7 - j)) & 1)) + (32 * ((b_6 >> (7 - j)) & 1)) + (64 * ((b_7 >> (7 - j)) & 1)) + (128 * ((b_8 >> (7 - j)) & 1));
	writeToFB(BG, orgx, orgy, getRGBAFromCGRAM(v, b_palette_nr, 0, 8));
}

void stepPPU() {
	u16 bg_base;
	u8 bg_size_w, bg_size_h, bg_palette_base;
	u16 tile_base[4] = {
		(readFromMem(0x210b) & 0xff) * 0x1000,
		(readFromMem(0x210b) >> 8) * 0x1000,
		(readFromMem(0x210c) & 0xff) * 0x1000,
		(readFromMem(0x210c) >> 8) * 0x1000,
	};
	u16 scroll_x, scroll_y;

	//	iterate all BGs
	for (u8 bg_id = 0; bg_id < 4; bg_id++) {
		if (((readFromMem(0x212c) >> bg_id) & 1) > 0) {							//	Check if the BG (1/2/3/4) is enabled
			u8 bg_mode = readFromMem(0x2105) & 0b111;
			if (BG_MODES[bg_mode][bg_id] != COLOR_DEPTH::CD_DISABLED) {
				scroll_x = readFromMem(0x210d + (2 * bg_id)) & 0x3ff;			//	Scroll-X value for the current BG
				scroll_y = readFromMem(0x210e + (2 * bg_id)) & 0x3ff;			//	Scroll-Y value for the current BG
				scroll_x = 100;
				scroll_y = 100;
				bg_palette_base = 0x20 * bg_id;									//	The offset inside CGRAM
				bg_base = ((readFromMem(0x2107 + bg_id) >> 2) << 10) & 0x7fff;	//	VRAM start address for rendering

				switch (readFromMem(0x2107 + bg_id) & 0b11) {					//	0 - 32x32, 1 - 64x32, 2 - 32x64, 3 - 64x64
				case 0b00: bg_size_w = 32; bg_size_h = 32; break;
				case 0b01: bg_size_w = 64; bg_size_h = 32; break;
				case 0b10: bg_size_w = 32; bg_size_h = 64; break;
				case 0b11: bg_size_w = 64; bg_size_h = 64; break;
				}
				for (u16 y = 0; y < 256; y++) {
					for (u16 x = 0; x < 256; x++) {
						if (BG_MODES[bg_mode][bg_id] == COLOR_DEPTH::CD_2BPP_4_COLORS)
							renderBGat2BPP(x, y, BGS[bg_id], bg_base, bg_size_w, bg_size_h, bg_palette_base, scroll_x, scroll_y);
						else if (BG_MODES[bg_mode][bg_id] == COLOR_DEPTH::CD_4BPP_16_COLORS)
							renderBGat4BPP(x, y, BGS[bg_id], bg_base, bg_size_w, bg_size_h, bg_palette_base, scroll_x, scroll_y);
						else if (BG_MODES[bg_mode][bg_id] == COLOR_DEPTH::CD_8BPP_256_COLORS)
							renderBGat8BPP(x, y, BGS[bg_id], bg_base, bg_size_w, bg_size_h, bg_palette_base, tile_base[bg_id], scroll_x, scroll_y);
					}
				}
			}
		}
	}

	drawFrame();
}

void writeToVRAMlow(u8 val, u16 adr) {
	VRAM[adr & 0x7fff] = (VRAM[adr & 0x7fff] & 0xff00) | val;
}
void writeToVRAMhigh(u8 val, u16 adr) {
	VRAM[adr & 0x7fff] = (VRAM[adr & 0x7fff] & 0xff) | (val << 8);
}

u16 readFromVRAM(u16 adr) {
	return VRAM[adr & 0x7fff];
}

void writeToCGRAM(u8 val, u8 adr) {
	//	if address is even, we just remember the current value
	if (!CGRAM_Flipflip) {
		CGRAM_Lsb = val;
		CGRAM_Flipflip = true;
	}
	//	else concat the remembered value with the current one, and store it to adr-1
	else {
		CGRAM[adr] = (val << 8) | CGRAM_Lsb;
		CGRAM_Flipflip = false;
		writeToMem(readFromMem(0x2121) + 1, 0x2121);
	}
}

u16 readFromCGRAM(u8 adr) {
	return CGRAM[adr];
}



//		DEBUG

void debug_drawBG(u8 id) {

	//	find out size
	memset(DEBUG, 0, sizeof(DEBUG));
	SDL_Window* tWindows;
	SDL_Renderer* tRenderer;
	SDL_Texture* tTexture;
	const u8 tiles_x = 32 + (readFromMem(0x2107 + id) & 1) * 32;
	const u8 tiles_y = 32 + ((readFromMem(0x2107 + id) >> 1) & 1) * 32;
	u16 tex_w = (8 * tiles_x);
	u16 tex_h = (8 * tiles_y);

	//	render full size
	u16 bg_base;
	u8 bg_size_w, bg_size_h, bg_palette_base;
	u16 tile_base[4] = {
		(readFromMem(0x210b) & 0xff) * 0x1000,
		(readFromMem(0x210b) >> 8) * 0x1000,
		(readFromMem(0x210c) & 0xff) * 0x1000,
		(readFromMem(0x210c) >> 8) * 0x1000,
	};
	u8 bg_mode = readFromMem(0x2105) & 0b111;
	if (BG_MODES[bg_mode][id] != COLOR_DEPTH::CD_DISABLED) {
		bg_palette_base = 0x20 * id;									//	The offset inside CGRAM
		bg_base = ((readFromMem(0x2107 + id) >> 2) << 10) & 0x7fff;	//	VRAM start address for rendering
		switch (readFromMem(0x2107 + id) & 0b11) {					//	0 - 32x32, 1 - 64x32, 2 - 32x64, 3 - 64x64
		case 0b00: bg_size_w = 32; bg_size_h = 32; break;
		case 0b01: bg_size_w = 64; bg_size_h = 32; break;
		case 0b10: bg_size_w = 32; bg_size_h = 64; break;
		case 0b11: bg_size_w = 64; bg_size_h = 64; break;
		}
		for (u16 y = 0; y < tex_h; y++) {
			for (u16 x = 0; x < tex_w; x++) {
				if (BG_MODES[bg_mode][id] == COLOR_DEPTH::CD_2BPP_4_COLORS)
					renderBGat2BPP(x, y, DEBUG, bg_base, bg_size_w, bg_size_h, bg_palette_base, 0, 0);
				else if (BG_MODES[bg_mode][id] == COLOR_DEPTH::CD_4BPP_16_COLORS)
					renderBGat4BPP(x, y, DEBUG, bg_base, bg_size_w, bg_size_h, bg_palette_base, 0, 0);
				else if (BG_MODES[bg_mode][id] == COLOR_DEPTH::CD_8BPP_256_COLORS)
					renderBGat8BPP(x, y, DEBUG, bg_base, bg_size_w, bg_size_h, bg_palette_base, tile_base[id], 0, 0);
			}
		}
	}

	//	create window
	SDL_CreateWindowAndRenderer(tex_w, tex_h, 0, &tWindows, &tRenderer);
	SDL_SetWindowSize(tWindows, tex_w * 2, tex_h * 2);

	//	create texture
	tTexture = SDL_CreateTexture(tRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, tex_w, tex_h);

	//	window decorations
	char title[50];
	snprintf(title, sizeof title, "[ BG%d ]", id+1);
	SDL_SetWindowTitle(tWindows, title);

	//	draw texture to renderer
	SDL_UpdateTexture(tTexture, NULL, DEBUG, tex_w * sizeof(u8) * 4);
	SDL_RenderCopy(tRenderer, tTexture, NULL, NULL);
	SDL_RenderPresent(tRenderer);

}