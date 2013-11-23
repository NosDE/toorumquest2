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

#ifndef ENEMY_H
#define ENEMY_H

#define MAX_ENEMIES	4

#define ENEMY_WYVERN	0
#define ENEMY_GHOST		1

struct Enemy {
	int8_t	x, y;
	int8_t	oy;			// y origin for wyvern
	int8_t	dir;
	uint8_t	frame;
	uint8_t	walkPhase;
	uint8_t sprite;		// sprite index, 0 or 1
	void (*updateFunc)(Enemy* e);
};

void initEnemies();
void updateEnemies();

#endif
