#pragma once
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
//string filename = "CPUEOR.sfc";		//	-	Passed
//string filename = "CPUINC.sfc";		//	-	Passed
//string filename = "CPUJMP.sfc";		//	-	Passed
//string filename = "CPULDR.sfc";		//	-	Passed
//string filename = "CPULSR.sfc";		//	-	Passed
//string filename = "CPUMOV.sfc";		//	-	Passed
//string filename = "CPUMSC.sfc";		//	-	Passed (still need to test STP with built-in reset-button though)
//string filename = "CPUORA.sfc";		//	-	Passed
//string filename = "CPUPHL.sfc";		//	-	Passed
//string filename = "CPUPSR.sfc";		//	-	Passed
//string filename = "CPURET.sfc";		//	-	Passed
//string filename = "CPUROL.sfc";		//	-	Passed
//string filename = "CPUROR.sfc";		//	-	Passed
//string filename = "CPUSBC.sfc";		//	-	Passed
//string filename = "CPUSTR.sfc";		//	-	Passed
//string filename = "CPUTRN.sfc";		//	-	Passed

//string filename = "8x8BG1Map2BPP32x328PAL.sfc";
//string filename = "8x8BG2Map2BPP32x328PAL.sfc";
//string filename = "8x8BG3Map2BPP32x328PAL.sfc";
//string filename = "8x8BG4Map2BPP32x328PAL.sfc";
//string filename = "8x8BGMap4BPP32x328PAL.sfc";
string filename = "8x8BGMap8BPP32x32.sfc";
//string filename = "8x8BGMap8BPP32x64.sfc";
//string filename = "8x8BGMap8BPP64x32.sfc";
//string filename = "8x8BGMap8BPP64x64.sfc";
//string filename = "8x8BGMap8BPP32x328PAL.sfc";
//string filename = "smw.smc";

bool unpaused = true;

int lastcyc = 0;
int ppus = 0;

uint16_t w = 0;

int main()
{

	//	load cartridge
	loadROM(filename);

	initPPU(filename);
	initAPU();

	while (1) {

		if (unpaused) {
			lastcyc = stepCPU();
			w += lastcyc;
			if (w > 10000) {
				w = 0;
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