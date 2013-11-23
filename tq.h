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

#ifndef TQ_H
#define TQ_H

#define SHOW_TITLESCREEN
#define ENABLE_SOUND
#define ENABLE_MUSIC
//#define DEBUG_SCANLINES

#define TILE_EMPTY				0
#define TILE_PLAYER_LEFT		1		// tiles 1-5
#define TILE_DOOR				6
#define TILE_SPIKES				7
#define TILE_PLAYER_RIGHT		8		// tiles 8-12
#define TILE_HEART				13
#define TILE_HEART_HALF			14
#define TILE_HEART_BIG			15
#define TILE_WALL				16
#define TILE_PRINCESS			17		// tiles 17-18
#define TILE_LADDER				19
#define TILE_PLAYER_CLIMBING	20		// tiles 20-21
#define TILE_KEY				22
#define TILE_GHOST_LEFT			23		// tiles 23-24
#define TILE_GHOST_RIGHT		25		// tiles 25-26
#define TILE_GOLD				27		// tiles 27-28
#define TILE_WYVERN				29		// tiles 29-30
#define TILE_WALL_DARK			31
#define TILE_WYVERN_2ND			32		// wyvern (sprite index 1)
#define TILE_GHOST_LEFT_2ND		33		// ghost (sprite index 2)

#define TIME_SPEEDUP			30

#endif
