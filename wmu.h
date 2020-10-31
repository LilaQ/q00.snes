#pragma once
#ifndef LIB_WMU
#define LIB_WMU

#define _CRT_SECURE_NO_DEPRECATE
#include "SDL2/include/SDL.h"
#include <string>
#include <string.h>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <WinUser.h>
#include "SDL2/include/SDL_syswm.h"
#endif // _WIN32
#include "commctrl.h"
#include "ppu.h"
#include "cpu.h"
#include "bus.h"
#include "input.h"
#include "commctrl.h"
#undef main

void initWindow(SDL_Window *win, std::string filename);
void handleWindowEvents();
void showCGRAMMap();
void showVRAMMap();
void setFPS(u16 fps);
void setTitle(std::string filename);

#endif