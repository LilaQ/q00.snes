#pragma once
#include <stdint.h>
#include <string>
using namespace::std;
void powerUp();
void reset();
void loadROM(string filename);
unsigned char readFromMem(uint16_t adr);
void writeToMem(uint16_t adr, uint8_t val);
