#pragma once
#include <string>
#include "SDL2/include/SDL.h"
void initWindow(SDL_Window *win, std::string filename);
void handleWindowEvents();
void showCGRAMMap();
void showVRAMMap();
