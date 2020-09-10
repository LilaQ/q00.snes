#pragma once
#include <string>
#include "mmu.h"
using namespace::std;
void initPPU(string filename);
void stepPPU();
void writeToVRAM(u16 val, u16 adr);
u16 readFromVRAM(u16 adr);
void drawFrame();
void setTitle(string filename);
u16 readFromCGRAM(u8 adr);
void writeToCGRAM(u8 val, u8 adr);