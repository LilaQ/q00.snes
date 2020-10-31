#pragma once
#ifndef LIB_MAIN
#define LIB_MAIN

#include <string>
#include <chrono>
#include "spc700_cpu.h"
#include "spc700_bus.h"
#include "bus.h"
#include "cpu.h"
#include "ppu.h"
#include "wmu.h"
#include "apu.h"
#include <iostream>
#include <cstdint>
#undef main

int getLastCyc();
void togglePause();
void setPause();
void resetPause();
void frameRendered();

#endif