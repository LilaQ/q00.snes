#include <string>
#include "mmu.h"
#include "cpu.h"
#include "ppu.h"
#include "wmu.h"
#include "apu.h"
#include <iostream>
#include <cstdint>
#include "SDL2/include/SDL_syswm.h"
#include "SDL2/include/SDL.h"
#undef main
using namespace::std;

SDL_Event event;					//	Eventhandler for all SDL events
string filename = "CPUAND.sfc";
//string filename = "smw.smc";
bool unpaused = true;

int lastcyc = 0;
int ppus = 0;

int main()
{

	//	load cartridge
	loadROM(filename);

	initPPU(filename);
	initAPU();

	while (1) {

		if (unpaused) {
			lastcyc = stepCPU();
			ppus = lastcyc * 3;
			while (ppus--) {
				stepPPU();
			}
			stepAPU(lastcyc);
		}
		handleWindowEvents(event);
	}

	return 1;
}

int getLastCyc() {
	return lastcyc;
}

void togglePause() {
	unpaused ^= 1;
}

void setPause() {
	unpaused = 0;
}

void resetPause() {
	unpaused = 1;
}