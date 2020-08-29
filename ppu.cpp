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
unsigned char *_VRAM[0x4000];	//	16 kbytes
unsigned char pVRAM[0x4000];		
unsigned char OAM[0x100];		//	256 bytes
const int FB_SIZE = 256 * 240 * 4;
SDL_Renderer *renderer, *renderer_nt, *renderer_oam;
SDL_Window *window, *window_nt, *window_oam;
SDL_Texture *texture, *texture_nt, *texture_oam;
unsigned char framebuffer[FB_SIZE];		//	4 bytes per pixel, RGBA24

void initPPU(string filename) {

	/*
		MAIN WINDOW
	*/

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_SetHint(SDL_HINT_RENDER_VSYNC, 0);
	SDL_CreateWindowAndRenderer(256, 240, 0, &window, &renderer);
	SDL_SetWindowSize(window, 512, 480);
	//SDL_RenderSetLogicalSize(renderer, 512, 480);
	SDL_SetWindowResizable(window, SDL_TRUE);

	//	for fast rendering, create a texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 240);

	/*
		INIT WINDOW
	*/
	initWindow(window, filename);

}

/*
	DRAW FRAME
*/
void drawFrame() {
	SDL_UpdateTexture(texture, NULL, framebuffer, 256 * sizeof(unsigned char) * 4);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void stepPPU() {

}
