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

#ifndef PLAYER_H
#define PLAYER_H

#define MAX_HEALTH	3

struct Player {
	uint8_t		frame;
	int8_t		x, y;
	int8_t		dir;		// -1 = left, 1 = right
	uint8_t		walkPhase;
	uint8_t		room;
	uint16_t	score;
	uint8_t		health;
	uint16_t	time;		// 8.8
	int8_t		vely;		// 3.5
	uint8_t		jumpTimer;
	bool		climbing;
	uint8_t		climbPhase;
	uint8_t		hurtTimer;
	uint8_t		gameover;			// 0 = game not over, 1 = game over, 2 = game won
	uint8_t		updateScanlines;	// DEBUG: this many scanlines is spent in game update
};

void initPlayer();
void updatePlayer();
void hurtPlayer();
void updateScoreBar();

extern Player p;

#endif
