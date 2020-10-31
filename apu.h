#pragma once
#ifndef LIB_APU
#define LIB_APU

#include <stdint.h>
void initAPU();
void stepAPU(unsigned char cycles);
void resetSC1length(uint8_t val);
void resetSC2length(uint8_t val);
void resetSC3length(uint8_t val);
void resetSC4length(uint8_t val);
void resetSC1Envelope();
void resetSC2Envelope();
void resetSC1Sweep();
void resetSC2Sweep();
void resetSC1hi();
void resetSC2hi();
void resetSC3hi();
void resetSC4hi();
void resetSC3linearReload();
void resetSC1Ctrl();
void resetSC4Ctrl();
void resetChannelEnables();

#endif