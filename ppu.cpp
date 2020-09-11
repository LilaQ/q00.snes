#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <string>
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
const int FB_SIZE = 256 * 239 * 4;
SDL_Renderer *renderer;
SDL_Window *window;
SDL_Texture *texture;
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
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 239);

	/*
		INIT WINDOW
	*/
	initWindow(window, filename);

}

void setTitle(string filename) {
	SDL_SetWindowTitle(window, filename.c_str());
}

/*
	DRAW FRAME
*/
void drawFrame() {
	SDL_UpdateTexture(texture, NULL, framebuffer, 256 * sizeof(u8) * 4);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	//printf("drawFrame\n");
}

void writeToFB(u16 x, u16 y, u32 v) {
	framebuffer[y * 4 * 256 + x * 4] = v >> 24;
	framebuffer[y * 4 * 256 + x * 4 + 1] = v >> 16 & 0xff;
	framebuffer[y * 4 * 256 + x * 4 + 2] = v >> 8 & 0xff;
	framebuffer[y * 4 * 256 + x * 4 + 3] = v & 0xff;
}

u32 getRGBAFromCGRAM(u8 id, u8 palette) {
	const u8 r = (CGRAM[id + palette * 4] & 0b11111) * 255 / 31;
	const u8 g = ((CGRAM[id + palette * 4] >> 5) & 0b11111) * 255 / 31;
	const u8 b = ((CGRAM[id + palette * 4] >> 10) & 0b11111) * 255 / 31;
	return (r << 24) | (g << 16) | (b << 8) | 0xff;
}

void stepPPU() {
	for (u16 a = 0x0000; a < 0x3c0; a++) {
		if (!a) printf("");
		const u16 tile_id = VRAM[0x7c00 + a] & 0x3ff;				//	mask bits that are for index
		const u16 tile_address = tile_id * 8;
		const u8 b_palette_nr = (VRAM[0x7c00 + a] >> 10) & 0b111;
		const u8 b_priority = (VRAM[0x7c00 + a] >> 13) & 1;			//	0 - lower, 1 - higher
		const u8 b_flip_x = (VRAM[0x7c00 + a] >> 14) & 1;			//	0 - normal, 1 - mirror horizontally
		const u8 b_flip_y = (VRAM[0x7c00 + a] >> 15) & 1;			//	0 - normal, 1 - mirror vertically
		for (int i = 0; i < 8; i++) {
			const u8 b_hi = VRAM[tile_address + i] >> 8;
			const u8 b_lo = VRAM[tile_address + i] & 0xff;
			for (int j = 0; j < 8; j++) {
				const u8 v = ((b_lo >> (7 - j)) & 1) + (2 * ((b_hi >> (7 - j)) & 1));
				writeToFB(a % 32 * 8 + j, a / 32 * 8 + i, getRGBAFromCGRAM(v, b_palette_nr));
			}
		}
	}
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
	if (adr % 2 == 0) {
		CGRAM_Lsb = val;
	}
	//	else concat the remembered value with the current one, and store it to adr-1
	else {
		CGRAM[adr/2] = (val << 8) | CGRAM_Lsb;
	}
}

u16 readFromCGRAM(u8 adr) {
	return CGRAM[adr];
}
