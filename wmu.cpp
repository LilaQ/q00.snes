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
#include "mmu.h"
#include "input.h"
#include "commctrl.h"
#undef main

using namespace::std;

SDL_Window* mainWindow;				//	Main Window

void initWindow(SDL_Window *win, string filename) {
	mainWindow = win;
	char title[50];
	string rom = filename;
	if (filename.find_last_of("\\") != string::npos)
		rom = filename.substr(filename.find_last_of("\\") + 1);
	snprintf(title, sizeof title, "[ q00.snes ][ rom: %s ]", rom.c_str());
	SDL_SetWindowTitle(mainWindow, title);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mainWindow, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	HMENU hMenuBar = CreateMenu();
	HMENU hFile = CreateMenu();
	HMENU hEdit = CreateMenu();
	HMENU hHelp = CreateMenu();
	HMENU hConfig = CreateMenu();
	HMENU hSound = CreateMenu();
	HMENU hPalettes = CreateMenu();
	HMENU hDebugger = CreateMenu();
	HMENU hSavestates = CreateMenu();
	HMENU hVol = CreateMenu();
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFile, "[ main ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hConfig, "[ config ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hVol, "[ vol ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hSavestates, "[ savestates ]");
	AppendMenu(hMenuBar, MF_STRING, 11, "[ ||> un/pause ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hDebugger, "[ debug ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, "[ help ]");
	AppendMenu(hFile, MF_STRING, 9, "» load rom");
	AppendMenu(hFile, MF_STRING, 7, "» reset");
	AppendMenu(hFile, MF_STRING, 1, "» exit");
	AppendMenu(hHelp, MF_STRING, 3, "» about");
	AppendMenu(hDebugger, MF_STRING, 4, "» CHR table");
	AppendMenu(hDebugger, MF_STRING, 5, "» Nametables");
	AppendMenu(hDebugger, MF_STRING, 10, "» OAM tables");
	AppendMenu(hSavestates, MF_STRING, 12, "» save state");
	AppendMenu(hSavestates, MF_STRING, 13, "» load state");
	AppendMenu(hConfig, MF_POPUP, (UINT_PTR)hSound, "[ sound ]");
	AppendMenu(hSound, MF_STRING, 14, "» disable/enable");
	AppendMenu(hSound, MF_STRING, 15, "» disable/enable SC1");
	AppendMenu(hSound, MF_STRING, 16, "» disable/enable SC2");
	AppendMenu(hSound, MF_STRING, 17, "» disable/enable SC3");
	AppendMenu(hSound, MF_STRING, 18, "» disable/enable SC4");
	AppendMenu(hSound, MF_STRING, 23, "» toggle 8-bit remix mode");
	AppendMenu(hVol, MF_STRING, 24, "» 10%");
	AppendMenu(hVol, MF_STRING, 25, "» 20%");
	AppendMenu(hVol, MF_STRING, 26, "» 30%");
	AppendMenu(hVol, MF_STRING, 27, "» 40%");
	AppendMenu(hVol, MF_STRING, 28, "» 50%");
	AppendMenu(hVol, MF_STRING, 29, "» 60%");
	AppendMenu(hVol, MF_STRING, 30, "» 70%");
	AppendMenu(hVol, MF_STRING, 31, "» 80%");
	AppendMenu(hVol, MF_STRING, 32, "» 90%");
	AppendMenu(hVol, MF_STRING, 33, "» 100%");
	SetMenu(hwnd, hMenuBar);

	//	Enable WM events for SDL Window
	SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
}

void handleWindowEvents(SDL_Event event) {
	//	poll events from menu
	SDL_PollEvent(&event);
	switch (event.type)
	{
		case SDL_SYSWMEVENT:
			if (event.syswm.msg->msg.win.msg == WM_COMMAND) {
				//	Exit
				if (LOWORD(event.syswm.msg->msg.win.wParam) == 1) {
					exit(0);
				}
				//	About
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 3) {
					//showAbout();
				}
				//	Reset
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 7) {
					reset();
				}
				//	Memory Map
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 8) {
					//showMemoryMap();
				}
				//	Load ROM
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 9) {
					char f[100];
					OPENFILENAME ofn;

					ZeroMemory(&f, sizeof(f));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
					ofn.lpstrFilter = "SNES Roms\0*.snes\0";
					ofn.lpstrFile = f;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrTitle = "[ rom selection ]";
					ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

					if (GetOpenFileNameA(&ofn)) {
						loadROM(f);
					}
				}
				//	pause / unpause
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 11) {
					togglePause();
				}
				//	save state
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 12) {
				}
				//	load state
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 13) {
				}
				//	set volume
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 24 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 25 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 26 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 27 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 28 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 29 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 30 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 31 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 32 ||
					LOWORD(event.syswm.msg->msg.win.wParam) == 33
					) {
					//setVolume((float)(LOWORD(event.syswm.msg->msg.win.wParam) - 23) * 0.1);
				}
			}
			//	close a window
			if (event.syswm.msg->msg.win.msg == WM_CLOSE) {
				DestroyWindow(event.syswm.msg->msg.win.hwnd);
				PostMessage(event.syswm.msg->msg.win.hwnd, WM_CLOSE, 0, 0);
			}
			break;
		default:
			break;
	};

	uint8_t* keys = (uint8_t*)SDL_GetKeyboardState(NULL);
	//	pass inputs
	setController1(keys);
	//	pause/unpause
	if (keys[SDL_SCANCODE_SPACE]) {
		togglePause();
		keys[SDL_SCANCODE_SPACE] = 0;
	}
}
