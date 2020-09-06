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
string filename = "CPUSTR.sfc";		//	-	Untested
//string filename = "CPUTRN.sfc";		//	-	Untested


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