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
#include "wmu.h"
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
	AppendMenu(hMenuBar, MF_STRING, 11, "[ ||> un/pause ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hDebugger, "[ debug ]");
	AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hHelp, "[ help ]");
	AppendMenu(hFile, MF_STRING, 9, "» load rom");
	AppendMenu(hFile, MF_STRING, 7, "» reset");
	AppendMenu(hFile, MF_STRING, 1, "» exit");
	AppendMenu(hHelp, MF_STRING, 3, "» about");
	AppendMenu(hDebugger, MF_STRING, 14, "» CGRAM");
	AppendMenu(hDebugger, MF_STRING, 5, "» VRAM");
	AppendMenu(hDebugger, MF_STRING, 3, "» BG1");
	AppendMenu(hDebugger, MF_STRING, 4, "» BG2");
	AppendMenu(hDebugger, MF_STRING, 6, "» BG3");
	AppendMenu(hDebugger, MF_STRING, 8, "» BG4");
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
				//	Reset
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 7) {
					reset();
				}

				//		DEBUG
				//	BG1
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 3) {
					debug_drawBG(0);
				}
				//	BG2
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 4) {
					debug_drawBG(1);
				}
				//	BG3
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 6) {
					debug_drawBG(2);
				}
				//	BG4
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 8) {
					debug_drawBG(3);
				}
				//	CGRAM Map
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 14) {
					showCGRAMMap();
				}
				//	VRAM Map
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 5) {
					showVRAMMap();
				}



				//	Load ROM
				else if (LOWORD(event.syswm.msg->msg.win.wParam) == 9) {
					char f[100];
					OPENFILENAME ofn;

					ZeroMemory(&f, sizeof(f));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
					ofn.lpstrFilter = "SNES Roms\0*.sfc\0";
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

void showCGRAMMap() {

	SDL_Renderer* renderer;
	SDL_Window* window;

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(730, 480, 0, &window, &renderer);
	SDL_SetWindowSize(window, 730, 480);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	SDL_SetWindowTitle(window, "[ q00.snes ][ cgram map ]");

	HWND hwnd = wmInfo.info.win.window;
	HINSTANCE hInst = wmInfo.info.win.hinstance;
	HWND hScroll = CreateWindow("EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_LEFT | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_MULTILINE | ES_READONLY, 10, 10, 710, 460, hwnd, NULL, hInst, NULL);

	//	MEMDUMP Control
	string s = "Offset      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n\r\n";
	for (int i = 0; i < 0x100; i += 0x8) {
		char title[185];
		snprintf(title, sizeof title, "0x%04x      %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x      |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n", i * 2,
			PPU_readCGRAM(i) & 0xff,
			PPU_readCGRAM(i) >> 8,
			PPU_readCGRAM(i + 1) & 0xff,
			PPU_readCGRAM(i + 1) >> 8,
			PPU_readCGRAM(i + 2) & 0xff,
			PPU_readCGRAM(i + 2) >> 8,
			PPU_readCGRAM(i + 3) & 0xff,
			PPU_readCGRAM(i + 3) >> 8,
			PPU_readCGRAM(i + 4) & 0xff,
			PPU_readCGRAM(i + 4) >> 8,
			PPU_readCGRAM(i + 5) & 0xff,
			PPU_readCGRAM(i + 5) >> 8,
			PPU_readCGRAM(i + 6) & 0xff,
			PPU_readCGRAM(i + 6) >> 8,
			PPU_readCGRAM(i + 7) & 0xff,
			PPU_readCGRAM(i + 7) >> 8,
			((PPU_readCGRAM(i) & 0xff) > 32) ? PPU_readCGRAM(i) & 0xff : '.',
			((PPU_readCGRAM(i) >> 8) > 32) ? PPU_readCGRAM(i) >> 8 : '.',
			((PPU_readCGRAM(i + 1) & 0xff) > 32) ? PPU_readCGRAM(i + 1) & 0xff : '.',
			((PPU_readCGRAM(i + 1) >> 8) > 32) ? PPU_readCGRAM(i + 1) >> 8 : '.',
			((PPU_readCGRAM(i + 2) & 0xff) > 32) ? PPU_readCGRAM(i + 2) & 0xff : '.'
		);
		s.append((string)title);
	}

	const TCHAR* text = s.c_str();
	HDC wdc = GetWindowDC(hScroll);
	HFONT font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	LOGFONT lf;
	GetObject(font, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_LIGHT;
	HFONT boldFont = CreateFontIndirect(&lf);
	SendMessage(hScroll, WM_SETFONT, (WPARAM)boldFont, 60);
	SendMessage(hScroll, WM_SETTEXT, 60, reinterpret_cast<LPARAM>(text));
}

void showVRAMMap() {

	SDL_Renderer* renderer;
	SDL_Window* window;

	//	init and create window and renderer
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(730, 880, 0, &window, &renderer);
	SDL_SetWindowSize(window, 730, 880);
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	SDL_SetWindowTitle(window, "[ q00.snes ][ vram map ]");

	HWND hwnd = wmInfo.info.win.window;
	HINSTANCE hInst = wmInfo.info.win.hinstance;
	HWND hScroll = CreateWindow("EDIT", NULL, WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL | ES_LEFT | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_MULTILINE | ES_READONLY, 10, 10, 710, 860, hwnd, NULL, hInst, NULL);

	//	MEMDUMP Control
	string s = "Offset      00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n\r\n";
	for (int i = 0; i < 0x8000; i += 0x8) {
		char title[85];
		snprintf(title, sizeof title, "0x%04x      %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x     |%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\r\n", i*2,
			PPU_readVRAM(i) & 0xff,
			PPU_readVRAM(i) >> 8,
			PPU_readVRAM(i + 1) & 0xff,
			PPU_readVRAM(i + 1) >> 8,
			PPU_readVRAM(i + 2) & 0xff,
			PPU_readVRAM(i + 2) >> 8,
			PPU_readVRAM(i + 3) & 0xff,
			PPU_readVRAM(i + 3) >> 8,
			PPU_readVRAM(i + 4) & 0xff,
			PPU_readVRAM(i + 4) >> 8,
			PPU_readVRAM(i + 5) & 0xff,
			PPU_readVRAM(i + 5) >> 8,
			PPU_readVRAM(i + 6) & 0xff,
			PPU_readVRAM(i + 6) >> 8,
			PPU_readVRAM(i + 7) & 0xff,
			PPU_readVRAM(i + 7) >> 8,
			((PPU_readVRAM(i) & 0xff) > 32) ? PPU_readVRAM(i) & 0xff : '.',
			((PPU_readVRAM(i) >> 8) > 32) ? PPU_readVRAM(i) >> 8 : '.',
			((PPU_readVRAM(i + 1) & 0xff) > 32) ? PPU_readVRAM(i + 1) & 0xff : '.',
			((PPU_readVRAM(i + 1) >> 8) > 32) ? PPU_readVRAM(i + 1) >> 8 : '.',
			((PPU_readVRAM(i + 2) & 0xff) > 32) ? PPU_readVRAM(i + 2) & 0xff : '.',
			((PPU_readVRAM(i + 2) >> 8) > 32) ? PPU_readVRAM(i + 2) >> 8 : '.',
			((PPU_readVRAM(i + 3) & 0xff) > 32) ? PPU_readVRAM(i + 3) & 0xff : '.',
			((PPU_readVRAM(i + 3) >> 8) > 32) ? PPU_readVRAM(i + 3) >> 8 : '.',
			((PPU_readVRAM(i + 4) & 0xff) > 32) ? PPU_readVRAM(i + 4) & 0xff : '.',
			((PPU_readVRAM(i + 4) >> 8) > 32) ? PPU_readVRAM(i + 4) >> 8 : '.',
			((PPU_readVRAM(i + 5) & 0xff) > 32) ? PPU_readVRAM(i + 5) & 0xff : '.',
			((PPU_readVRAM(i + 5) >> 8) > 32) ? PPU_readVRAM(i + 5) >> 8 : '.',
			((PPU_readVRAM(i + 6) & 0xff) > 32) ? PPU_readVRAM(i + 6) & 0xff : '.',
			((PPU_readVRAM(i + 6) >> 8) > 32) ? PPU_readVRAM(i + 6) >> 8 : '.',
			((PPU_readVRAM(i + 7) & 0xff) > 32) ? PPU_readVRAM(i + 7) & 0xff : '.',
			((PPU_readVRAM(i + 7) >> 8) > 32) ? PPU_readVRAM(i + 7) >> 8 : '.'
		);
		s.append((string)title);
	}

	const TCHAR* text = s.c_str();
	HDC wdc = GetWindowDC(hScroll);
	HFONT font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	LOGFONT lf;
	GetObject(font, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_LIGHT;
	HFONT boldFont = CreateFontIndirect(&lf);
	SendMessage(hScroll, WM_SETFONT, (WPARAM)boldFont, 60);
	SendMessage(hScroll, WM_SETTEXT, 60, reinterpret_cast<LPARAM>(text));
}