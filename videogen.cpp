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
#include <avr/interrupt.h>
#include <avr/io.h>
#include "videogen.h"
#include "tq.h"
#include "audio.h"

#define LINES_PER_FRAME		_NTSC_LINE_FRAME
#define VSYNC_END			_NTSC_LINE_STOP_VSYNC
#define OUTPUT_DELAY		_NTSC_CYCLES_OUTPUT_START

#ifdef SHOW_TITLESCREEN
#include "titlescreen.h"
void* titlescreenPtr = titlescreen;
#else
void* titlescreenPtr = 0;
#endif

volatile int scanLine;
int renderLine;					// for titlescreen mode
void (*interruptRoutine)();		// current scanline interrupt routine
void (*videomode)();

void initScreen() {
	// set pins 0-7 to output mode
	DDR_VID = 0xff;
	PORT_VID = 0;

	DDR_SYNC |= _BV(SYNC_PIN);
	PORT_SYNC |= _BV(SYNC_PIN);
	
	// inverted fast pwm mode on timer 1
	TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11);
	TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
	
	ICR1 = _NTSC_CYCLES_SCANLINE;
	//ICR1 = _PAL_CYCLES_SCANLINE;

	OCR1A = _CYCLES_HORZ_SYNC;

	scanLine = LINES_PER_FRAME+1;
	interruptRoutine = &vsync_line;

	prepareTilesWithSprites();
	setVideoMode(VIDMODE_TILES_AND_SPRITES);

	TIMSK1 = _BV(TOIE1);
	sei();
}

void setVideoMode(uint8_t mode) {
	switch(mode) {
	case VIDMODE_TILES_AND_SPRITES:
		videomode = active_line_even;
		break;

	case VIDMODE_INTRO:
		videomode = active_line_intro;
		break;

	case VIDMODE_TITLESCREEN:
		videomode = active_line_titlescreen;
		break;
	}
}

void waitForVBlank() {
	int stop = (int)SCREEN_END+1;
	while(scanLine != stop);
	while(scanLine == stop);
}

// video signal generation interrupt (timer1 interrupt)
// this will be called every 63.55us (15735.64122738 Hz)
ISR(TIMER1_OVF_vect) {
#ifdef ENABLE_SOUND
	// pull audio from buffer and feed to OCR2A
	OCR2A = audioBufferReadPtr[scanLine-1];

	// 440hz test tone
	//static uint16_t phase = 0;
	//phase += 1832; //65536 * 440 / 15735;
	//OCR2A = phase < 32768 ? 255 : 0;
#endif

	interruptRoutine();
}

void blank_line() {
	if(scanLine == SCREEN_START) {
		renderLine = 0;
		tmapPtr = tmap;
		prepareTilesWithSprites();
		interruptRoutine = videomode;
	}
	else if(scanLine == LINES_PER_FRAME) {
		interruptRoutine = &vsync_line;
	}
	
	scanLine++;
}

void active_line_titlescreen() {
	wait_until(OUTPUT_DELAY);
	render_titlescreen();

	// advance to next row of pixels in titlescreen every other scanline
	if(scanLine & 1)
		renderLine += 128;

	scanLine++;
	if(scanLine == SCREEN_END)
		interruptRoutine = &blank_line;
}

void active_line_intro() {
	wait_until(OUTPUT_DELAY);
	render_tiles_14();

	// advance to next row every other scanline
	if(scanLine & 1) {
		tileOffset += 8;
		if(tileOffset == 64) {
			if(scanLine < SCREEN_START + 9*8*2)
				tmapPtr += 14;	// intro screen is 14 tiles wide!
			tileOffset = 0;
		}
	}

	scanLine++;
	if(scanLine == SCREEN_END)
		interruptRoutine = &blank_line;
}

void active_line_even() {
	wait_until(OUTPUT_DELAY);
	render_tiles_with_sprites_even();

	interruptRoutine = &active_line_odd;

	scanLine++;
	if(scanLine == SCREEN_END)
		interruptRoutine = &blank_line;
}

void active_line_odd() {
	wait_until(OUTPUT_DELAY);
	render_tiles_with_sprites_odd();

	// advance to next row of pixels in tiles
	tileOffset = (tileOffset + 8) & (7*8);
	if(tileOffset == 0)
		tmapPtr += NUM_TILES_X;	// advance to next row of tiles

	// swap line buffer
	uint8_t* tmp = linebuf1;
	linebuf1 = linebuf2;
	linebuf2 = tmp;

	interruptRoutine = &active_line_even;

	scanLine++;
	if(scanLine == SCREEN_END)
		interruptRoutine = &blank_line;
}

void vsync_line() {
	if(scanLine >= LINES_PER_FRAME) {
		OCR1A = _CYCLES_VIRT_SYNC;
		scanLine = 0;

		for(int i = 0; i < SCREEN_WIDTH; i++)
			linebuf2[i] = 0;

		// swap audio buffers
		volatile uint8_t* tmp = audioBufferReadPtr;
		audioBufferReadPtr = audioBufferWritePtr;
		audioBufferWritePtr = tmp;
	}
	else if(scanLine == VSYNC_END) {
		OCR1A = _CYCLES_HORZ_SYNC;
		interruptRoutine = &blank_line;
	}
	scanLine++;
}

void render_titlescreen() {
	__asm__ __volatile__ (
		// Z = Z + Y
		"add	r30,r28\n\t"
		"adc	r31,r29\n\t"

		// output 128 pixels
	".rept 128\n\t"
		"lpm	r16, Z+\n\t"
		"out	%[port],r16\n\t"
		"nop\n\t"
	".endr\n\t"

		// black
		"ldi	r16, 0\n\t"
		"out	%[port],r16\n\t"

		:
		: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
		"y" (renderLine),
		"z" (titlescreenPtr)
		: "r16", "r17" // clobbered registers
	);
}

void render_tiles_14() {
	__asm__ __volatile__ (
		"movw	r26, r28\n\t"	// X=Y
		// X=r27:r26, Y=r29:r28, Z=r31:r30

		// load first tile 
		"ld		r30, X+\n\t"		// 2
		"ld		r31, X+\n\t"		// 2
		// Z = Z + Y
		//"add	r30,r28\n\t"
		//"adc	r31,r29\n\t"
		"add	r30, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r31, r1\n\t"		// 1c

		// do 14 tiles
		// 6 cycles per pixel
	".rept 14\n\t"
		"lpm	r16, Z+\n\t"		// 3c
		"ld		r18, X+\n\t"		// preload next tile (lo), 2c
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		"ld		r19, X+\n\t"		// preload next tile (hi), 2c
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		//"add	r18,r28\n\t"		// increment row, 1c
		//"adc	r19,r29\n\t"		// 1c
		"add	r18, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r19, r1\n\t"		// 1c
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		"nop\n\t"
		"nop\n\t"
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		"nop\n\t"
		"nop\n\t"
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		"nop\n\t"
		"nop\n\t"
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		"nop\n\t"
		"nop\n\t"
		"out	%[port],r16\n\t"	// 1c

		"lpm	r16, Z+\n\t"		// 3c
		"movw	r30, r18\n\t"		// load Z, 1c
		"nop\n\t"
		"out	%[port],r16\n\t"	// 1c
	".endr\n\t"
		
		// black
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"out	%[port],r1\n\t"

		:
		: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
		"y" (tmapPtr),
		[tileOffset] "r" (tileOffset)
		: "r16", "r17", "r18", "r19", "r26", "r27", "r30", "r31" // clobbered registers
	);
}

PROGMEM const prog_char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!&'(),-.0123456789?";

void clearScreen() {
	for(uint8_t i = 0; i < NUM_TILES_X*NUM_TILES_Y; i++)
		setTile(i, 0);
}

void drawText(uint8_t x, uint8_t y, const char* text) {
	uint8_t ox = x;
	while(*text != 0) {
		if(x < NUM_TILES_X && y < NUM_TILES_Y) // && *text != ' ')
			setTile(x, y, charToTile(*text));
		x++;
		if(*text == '\n') {
			x = ox;
			y++;
		}
		text++;
	}
}

uint8_t charToTile(uint8_t ch) {
	for(uint8_t i = 0; i < sizeof(charset); i++)
		if(pgm_read_byte_near(charset + i) == ch)
			return 32 + i;
	return 0;
}
