#pragma once
#ifndef LIB_INPUT
#define LIB_INPUT

#include <stdint.h>
#include "SDL2/include/SDL.h"
#include "bus.h"

uint8_t readController1(uint8_t bit);
uint8_t readController2(uint8_t bit);
void setController1(uint8_t *SDL_keys);

void INPUT_setJoypadAutoread(bool v);
bool INPUT_getJoypadAutoread();

void INPUT_stepJoypadAutoread();

#endif