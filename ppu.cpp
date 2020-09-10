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
u16 VRAM[0x8000];	//	64 kbytes
const int FB_SIZE = 256 * 239 * 4;
SDL_Renderer *renderer, *renderer_nt, *renderer_oam;
SDL_Window *window, *window_nt, *window_oam;
SDL_Texture *texture, *texture_nt, *texture_oam;
u8 framebuffer[FB_SIZE];

void initPPU(string filename) {

	/*
		MAIN WINDOW
	*/

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	SDL_CreateWindowAndRenderer(256, 224, 0, &window, &renderer);
	SDL_SetWindowSize(window, 512, 448);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 224);

	/*
		INIT WINDOW
	*/
	initWindow(window, filename);

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
	framebuffer[y * 4 * 256 + x * 4] = v >> 16;
	framebuffer[y * 4 * 256 + x * 4 + 1] = v >> 8 & 0xff;
	framebuffer[y * 4 * 256 + x * 4 + 2] = v & 0xff;
	framebuffer[y * 4 * 256 + x * 4 + 3] = 0xff;
}

void stepPPU() {
	for (u16 a = 0x0000; a < 0x3a0; a++) {
		u16 tile_id = VRAM[0x7c00 + a];
		u16 tile_address = tile_id * 8;
		for (int i = 0; i < 8; i++) {
			const u8 b_lo = VRAM[tile_address + i] >> 8;
			const u8 b_hi = VRAM[tile_address + i] & 0xff;
			for (int j = 0; j < 8; j++) {
				u8 v = (((b_lo >> (7 - j)) & 1) + (2 * ((b_hi >> (7 - j)) & 1))) > 0;
				writeToFB(a % 32 * 8 + j, a / 32 * 8 + i, (v) ? 0xffffff : 0x0000ff);
			}
		}
	}
}


void writeToVRAM(u16 val, u16 adr) {
	//printf("VRAM Write to %x with val %x\n", adr, val);
	VRAM[adr & 0x7fff] = val;
}

u16 readFromVRAM(u16 adr) {
	return VRAM[adr & 0x7fff];
}
