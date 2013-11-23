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
#include "enemy.h"
#include "player.h"
#include "videogen.h"

Enemy	enemies[MAX_ENEMIES];
uint8_t	numEnemies;

// sine table for wyvern movement
static PROGMEM prog_char sintab[] = {
	0,1,2,2,3,4,4,5,6,6,7,7,7,8,8,8,8,8,8,8,7,7,7,6,6,5,4,4,3,2,2,1,0,-1,-2,-2,-3,-4,
	-4,-5,-6,-6,-7,-7,-7,-8,-8,-8,-8,-8,-8,-8,-7,-7,-7,-6,-6,-5,-4,-4,-3,-2,-2,-1
};

void updateWyvern(Enemy* e);
void updateGhost(Enemy* e);

inline bool isObstacle(int8_t x, int8_t y) {
	uint8_t tile = sampleTile(x, y);
	return tile == TILE_WALL || tile == TILE_WALL_DARK || tile == TILE_DOOR;
}

inline bool isWalkable(int8_t x, int8_t y) {
	uint8_t tile = sampleTile(x, y);
	return tile == TILE_WALL || tile == TILE_WALL_DARK || tile == TILE_LADDER;
}

inline bool hitPlayer(Enemy* e) {
	//return p.x + 6 >= e->x && p.y + 7 >= e->y && p.x + 1 <= e->x + 7 && p.y <= e->y + 7;
	return p.x + 5 >= e->x && p.y + 7 >= e->y && p.x + 2 <= e->x + 7 && p.y <= e->y + 7;
}

void initEnemies() {
	numEnemies = 0;

	for(uint8_t y = 0; y < NUM_TILES_Y; y++) {
		for(uint8_t x = 0; x < NUM_TILES_X; x++) {
			uint8_t tile = getTile(x, y);
			if(tile == TILE_WYVERN || tile == TILE_WYVERN_2ND || tile == TILE_GHOST_LEFT || tile == TILE_GHOST_RIGHT || tile == TILE_GHOST_LEFT_2ND) {
				if(numEnemies < MAX_ENEMIES) {
					Enemy* e = &enemies[numEnemies++];
					e->sprite = 0;

					// use sprite index 1?
					if(tile == TILE_WYVERN_2ND) {
						tile = TILE_WYVERN;
						e->sprite = 1;
					}
					if(tile == TILE_GHOST_LEFT_2ND) {
						tile = TILE_GHOST_LEFT;
						e->sprite = 1;
					}

					e->x = x * 8;
					e->y = y * 8;
					e->oy = e->y;
					e->frame = tile;
					e->dir = (tile == TILE_GHOST_LEFT ? -1 : 1);
					e->walkPhase = numEnemies * 123;
					e->updateFunc = (tile == TILE_WYVERN ? updateWyvern : updateGhost);
					setTile(x, y, TILE_EMPTY);
				}
			}
		}
	}

}

void updateEnemies() {
	for(uint8_t i = 0; i < numEnemies; i++) {
		Enemy* e = &enemies[i];
		e->updateFunc(e);
		updateSprite(e->sprite, e->frame, e->x, e->y);
		if(hitPlayer(e))
			hurtPlayer();
	}
}

void updateWyvern(Enemy* e) {
	if(e->walkPhase & 1) {
		uint8_t o = (e->walkPhase>>1) & 63;
		int8_t w = pgm_read_byte_near(sintab + o);
		e->y = e->oy + w;
	}

	e->walkPhase++;
	e->frame = TILE_WYVERN + ((e->walkPhase/4) & 1);
}

void updateGhost(Enemy* e) {
	if(e->walkPhase & 1) {
		// turn around
		if(e->dir < 0) {
			if(e->x == 0 || isObstacle(e->x - 1, e->y + 7) || !isWalkable(e->x - 1, e->y + 8))
				e->dir = 1;
		} else {
			if(e->x == SCREEN_WIDTH-1 || isObstacle(e->x + 8, e->y + 7) || !isWalkable(e->x + 8, e->y + 8))
				e->dir = -1;
		}
		e->x += e->dir;
	}

	e->walkPhase++;

	e->frame = ((e->walkPhase/6) & 1);
	e->frame += (e->dir < 0 ? TILE_GHOST_LEFT : TILE_GHOST_RIGHT);
}
