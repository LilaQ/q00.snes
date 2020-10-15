#include <string>
#include <chrono>
#include <thread>
#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "wmu.h"
#include "apu.h"
#include <iostream>
#include <cstdint>
#include "SDL2/include/SDL_syswm.h"
#include "SDL2/include/SDL.h"
#undef main

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
//std::string filename = "8x8BGMap8BPP32x32.sfc";
//string filename = "8x8BGMap8BPP32x64.sfc";
//string filename = "8x8BGMap8BPP64x32.sfc";
//string filename = "8x8BGMap8BPP64x64.sfc";
//string filename = "8x8BGMap8BPP32x328PAL.sfc";
//string filename = "smw.smc";
//string filename = "snestest.smc";

//string filename = "HiColor575MystSub.sfc";
std::string filename = "translucent.smc";

bool unpaused = true;

int lastcyc = 0;
int ppus = 0;
int frames = 0;

u16 w = 0;
u16 ppu_cycles_left;
u16 delay = 0;


int main()
{

	//	load cartridge
	BUS_loadROM(filename);

	PPU_init(filename);
	initAPU();

	frames = 0;
	auto last_second = std::chrono::high_resolution_clock::now();
	auto last_time = std::chrono::high_resolution_clock::now();

	while (1) {

		if (unpaused) {

			//	FPS counter
			const auto current_time = std::chrono::high_resolution_clock::now();
			const auto second_duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_second).count();
			const float fps = (1000000.0f * float(frames)) / float(second_duration);
			if (second_duration >= 1000000) {
				last_second = current_time;
				frames = 0;
				setFPS(u16(fps));
			}
			last_time = current_time;

			// execute CPU, PPU, APU
			lastcyc = CPU_step();
			ppu_cycles_left += lastcyc * 6;	//	multiply by 6 to go from CPU cycle to master cycle, then divide by 4 to go to dot-cycles
			PPU_step( ppu_cycles_left >> 2);
			ppu_cycles_left -= (ppu_cycles_left >> 2);

			//stepAPU(lastcyc);
			
		}
		if(++delay % 1000 == 0)
			handleWindowEvents();
	}

	return 1;
}

void frameRendered() {
	frames++;
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