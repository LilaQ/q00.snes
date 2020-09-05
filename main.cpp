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

//		Kroms tests from https://github.com/PeterLemon/SNES/tree/master/CPUTest/CPU
//string filename = "CPUADC.sfc";		//	-	Passed
//string filename = "CPUAND.sfc";		//	-	Passed
//string filename = "CPUASL.sfc";		//	-	Passed
//string filename = "CPUBIT.sfc";		//	-	Passed
//string filename = "CPUBRA.sfc";		//	-	Passed
//string filename = "CPUCMP.sfc";		//	-	Passed
//string filename = "CPUDEC.sfc";		//	-	Passed
string filename = "CPUEOR.sfc";
//string filename = "CPUJMP.sfc";		//	-	Passed
//string filename = "CPUROL.sfc";		//	-	Passed
//string filename = "CPUROR.sfc";		//	-	Passed
//string filename = "CPUORA.sfc";		//	-	Passed

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