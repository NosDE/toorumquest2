/*
 The following timing definitions and routines are taken from TVout library.

 Copyright (c) 2010 Myles Metzer

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef TVOUT_H
#define TVOUT_H

#define _CYCLES_PER_US				(F_CPU / 1000000)

#define _TIME_HORZ_SYNC				4.7
#define _TIME_VIRT_SYNC				58.85
#define _TIME_ACTIVE				46
#define _CYCLES_VIRT_SYNC			((_TIME_VIRT_SYNC * _CYCLES_PER_US) - 1)
#define _CYCLES_HORZ_SYNC			((_TIME_HORZ_SYNC * _CYCLES_PER_US) - 1)

//Timing settings for NTSC
#define _NTSC_TIME_SCANLINE			63.55
#define _NTSC_TIME_OUTPUT_START		12

#define _NTSC_LINE_FRAME			262
#define _NTSC_LINE_START_VSYNC		0
#define _NTSC_LINE_STOP_VSYNC		3
#define _NTSC_LINE_DISPLAY			216
#define _NTSC_LINE_MID				((_NTSC_LINE_FRAME - _NTSC_LINE_DISPLAY)/2 + _NTSC_LINE_DISPLAY/2)

#define _NTSC_CYCLES_SCANLINE		((_NTSC_TIME_SCANLINE * _CYCLES_PER_US) - 1)
#define _NTSC_CYCLES_OUTPUT_START	((_NTSC_TIME_OUTPUT_START * _CYCLES_PER_US) - 1)

//Timing settings for PAL
#define _PAL_TIME_SCANLINE			64
#define _PAL_TIME_OUTPUT_START		12.5

#define _PAL_LINE_FRAME				312
#define _PAL_LINE_START_VSYNC		0
#define _PAL_LINE_STOP_VSYNC		7
#define _PAL_LINE_DISPLAY			260
#define _PAL_LINE_MID				((_PAL_LINE_FRAME - _PAL_LINE_DISPLAY)/2 + _PAL_LINE_DISPLAY/2)

#define _PAL_CYCLES_SCANLINE		((_PAL_TIME_SCANLINE * _CYCLES_PER_US) - 1)
#define _PAL_CYCLES_OUTPUT_START	((_PAL_TIME_OUTPUT_START * _CYCLES_PER_US) - 1)

// video
#define PORT_VID	PORTD
#define	DDR_VID		DDRD
#define	VID_PIN		7

// sync
#define PORT_SYNC	PORTB
#define DDR_SYNC	DDRB
#define SYNC_PIN	1

void inline wait_until(uint8_t time) {
	__asm__ __volatile__ (
			"subi	%[time], 10\n"
			"sub	%[time], %[tcnt1l]\n\t"
		"100:\n\t"
			"subi	%[time], 3\n\t"
			"brcc	100b\n\t"
			"subi	%[time], 0-3\n\t"
			"breq	101f\n\t"
			"dec	%[time]\n\t"
			"breq	102f\n\t"
			"rjmp	102f\n"
		"101:\n\t"
			"nop\n" 
		"102:\n"
		:
		: [time] "a" (time),
		[tcnt1l] "a" (TCNT1L)
	);
}

#endif
