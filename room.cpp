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
#include "room.h"
#include "roomdat_compressed.h"
#include "enemy.h"
#include "videogen.h"

#define ROOM_SIZE		(NUM_TILES_X*(NUM_TILES_Y-1))
#define NUM_ROOMS		(sizeof(roomadj) / 4)

static uint8_t roomstate[NUM_ROOMS];

// room decompression routines
// room data is compressed with simple RLE
// high nibble of each compressed byte holds row length
// low nibble store 4-bit tile

static const prog_uchar* decompressPtr;
static uint8_t decompressData;
static uint8_t decompressRowLength;

uint8_t decompressByte() {
	if(decompressRowLength == 0) {
		decompressData = pgm_read_byte_near(decompressPtr);
		decompressPtr++;
		decompressRowLength = decompressData >> 4;
		decompressData = pgm_read_byte_near(roomNibbleToByte + (decompressData & 15));
	}
	decompressRowLength--;
	return decompressData;
}

void decompressRoom(uint8_t room) {
	// init decompression state
	decompressPtr = rooms;
	decompressData = 0;
	decompressRowLength = 0;

	// skip rooms
	for(int i = 0; i < room * ROOM_SIZE; i++)
		decompressByte();
}

inline bool isCollectible(uint8_t tile) {
	return tile == TILE_KEY || tile == TILE_HEART || tile == TILE_GOLD || tile == TILE_DOOR;
}

void initRoom(uint8_t room) {
	decompressRoom(room);
	for(uint8_t i = 0; i < NUM_TILES_X*(NUM_TILES_Y-1); i++) {
		uint8_t tile = decompressByte();
		tmap[i+NUM_TILES_X] = &tiles[tile * 64];
	}
	initEnemies();
}

uint8_t getAdjacentRoom(uint8_t room, uint8_t adj) {
	return pgm_read_byte_near(roomadj + room * 4 + adj);
}

void clearRoomState() {
	for(uint8_t i = 0; i < NUM_ROOMS; i++)
		roomstate[i] = 0;
}

void storeRoomState(uint8_t room) {
	// store removed keys, hearts, gold and doors
	// up to 8 items can be stored per room

	decompressRoom(room);
	uint8_t state = 0;
	uint8_t bit = 1;

	for(uint8_t i = 0; i < NUM_TILES_X*(NUM_TILES_Y-1); i++) {
		uint8_t tile = decompressByte();
		if(isCollectible(tile)) {
			if(getTile(i + NUM_TILES_X) == TILE_EMPTY) {
				// tile has been removed
				state |= bit;
			}
			bit <<= 1;
		}
	}

	roomstate[room] = state;
}

void restoreRoomState(uint8_t room) {
	decompressRoom(room);
	uint8_t state = roomstate[room];
	uint8_t bit = 1;

	for(uint8_t i = 0; i < NUM_TILES_X*(NUM_TILES_Y-1); i++) {
		uint8_t tile = decompressByte();
		if(isCollectible(tile)) {
			if(state & bit)
				setTile(i + NUM_TILES_X, TILE_EMPTY);
			bit <<= 1;
		}
	}	
}
