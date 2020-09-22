#include "SDL2/include/SDL.h"
#include "bus.h"

bool a1, b1, x1, y_1, select1, start1, up1, down1, left1, right1, shoulder_l1, shoulder_r1;
bool a2, b2, x2, y2, select2, start2, up2, down2, left2, right2, shoulder_l2, shoulder_r2;

bool JOYPAD_AUTOREAD = false;
u8 JOYPAD_AUTOREAD_INDEX = 0;

void resetController1() {
	a1 = 0;
	b1 = 0;
	x1 = 0;
	y_1 = 0;
	shoulder_l1 = 0;
	shoulder_r1 = 0;
	select1 = 0;
	start1 = 0;
	up1 = 0;
	down1 = 0;
	left1 = 0;
	right1 = 0;
}

void resetController2() {
	a2 = 0;
	b2 = 0;
	x2 = 0;
	y2 = 0;
	shoulder_l1 = 0;
	shoulder_r1 = 0;
	select2 = 0;
	start2 = 0;
	up2 = 0;
	down2 = 0;
	left2 = 0;
	right2 = 0;
}

void setController1(uint8_t *SDL_keys) {
	resetController1();
	if (SDL_keys[SDL_SCANCODE_J])
		a1 = 1;
	if (SDL_keys[SDL_SCANCODE_K])
		b1 = 1;
	if (SDL_keys[SDL_SCANCODE_U])
		x1 = 1;
	if (SDL_keys[SDL_SCANCODE_I])
		y_1 = 1;
	if (SDL_keys[SDL_SCANCODE_O])
		shoulder_l1 = 1;
	if (SDL_keys[SDL_SCANCODE_P])
		shoulder_r1 = 1;
	if (SDL_keys[SDL_SCANCODE_S])
		down1 = 1;
	if (SDL_keys[SDL_SCANCODE_A])
		left1 = 1;
	if (SDL_keys[SDL_SCANCODE_D])
		right1 = 1;
	if (SDL_keys[SDL_SCANCODE_W])
		up1 = 1;
	if (SDL_keys[SDL_SCANCODE_Q])
		select1 = 1;
	if (SDL_keys[SDL_SCANCODE_E])
		start1 = 1;
}

void setController2(uint8_t* SDL_keys) {
	resetController2();
	if (SDL_keys[SDL_SCANCODE_J])
		a2 = 1;
	if (SDL_keys[SDL_SCANCODE_K])
		b2 = 1;
	if (SDL_keys[SDL_SCANCODE_U])
		x2 = 1;
	if (SDL_keys[SDL_SCANCODE_I])
		y2 = 1;
	if (SDL_keys[SDL_SCANCODE_O])
		shoulder_l2 = 1;
	if (SDL_keys[SDL_SCANCODE_P])
		shoulder_r2 = 1;
	if (SDL_keys[SDL_SCANCODE_S])
		down2 = 1;
	if (SDL_keys[SDL_SCANCODE_A])
		left2 = 1;
	if (SDL_keys[SDL_SCANCODE_D])
		right2 = 1;
	if (SDL_keys[SDL_SCANCODE_W])
		up2 = 1;
	if (SDL_keys[SDL_SCANCODE_Q])
		select2 = 1;
	if (SDL_keys[SDL_SCANCODE_E])
		start2 = 1;
}

u16 readController1() {

	return (b1 << 15) | (y_1 << 14) | (select1 << 13) | (start1 << 12) | (up1 << 11) | (down1 << 10) | (left1 << 9) |
		(right1 << 8) | (a1 << 7) | (x1 << 6) | (shoulder_l1 << 5) | (shoulder_r1 << 4);

}

u16 readController2() {

	return (b2 << 15) | (y2 << 14) | (select2 << 13) | (start2 << 12) | (up2 << 11) | (down2 << 10) | (left2 << 9) |
		(right2 << 8) | (a2 << 7) | (x2 << 6) | (shoulder_l2 << 5) | (shoulder_r2 << 4);

}

void INPUT_setJoypadAutoread(bool v) {
	JOYPAD_AUTOREAD = v;
};

bool INPUT_getJoypadAutoread() {
	return JOYPAD_AUTOREAD;
}

void INPUT_stepJoypadAutoread() {
	//	starting autoread, set flag in $4212
	/*if (JOYPAD_AUTOREAD_INDEX == 0) {
		u8 v = (readFromMem(0x4212) & 0xfe) | 1;
		writeToMem(v, 0x4212);
	}*/
	//	for complexitys sake, we immediately read in the 16 bits to the registers, and skip the serial transmission
	BUS_writeToMem(readController1() & 0xff, 0x4218);
	BUS_writeToMem(readController1() >> 8, 0x4219);
	BUS_writeToMem(readController2() & 0xff, 0x421a);
	BUS_writeToMem(readController2() >> 8, 0x421b);
	//	TODO - player 3 and 4
	/*writeToMem(readController3() & 0xff, 0x421c);
	writeToMem(readController3() >> 8, 0x421d);
	writeToMem(readController4() & 0xff, 0x421e);
	writeToMem(readController4() >> 8, 0x421f);*/
};