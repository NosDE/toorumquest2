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

#ifndef VIDEO_GEN_H
#define VIDEO_GEN_H

#include <arduino.h>
#include "tvout.h"

#define NUM_TILES_X					13
#define NUM_TILES_Y					10

#define SCREEN_WIDTH				(NUM_TILES_X*8)
#define SCREEN_HEIGHT				(NUM_TILES_Y*8)

#define SCREEN_START				59
#define SCREEN_END					(SCREEN_START+SCREEN_HEIGHT*2)	// exclusive

#define NUM_SPRITES					3
 
#define VIDMODE_TILES_AND_SPRITES	0	// 13 tiles wide with 2 sprites per scanline
#define VIDMODE_INTRO				1	// 14 tiles wide tile only mode
#define VIDMODE_TITLESCREEN			2	// non-tiled mode

void initScreen();
void setVideoMode(uint8_t mode);
void waitForVBlank();

// video interrupt routines
void blank_line();
void active_line_titlescreen();
void active_line_intro();
void active_line_even();
void active_line_odd();
void vsync_line();

// render scanline routines
void render_titlescreen();
void render_tiles_14();
void render_tiles_with_sprites_even();	// even scanlines
void render_tiles_with_sprites_odd(); 	// odd scanlines

struct SpriteLine {
	uint8_t*	img;	// sprite image in flash
	uint8_t		x;
};

extern volatile int 		scanLine;
extern volatile uint8_t* 	tmap[NUM_TILES_X*NUM_TILES_Y];	// 234 bytes
extern volatile uint8_t**	tmapPtr;
extern volatile uint8_t		tileOffset;
extern uint8_t				linebuf[(SCREEN_WIDTH+16)*2];
extern uint8_t*				linebuf1;
extern uint8_t*				linebuf2;
extern volatile SpriteLine	spriteBuffer[NUM_SPRITES*SCREEN_HEIGHT+2];
extern volatile SpriteLine*	spriteBufferPtr;
extern PROGMEM prog_uchar tiles[];

// call at start of new frame
inline void prepareTilesWithSprites() {
	linebuf1 = &linebuf[8];
	linebuf2 = linebuf1 + SCREEN_WIDTH + 16;
	tileOffset = 0;
	spriteBufferPtr = spriteBuffer;
}

inline void setTile(uint8_t i, uint8_t tile) {
	tmap[i] = &tiles[tile * 64];
}

inline void setTile(uint8_t x, uint8_t y, uint8_t tile) {
	tmap[y * NUM_TILES_X + x] = &tiles[tile * 64];
}

inline uint8_t getTile(uint8_t i) {
	return (uint16_t)(tmap[i] - tiles) >> 6; // / 64
}

inline uint8_t getTile(uint8_t x, uint8_t y) {
	return (uint16_t)(tmap[y * NUM_TILES_X + x] - tiles) >> 6; // / 64
}

inline uint8_t sampleTile(int8_t x, int8_t y) {
	// clamp sampling point to screen
	x = constrain(x, 0, SCREEN_WIDTH-1);
	y = constrain(y, 0, SCREEN_HEIGHT-1);
	return getTile((uint8_t)x >> 3, (uint8_t)y >> 3);
}

void clearScreen();
void clearSprites();
void updateSprite(uint8_t sp, uint8_t img, int8_t x, int8_t y);
void drawText(uint8_t x, uint8_t y, const char* text);
uint8_t charToTile(uint8_t ch);

#endif
