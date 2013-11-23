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

#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "tq.h"
#include "videogen.h"
#include "tiles.h"
#include "room.h"
#include "gamepad.h"
#include "player.h"
#include "enemy.h"
#include "audio.h"
#include "sfx.h"
#include "playroutine.h"

extern void intro();

void newgame() {
	setVideoMode(VIDMODE_TILES_AND_SPRITES);
	clearRoomState();
	initRoom(0);
	initPlayer();
}

void setup() {
	initAudio();
	initPlayroutine();
	clearSprites();
	initScreen();
	initController();
#ifdef SHOW_TITLESCREEN
	intro();
#endif
	newgame();

	// debug tiles
	//for(int i = 0; i < NUM_TILES_X*NUM_TILES_Y; i++)
	//	tmap[i] = &tiles[i * 64];
}

void updateTiles() {
	static uint8_t frame = 0;
	frame++;

	// update single row per frame
	uint8_t base = (frame & 7) + 1;
	base *= NUM_TILES_X;

	for(uint8_t i = base; i < base + NUM_TILES_X; i++) {
		switch(getTile(i)) {
		case TILE_GOLD:
			setTile(i, TILE_GOLD+1);
			break;

		case TILE_GOLD+1:
			setTile(i, TILE_GOLD);
			break;

		case TILE_HEART:
			setTile(i, TILE_HEART+1);
			break;

		case TILE_HEART+1:
			setTile(i, TILE_HEART);
			break;
			
		case TILE_PRINCESS:
		case TILE_PRINCESS+1:
			static uint8_t princessTimer = 0;
			princessTimer = (princessTimer + 1) & 7;
			setTile(i, princessTimer < 4 ? TILE_PRINCESS : TILE_PRINCESS + 1);
			break;
		}
	}
}

void updateAudio() {
	// mix audio for next frame to be displayed
	// audio buffers will be swapped at the last scanline by vid gen timer interrupt
	mixAudio(audioBufferWritePtr, 263);

#ifdef ENABLE_MUSIC
	updateSounds();
	updatePlayroutine();
	updateEffects();
	updateEnvelopes();
#endif
}

void loop() {
	while(true) {
#ifdef DEBUG_SCANLINES
		int start = scanLine;
#endif

		updateController();	// 3 scanlines

		if(!p.gameover) {
			clearSprites();	// 2 scanlines
			updatePlayer();	// 2 scanlines
			updateTiles();	// 1 scanlines
			updateEnemies();
		} else {
			// game over

			// clear sprites that are on top of game over text
			for(uint8_t y = 5*8; y < 6*8; y++) {
				for(uint8_t sp = 0; sp < NUM_SPRITES; sp++) {
					volatile SpriteLine* buf = &spriteBuffer[y * NUM_SPRITES + sp];
					if(buf->x >= 2*8 && buf->x < 11*8) {
						buf->img = tiles;
						buf->x = 0;
					}
				}
			}

			// score bonus from remaining time
			if(p.gameover == 2) {
				if(p.time >= 256) {
					playSound(SOUND_GOLD);
					p.score += 25;					
					p.time -= 256;
				} else {
					p.time = 0;
				}
			}

			if(controllerState & BUTTON_START) {
				intro();
				newgame();
			}
		}

		updateScoreBar();
		updateAudio();

#ifdef DEBUG_SCANLINES
		// how many scanlines was spent updating the game
		// NTSC vblank is 46 scanlines
		int lines = scanLine - start;
		p.updateScanlines = lines;
		//p.updateScanlines = p.room;
#endif

		waitForVBlank();

		updateAudio();

		waitForVBlank();
	}
}
