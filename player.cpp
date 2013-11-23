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
#include "player.h"
#include "gamepad.h"
#include "room.h"
#include "videogen.h"
#include "audio.h"
#include "sfx.h"

Player p;

inline void updateMoving();
inline void updateClimbing();
inline void updatePickup();
inline void updateJumpAndFall();
inline void updateCurrentRoom();
inline void updateHurt();
inline void updateTime();

extern void initRoom(uint8_t room);

uint8_t coordToTileIndex(int8_t x, int8_t y) {
	x = constrain(x, 0, SCREEN_WIDTH-1);
	y = constrain(y, 0, SCREEN_HEIGHT-1);
	uint8_t xx = ((uint8_t)x) >> 3;
	uint8_t yy = ((uint8_t)y) >> 3;
	return yy * NUM_TILES_X + xx;
}

bool isSolid(int8_t x, int8_t y) {
	uint8_t tile = sampleTile(x, y);
	return tile == TILE_WALL || tile == TILE_WALL_DARK || tile == TILE_DOOR;
}

bool canStandOn(int8_t x, int8_t y) {
	uint8_t tile = sampleTile(x, y);
	return tile == TILE_WALL || tile == TILE_WALL_DARK || tile == TILE_DOOR || tile == TILE_LADDER;
}

void initPlayer() {
	p.frame = TILE_PLAYER_RIGHT;
	p.x = 4*8;
	p.y = 4*8;
	//p.x = 8*8;
	//p.y = 6*8;
	p.dir = 1;
	p.walkPhase = 0;
	p.room = 0;
	p.score = 0;
	p.health = MAX_HEALTH;
	p.time = (249<<8) + 255;
	p.vely = 0;
	p.jumpTimer = 0;
	p.climbing = false;
	p.climbPhase = 0;
	p.hurtTimer = 0;
	p.gameover = 0;
}

void updatePlayer() {
	updateMoving();
	updateClimbing();
	updateJumpAndFall();
	updatePickup();
	updateCurrentRoom();
	updateHurt();
	updateTime();
	updateSprite(NUM_SPRITES - 1, p.frame, p.x, p.y);
}

void gameover() {
	playSound(SOUND_GAME_OVER);
	drawText(2, 5, "GAME OVER");
	p.gameover = 1;
}

void winGame() {
	if(p.gameover == 0) {
		drawText(3, 5, "YOU WIN");
		p.gameover = 2;
		p.score += 2000;
	}
}

void hurtPlayer() {
	if(p.hurtTimer > 0)
		return;	// player is invulnerable

	playSound(SOUND_HURT);

	p.hurtTimer = 1;
	p.vely = -70;

	if(p.health > 0) {
		p.health--;

		if(p.score >= 100)
			p.score -= 100;
	}

	if(p.health == 0)
		gameover();
}

inline void updatePickup() {
	for(int8_t dir = -1; dir <= 1; dir += 2) {
		int8_t x = (dir < 0 ? p.x + 1 : p.x + 6);
		int8_t y = p.y + 7;
		uint8_t i = coordToTileIndex(x, y);
		uint8_t t = getTile(i);

		if(i >= NUM_TILES_X) {
			if(t == TILE_GOLD || t == TILE_GOLD+1) {
				// pick up gold
				setTile(i, TILE_EMPTY);
				playSound(SOUND_GOLD);
				p.score += 500;
			}

			if(t == TILE_HEART || t == TILE_HEART+1) {
				// pick up heart
				setTile(i, TILE_EMPTY);
				playSound(SOUND_GOLD);
				p.health = MAX_HEALTH;
				p.score += 100;
			}

			if(t == TILE_KEY) {
				// pick up key
				setTile(i, TILE_EMPTY);
				playSound(SOUND_GOLD);
				for(uint8_t i = 0; i < NUM_TILES_X*NUM_TILES_Y; i++)
					if(getTile(i) == TILE_DOOR)
						setTile(i, TILE_EMPTY);
			}

			if(t == TILE_PRINCESS || t == TILE_PRINCESS+1)
				winGame();
		}
	}
}

inline void updateMoving() {
	// moving
	if(controllerState & BUTTON_LEFT) {
		// move left
		p.dir = -1;
		p.walkPhase++;
		if(!isSolid(p.x, p.y) && !isSolid(p.x, p.y + 7))
			p.x--;
	} else if(controllerState & BUTTON_RIGHT) {
		// move right
		p.dir = 1;
		p.walkPhase++;
		if(!isSolid(p.x + 7, p.y) && !isSolid(p.x + 7, p.y + 7))
			p.x++;
	} else {
		// stand still
		p.walkPhase = 0;
	}

	// animate walking
	p.frame = (p.walkPhase/3) % 5;
	p.frame += (p.dir < 0 ? TILE_PLAYER_LEFT : TILE_PLAYER_RIGHT);		
}

inline void updateClimbing() {
	p.climbing = sampleTile(p.x + 1, p.y + 7) == TILE_LADDER || sampleTile(p.x + 6, p.y + 7) == TILE_LADDER;

	// start climbing up
	if(!p.climbing && (controllerState & BUTTON_UP) != 0 && sampleTile(p.x + 2, p.y - 1) == TILE_LADDER && sampleTile(p.x + 5, p.y - 1) == TILE_LADDER) {
		p.y--;
		p.climbing = true;
	}

	// start climbing down
	if(!p.climbing && (controllerState & BUTTON_DOWN) != 0 && sampleTile(p.x + 2, p.y + 8) == TILE_LADDER && sampleTile(p.x + 5, p.y + 8) == TILE_LADDER) {
		p.y++;
		p.climbing = true;
	}
	
	if(!p.climbing)
		return;

	if(controllerState & BUTTON_UP) {
		p.climbPhase++;
		if(!isSolid(p.x + 1, p.y - 1) && !isSolid(p.x + 6, p.y - 1))
			p.y--;
	} 
	if(controllerState & BUTTON_DOWN) {
		p.climbPhase++;
		if(!isSolid(p.x + 1, p.y + 8) && !isSolid(p.x + 6, p.y + 8))
			p.y++;
	}
	if((controllerState & (BUTTON_UP|BUTTON_DOWN)) == 0 && (controllerState & (BUTTON_LEFT|BUTTON_RIGHT)) != 0)
		p.climbPhase++;

	// animate climbing
	p.frame = TILE_PLAYER_CLIMBING + ((p.climbPhase/5) & 1);

	p.vely = 0;
}

inline void updateJumpAndFall() {
	if(p.climbing)
		return;

	// jump
	bool buttonDown = (controllerState & BUTTON_A) != 0;
	bool buttonPressed = buttonDown && (prevControllerState & BUTTON_A) == 0;
	bool onsolid = canStandOn(p.x + 1, p.y + 8) || canStandOn(p.x + 6, p.y + 8);
	if(buttonPressed && onsolid) {
		playSound(SOUND_JUMP);
		p.vely = -33;
		p.jumpTimer = 0;
	}

	// jump higher if button is held down
	if(p.jumpTimer >= 4 && !buttonDown)
		p.jumpTimer = 128;
	if(p.jumpTimer == 4)
		p.vely = -55;
	if(p.jumpTimer == 5)
		p.vely = -55;

	// jump/fall
	p.y += p.vely>>5;

	// hit obstacle above?
	bool solidAbove = isSolid(p.x + 1, p.y - 1) || isSolid(p.x + 6, p.y - 1);
	if(p.vely < 0 && solidAbove) {
		p.vely = 0;
		p.jumpTimer = 255;
	}

	// end of fall
	onsolid = canStandOn(p.x + 1, p.y + 8) || canStandOn(p.x + 6, p.y + 8);
	if(p.vely > 0 && onsolid) {
		p.vely = 0;
		p.y = p.y & ~7;	// snap player to ground
		p.jumpTimer = 255;
	}

	// fall on spikes
	bool spikes = sampleTile(p.x + 1, p.y + 7) == TILE_SPIKES || sampleTile(p.x + 6, p.y + 7) == TILE_SPIKES;
	if(p.vely > 0 && spikes)
		hurtPlayer();

	// gravity
	if(p.jumpTimer >= 3 && p.vely < 100)
		p.vely += 11;

	p.jumpTimer = min(p.jumpTimer + 1, 128);
}

static void changeRoom(uint8_t room) {
	storeRoomState(p.room);
	p.room = room;
	initRoom(room);
	restoreRoomState(room);
}

inline void updateCurrentRoom() {
	if(p.x <= -8) {
		uint8_t r = getAdjacentRoom(p.room, 0);
		if(r != 0xff) {
			changeRoom(r);
			p.x = SCREEN_WIDTH - 8;
		}
	}

	if(p.x >= SCREEN_WIDTH) {
		uint8_t r = getAdjacentRoom(p.room, 1);
		if(r != 0xff) {
			changeRoom(r);
			p.x = 0;
		}
	}

	if(p.y <= 0) {
		uint8_t r = getAdjacentRoom(p.room, 2);
		if(r != 0xff) {
			changeRoom(r);
			p.y = SCREEN_HEIGHT - 4;	
		}
	}

	if(p.y >= SCREEN_HEIGHT) {
		uint8_t r = getAdjacentRoom(p.room, 3);
		if(r != 0xff) {
			changeRoom(r);
			p.y = 4;
		}
	}
}

inline void updateHurt() {
	if(p.hurtTimer == 0)
		return;

	p.hurtTimer++;
	if(p.hurtTimer > 60)
		p.hurtTimer = 0;

	if((p.hurtTimer & 3) < 2)
		p.frame = TILE_EMPTY;
}

void updateScoreBar() {
	for(uint8_t i = 0; i < NUM_TILES_X; i++)
		setTile(i, TILE_EMPTY);

	// update score
#ifdef DEBUG_SCANLINES
	uint16_t score = p.updateScanlines;
#else
	uint16_t score = p.score;
#endif
	uint16_t s = 10000;
	for(uint8_t i = 0; i < 5; i++) {
		uint8_t n = score / s;
		score -= n * s;
		s /= 10;
		setTile(i, 0, 32 + 34 + n);
	}

	// clear leading zeros
	for(uint8_t i = 0; i < 4; i++) {
		if(getTile(i, 0) == 32 + 34)
			setTile(i, 0, TILE_EMPTY);
		else
			break;
	}

	// update time
	uint16_t time = p.time >> 8;
	if(time >= TIME_SPEEDUP || (p.time & 127) <= 64 || p.gameover) {
		s = 100;
		for(uint8_t i = 0; i < 3; i++) {
			uint8_t n = time / s;
			time -= n * s;
			s /= 10;
			setTile(6 + i, 0, 32 + 34 + n);
		}
	}

	// update hearts
	for(uint8_t i = 0; i < p.health; i++) {
		setTile(12 - i, 0, TILE_HEART);
	}
}

inline void updateTime() {
	uint8_t dt = 8;
	if(p.time < TIME_SPEEDUP<<8)
		dt = 4;

	if(p.time >= dt)
		p.time -= dt;
	else {
		p.time = 0;
		gameover();
	}
}
