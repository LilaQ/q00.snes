#pragma once
#include <string>
#include "SDL2/include/SDL.h"
using namespace::std;
void initWindow(SDL_Window *win, string filename);
void handleWindowEvents(SDL_Event event);