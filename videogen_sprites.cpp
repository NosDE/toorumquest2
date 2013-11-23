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

// Render mode with tiles and sprites
//
// Basic idea:
// 1) copy background tiles from flash to ram for one scanline
// 2) blit sprites on top
// 3) output the scanline
//
// This has to be done while outputting the previously computed scanline.
// Output a pixel every 6th cycle.
//
// There is not enough time to do all this on a single scanline, so we split the work across two scanlines:
// On even scanlines, we write 9 tiles (9*8 pixels) to linebuf2 while outputting pixels from linebuf1 every 6th cycle.  
// On odd scanlines, we write remaining 4 tiles (4*8 pixels) to linebuf2, process three sprites, and output pixels from linebuf1.
// Linebuf1 and linebuf2 are then swapped and process repeats for 80*2 scanlines. 

#include <arduino.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "videogen.h"

volatile uint8_t* 	tmap[NUM_TILES_X*NUM_TILES_Y];
volatile uint8_t**	tmapPtr = tmap;
uint8_t	volatile 	tileOffset;						// tile row offset for scanline (0,8,16,24,32,40,48,56)
uint8_t				linebuf[(SCREEN_WIDTH+16)*2];	// +16 is for sprite clipping; 240 bytes
uint8_t*			linebuf1;						// scanline work buffer (src)
uint8_t*			linebuf2;						// scanline work buffer (dst)

// for each scanline stores:
// sprite image ptr, sprite x-coordinate
volatile SpriteLine		spriteBuffer[NUM_SPRITES*SCREEN_HEIGHT+2];	// 438 bytes	// TODO: why +2?
volatile SpriteLine*	spriteBufferPtr = spriteBuffer;

void clearSprites() {
	volatile SpriteLine* buf = spriteBuffer;
	for(uint8_t i = 0; i < NUM_SPRITES*NUM_TILES_Y; i++) {
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
		buf->img = tiles;
		buf->x = 0;
		buf++;
	}
}

void updateSprite(uint8_t sp, uint8_t img, int8_t x, int8_t y) {
	// cull sprite
	if(x <= -8 || x >= SCREEN_WIDTH) {
		img = 0;
		x = 0;
	}

	// left clip
	x += 8;

	uint8_t* p = &tiles[img*64];
	uint8_t h = 8;

	// top clip
	// if(y <= -8 || y >= SCREEN_HEIGHT)
	// 	return;
	// if(y < 0) {
	// 	p -= y*8;
	// 	h += y;
	// 	y = 0;
	// }

	// top clip at y=8
	if(y <= 0 || y >= SCREEN_HEIGHT)
		return;
	if(y < 8) {
		y -= 8;
		p -= y*8;
		h += y;
		y = 8;
	}

	// bottom clip
	h = min(h, SCREEN_HEIGHT - y);

	// hw sprite 3 (player) is only 6 pixels wide, adjust coordinates so that first pixel of sprite data is skipped
	if(sp == 2) {
		x++;
		p++;
	}

	volatile SpriteLine* buf = &spriteBuffer[((uint8_t)y)*NUM_SPRITES + sp];
	for(uint8_t i = 0; i < h; i++) {
		buf->img = p;
		buf->x = x;
		buf += NUM_SPRITES;
		p += 8;
	}
}

void render_tiles_with_sprites_even() {
	__asm__ __volatile__ (
		// X = linebuf1 (src)
		// Y = linebuf2 (dst)
		// Z = tmap
		// [tileOffset] = tile row offset (0,8,16,24,32,40,48,56)

		"movw	r18, r30\n\t"		// r19:r18 = tmap

		"nop\n\t"
		"nop\n\t"

		// copy first 5 tiles from flash to sram
		// output pixels every 6th cycle

	".rept 5\n\t"
		// fetch tile address from tmap, output 3 pixels
		"movw	r30, r18\n\t"		// restore tmap to Z
		"ld		r16, Z+\n\t"		// load tile address to r16, 2c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"ld		r17, Z+\n\t"		// load tile address to r17, 2c
		"movw	r18, r30\n\t"		// r:19:r18 = Z, 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"movw	r30, r16\n\t"		// Z = r17:r16, 1c
		"add	r30, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r31, r1\n\t"		// 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// copy 8 pixels from flash to buf, output 16 pixels
		// X = linebuf1 (src)
		// Y = linebuf2 (dst)
		// Z = tile address
		// 6*8 = 48 cycles
	".rept 8\n\t"
		"lpm	r16, Z+\n\t"		// load pixel from tile, 3c
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c

		"st		Y+, r16\n\t"		// store pixel to buf, 2c
		"nop\n\t"
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c
	".endr\n\t"

	".endr\n\t"

		// 95 pixels outputted at this point
		// output remaining 9 pixels for total 104 pixels

		// fetch tile address from tmap, output 3 pixels
		"movw	r30, r18\n\t"		// restore tmap to Z
		"ld		r16, Z+\n\t"		// load tile address to r16, 2c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"ld		r17, Z+\n\t"		// load tile address to r17, 2c
		"movw	r18, r30\n\t"		// r:19:r18 = Z, 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"movw	r30, r16\n\t"		// Z = r17:r16, 1c
		"add	r30, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r31, r1\n\t"		// 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// output 6 pixels and copy 3 pixels
".rept 3\n\t"
		"lpm	r16, Z+\n\t"		// load pixel from tile, 3c
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c

		"st		Y+, r16\n\t"		// store pixel to buf, 2c
		"nop\n\t"
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c
".endr\n\t"

		// copy remaining 5 pixels for the sixth tile
		"lpm	r0, Z+\n\t"			// load pixel from tile, 3c
		"st		Y+, r0\n\t"			// store pixel to buf, 2c
		"out	%[port],r1\n\t"		// output black
		"lpm	r0, Z+\n\t"			// load pixel from tile, 3c
		"st		Y+, r0\n\t"			// store pixel to buf, 2c
		"lpm	r0, Z+\n\t"			// load pixel from tile, 3c
		"st		Y+, r0\n\t"			// store pixel to buf, 2c
		"lpm	r0, Z+\n\t"			// load pixel from tile, 3c
		"st		Y+, r0\n\t"			// store pixel to buf, 2c
		"lpm	r0, Z+\n\t"			// load pixel from tile, 3c
		"st		Y+, r0\n\t"			// store pixel to buf, 2c

		// all 104 pixels have been outputted
		// 6 tiles (48 pixels) have been copied

		"movw	r26, r18\n\t"		// restore tmap to X
		// copy three more tiles (no time for more)
		// 6 + 8 * 5 = 46 cycles per tile
	".rept 3\n\t"
		"ld		r30, X+\n\t"		// load tile address to ZL, 2c
		"ld		r31, X+\n\t"		// load tile address to ZH, 2c
		"add	r30, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r31, r1\n\t"		// 1c
	".rept 8\n\t"
		"lpm	r0, Z+\n\t"			// load pixel from tile, 3c
		"st		Y+, r0\n\t"			// store pixel to buf, 2c
	".endr\n\t"
	".endr\n\t"

		// total 9 tiles copied, 104 pixels outputted

		:
		: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
		"x" (linebuf1),
		"y" (linebuf2),
		"z" (tmapPtr),
		[tileOffset] "r" (tileOffset)
		: "r0", "r16", "r17", "r18", "r19" // clobbered registers
	);
}

void render_tiles_with_sprites_odd() {
	__asm__ __volatile__ (
		// X = linebuf1 (src)
		// Y = linebuf2 (dst)
		// Z = tmap
		// [tileOffset] = tile row offset (0,8,16,24,32,40,48,56)

		// skip 9 tiles (72 pixels) that we're already copied in render_tiles_with_sprites_even
		// increment Y 72 pixels
		"ldi	r16, 72\n\t"
		"add	r28, r16\n\t"		// Y = Y + 72
		"adc	r29, r1\n\t"

		// offset tmap pointer by 9 tiles (18 bytes)
		"adiw	r30, 18\n\t"		// 2c
		"movw	r18, r30\n\t"		// r19:r18 = tmap
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// copy 4 tiles from flash to sram
		// output pixels every 6th cycle

	".rept 4\n\t"
		// fetch tile address from tmap, output 3 pixels
		"movw	r30, r18\n\t"		// restore tmap to Z
		"ld		r16, Z+\n\t"		// load tile address to r16, 2c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"ld		r17, Z+\n\t"		// load tile address to r17, 2c
		"movw	r18, r30\n\t"		// r:19:r18 = Z, 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"movw	r30, r16\n\t"		// Z = r17:r16, 1c
		"add	r30, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r31, r1\n\t"		// 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// copy 8 pixels from flash to buf, output 16 pixels
		// X = linebuf1 (src)
		// Y = linebuf2 (dst)
		// Z = tile address
		// 6*8 = 48 cycles
	".rept 8\n\t"
		"lpm	r16, Z+\n\t"		// load pixel from tile, 3c
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c

		"st		Y+, r16\n\t"		// store pixel to buf, 2c
		"nop\n\t"
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c
	".endr\n\t"

	".endr\n\t"

		// all tiles have been copied, 77 pixels have been outputted, 27 pixels remaining

		// TODO: interleave first sprite setup with video output

		// rewind Y back to start of line and output 1 pixel
		"ldi	r18, 104+8\n\t"		// +8 for sprite clipping
		"sub	r28, r18\n\t"
		"sbc	r29, r1\n\t"
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"movw	r18, r28\n\t"		// store line start address in r19:r18
		"movw	r30, r26\n\t"		// Z = X
		"nop\n\t"
		"ld		r0, Z+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// load sprite buffer address to X (lo)
		"lds	r26, spriteBufferPtr\n\t"	// 2c
		"nop\n\t"
		"ld		r0, Z+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// load sprite buffer address to X (hi)
		"lds	r27, spriteBufferPtr+1\n\t"	// 2c
		"nop\n\t"
		"ld		r0, Z+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// output 23 pixels
	".rept 23\n\t"
		"nop\n\t"	// cycles free: 3*23 = 69
		"nop\n\t"
		"nop\n\t"
		"ld		r0, Z+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel
	".endr\n\t"

		// do sprites
		// 9 + 8*6 = 57 cycles per sprite

		// === SPRITE 1 ===

		// load sprite image address to Z
		// load sprite x-coordinate to r0 
		"ld		r30, X+\n\t"				// 2c
		"ld		r31, X+\n\t"				// 2c

		"movw	r28, r18\n\t"				// 1c; restore line start address
		"out	%[port],r1\n\t"		// output black

		"ld		r0, X+\n\t"					// 2c; r0 = spriteX

		// load dest address to Y
		"add	r28, r0\n\t"				// 1c;
		"adc	r29, r1\n\t"				// 1c; Y = start of sprite on line

		// do 8 sprite pixels (6 cycles per pixel)
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+0, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+1, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+2, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+3, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+4, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+5, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+6, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+7, r0\n\t"				// 2c

		// === SPRITE 2 ===

		// load sprite image address to Z
		// load sprite x-coordinate to r0 
		"ld		r30, X+\n\t"				// 2c
		"ld		r31, X+\n\t"				// 2c
		"ld		r0, X+\n\t"					// 2c; r0 = spriteX

		// load dest address to Y
		"movw	r28, r18\n\t"				// 1c; restore line start address
		"add	r28, r0\n\t"				// 1c;
		"adc	r29, r1\n\t"				// 1c; Y = start of sprite on line

		// do 8 sprite pixels (6 cycles per pixel)
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+0, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+1, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+2, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+3, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+4, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+5, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+6, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+7, r0\n\t"				// 2c

		/// === SPRITE 3 ===

		// load sprite image address to Z
		// load sprite x-coordinate to r0 
		"ld		r30, X+\n\t"				// 2c
		"ld		r31, X+\n\t"				// 2c
		"ld		r0, X+\n\t"					// 2c; r0 = spriteX

		// load dest address to Y
		"movw	r28, r18\n\t"				// 1c; restore line start address
		"add	r28, r0\n\t"				// 1c;
		"adc	r29, r1\n\t"				// 1c; Y = start of sprite on line

		// sprite 3 (player) is only 6 pixels wide!
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+0, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+1, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+2, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+3, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+4, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+5, r0\n\t"				// 2c
		// "lpm	r0, Z+\n\t"					// 3c
		// "cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		// "std	Y+6, r0\n\t"				// 2c
		// "lpm	r0, Z+\n\t"					// 3c
		// "cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		// "std	Y+7, r0\n\t"				// 2c

		// sprites done!

		// store sprite buffer address
		"sts	spriteBufferPtr, r26\n\t"	// 2c
		"sts	spriteBufferPtr+1, r27\n\t"	// 2c

		:
		: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
		"x" (linebuf1),
		"y" (linebuf2),
		"z" (tmapPtr),
		[tileOffset] "r" (tileOffset)
		: "r0", "r16", "r17", "r18", "r19" // clobbered registers
	);
}

/*
void render_tiles_with_sprites_odd() {
	__asm__ __volatile__ (
		// X = linebuf1 (src)
		// Y = linebuf2 (dst)
		// Z = tmap
		// [tileOffset] = tile row offset (0,8,16,24,32,40,48,56)

		// skip 9 tiles (72 pixels) that we're already copied in render_tiles_with_sprites_even
		// increment Y 72 pixels
		"ldi	r16, 72\n\t"
		"add	r28, r16\n\t"		// Y = Y + 72
		"adc	r29, r1\n\t"

		// offset tmap pointer by 9 tiles (18 bytes)
		"adiw	r30, 18\n\t"		// 2c
		"movw	r18, r30\n\t"		// r19:r18 = tmap
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// copy 4 tiles from flash to sram
		// output pixels every 6th cycle

	".rept 4\n\t"
		// fetch tile address from tmap, output 3 pixels
		"movw	r30, r18\n\t"		// restore tmap to Z
		"ld		r16, Z+\n\t"		// load tile address to r16, 2c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"ld		r17, Z+\n\t"		// load tile address to r17, 2c
		"movw	r18, r30\n\t"		// r:19:r18 = Z, 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"movw	r30, r16\n\t"		// Z = r17:r16, 1c
		"add	r30, %[tileOffset]\n\t"	// Z = Z + offset, 1c
		"adc	r31, r1\n\t"		// 1c
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		// copy 8 pixels from flash to buf, output 16 pixels
		// X = linebuf1 (src)
		// Y = linebuf2 (dst)
		// Z = tile address
		// 6*8 = 48 cycles
	".rept 8\n\t"
		"lpm	r16, Z+\n\t"		// load pixel from tile, 3c
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c

		"st		Y+, r16\n\t"		// store pixel to buf, 2c
		"nop\n\t"
		"ld		r0, X+\n\t"			// load pixel from buf, 2c
		"out	%[port], r0\n\t"	// output pixel, 1c
	".endr\n\t"

	".endr\n\t"

		// all tiles have been copied, 77 pixels have been outputted, 27 pixels remaining

		// TODO: interleave first sprite setup with video output

		// output 26 pixels
	".rept 26\n\t"
		// TODO: load sprite buffer to Y and first sprite data here
		// TODO: load first two pixels from flash for sprite0 here, store in r16 & r17
		"nop\n\t"	// cycles free: 3*26 = 78
		"nop\n\t"
		"nop\n\t"
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel
	".endr\n\t"

		// rewind Y back to start of line and output 1 pixel
		"ldi	r18, 104+8\n\t"		// +8 for sprite clipping
		"sub	r28, r18\n\t"
		"sbc	r29, r1\n\t"
		"ld		r0, X+\n\t"			// read pixel
		"out	%[port], r0\n\t"	// output pixel

		"movw	r18, r28\n\t"		// store line start address in r19:r18
		// load sprite buffer address to X
		"lds	r26, spriteBufferPtr\n\t"	// 2c
		"lds	r27, spriteBufferPtr+1\n\t"	// 2c
		"out	%[port],r1\n\t"		// output black

		// do sprites
		// 9 + 8*6 = 57 cycles per sprite
	".rept 2\n\r"
		// load sprite image address to Z
		// load sprite x-coordinate to r0 
		"ld		r30, X+\n\t"				// 2c
		"ld		r31, X+\n\t"				// 2c
		"ld		r0, X+\n\t"					// 2c; r0 = spriteX

		// load dest address to Y
		"movw	r28, r18\n\t"				// 1c; restore line start address
		"add	r28, r0\n\t"				// 1c;
		"adc	r29, r1\n\t"				// 1c; Y = start of sprite on line

		// do 8 sprite pixels (6 cycles per pixel)
		// optimization ideas:
		// - player sprite could be only 6 pixels wide; saves 12 cycles
		// - remove transparency from enemy sprites; saves 8 cycles
		// - store image for player sprite in ram (copy from flash to ram when image changes); saves 8 cycles
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+0, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+1, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+2, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+3, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+4, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+5, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+6, r0\n\t"				// 2c
		"lpm	r0, Z+\n\t"					// 3c
		"cpse	r0, r1\n\t"					// 1c if no skip, 2c if next instr is skipped
		"std	Y+7, r0\n\t"				// 2c
	".endr\n\t"

		// store sprite buffer address
		"sts	spriteBufferPtr, r26\n\t"	// 2c
		"sts	spriteBufferPtr+1, r27\n\t"	// 2c

 		// cycles remaining
 		// +8 for narrow player sprite
 		// +9 for interleaving first sprite setup with pixel loop
 		// +6 for loading first two sprite pixels to r16 & r17
 		// +50 nops
 		// => total 48 cycles
 		// 50 ok
	 ".rept 50\n\t"
	  	"nop\n\t"
	 ".endr"
		:
		: [port] "i" (_SFR_IO_ADDR(PORT_VID)),
		"x" (linebuf1),
		"y" (linebuf2),
		"z" (tmapPtr),
		[tileOffset] "r" (tileOffset)
		: "r0", "r16", "r17", "r18", "r19" // clobbered registers
	);
}

*/