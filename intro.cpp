/*
 Toorum's Quest II
 Copyright (c) 2013 Petri Hakkinen
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions: 

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include <arduino.h>
#include "tq.h"
#include "gamepad.h"
#include "videogen.h"
#include "playroutine.h"
#include "audio.h"

PROGMEM prog_char intro1[] =
	"OH NO!\n\n"
	"TOORUM'S BE-\n"
	"LOVED PRINCESS\n"
	"ADELA HAS BEEN\n"
	"KIDNAPPED BY\n"
	"EVIL MAHARADJA\n"
	"KOVALSKY!\n";

PROGMEM prog_char intro2[] =
	"HELP TOORUM\n"
	"FIND THE PRIN-\n"
	"CESS BEFORE\n"
	"THE MAHARADJA\n"
	"SEDUCES HER!\n\n"
	"GO!!!";

void updateAudio();

void textWriter(prog_char* text) {
	uint8_t x = 0;
	uint8_t y = 0;
	char ch = 0xff;
	bool slow = true;

	clearScreen();

	while(true) {
		while(true) {
			ch = pgm_read_byte_near(text);
			if(ch != 0) {
				if(ch == '\n') {
					x = 0;
					y++;
				} else {
					uint8_t tile = charToTile(ch);
					setTile(y * 14 + x, tile);
			 		x++;
			 	}
			 	text++;
			}
			if(slow || ch == 0)
				break;
		}

		updateController();
		if(y > 0 && (controllerState & (BUTTON_A|BUTTON_START)) && prevControllerState == 0) {
			if(ch == 0)
				break;
			else
				slow = false;
		}


		for(int i = 0; i < 2; i++) {
			waitForVBlank();
			updateAudio();
		}
	 }
}

void title() {
	setVideoMode(VIDMODE_TITLESCREEN);

	for(int i = 0; i < 25; i++)
		waitForVBlank();

	while(true) {
		updateController();
		if(controllerState & BUTTON_START)
			break;

		waitForVBlank();
		updateAudio();
	 }
}

void intro() {
	title();
	setVideoMode(VIDMODE_INTRO);
	textWriter(intro1);
	textWriter(intro2);
	clearScreen();
}
