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
#include "sfx.h"
#include "audio.h"
#include "playroutine.h"

#define HZ_TO_FREQ(hz) (65536 * hz / 15735)
#define CHANNELS		1

static uint8_t playingSound = -1;
static uint8_t soundPhase = 0;

struct Sound {
	uint8_t		waveform;
	uint8_t		pulseWidth;
	uint16_t	frequency;
	uint8_t		length;			// in frames
};

PROGMEM Sound sounds[] = {
	{ PULSE, 128, HZ_TO_FREQ(2170),  12 }, // gold
	{ PULSE, 128, HZ_TO_FREQ( 340),  25 }, // jump
	{ NOISE,  64, HZ_TO_FREQ(1000),  10 }, // hurt
	{ NOISE,  64, HZ_TO_FREQ(1500), 150 }, // game over
};

void playSound(uint8_t sound) {
	captureChannels = 1;	// capture channel 1
	playingSound = sound;
	soundPhase = 0;

	for(uint8_t i = 0; i < CHANNELS; i++) {
		if(captureChannels & (1<<i)) {
			Oscillator* o = &osc[i];

			o->frequency = 0;

			if(sound >= 0) {
				Sound* s = &sounds[sound];
				o->waveform = pgm_read_byte_near(&s->waveform);
				o->pulseWidth = pgm_read_byte_near(&s->pulseWidth);
				o->frequency = pgm_read_word_near(&s->frequency);
				o->phase = 0;
				o->amp = 0x7fff;
				o->ctrl = 3;
			}
		}
	}
}

void stopSound() {
	for(uint8_t i = 0; i < CHANNELS; i++)
		if(captureChannels & (1<<i))
			osc[i].frequency = 0;
	playingSound = -1;
	captureChannels = 0;
}

void updateSounds() {
	if(playingSound < 0)
		return;

	Sound* s = &sounds[playingSound];
	soundPhase++;

	for(uint8_t i = 0; i < CHANNELS; i++) {
		if(captureChannels & (1<<i)) {
			Oscillator* o = &osc[i];
			switch(playingSound) {
			case SOUND_GOLD:
				o->amp -= 32000/12;
				if(soundPhase == 6)
					o->frequency = HZ_TO_FREQ(2710);
				break;

			case SOUND_JUMP:
				o->frequency += 20;
				if(soundPhase > 17)
					o->amp -= 32000/8;
				break;

			case SOUND_HURT:
				o->frequency -= 5;
				o->amp -= 32000/10;
				if(soundPhase == 3) {
					o->waveform = PULSE;
					o->frequency = 150;
				}
				break;

			case SOUND_GAME_OVER:
				o->frequency -= 150;
				o->amp -= 32000/150;
				if((soundPhase & 15) == 0)
					o->frequency = HZ_TO_FREQ(1500);
				break;
			}
		}
	}

	// stop sound?
	if(soundPhase >= pgm_read_byte_near(&s->length))
		stopSound();
}
