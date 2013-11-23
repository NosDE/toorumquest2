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

#ifndef PLAYROUTINE_H
#define PLAYROUTINE_H

#define CHANNELS		OSCILLATORS
#define TRACK_LENGTH	64

// effects
#define NOEFFECT        0
#define SLIDEUP         1
#define SLIDEDOWN       2
#define ARPEGGIO		3
#define DRUM            4		// use noise waveform for first 50hz cycle
#define SLIDE_NOTE		5		// slide to new note
#define VOLUME_UP		6		// slide volume up by param
#define VOLUME_DOWN		7		// slide volume down by param

// arpeggios
#define ARPEGGIO_NONE				0
#define ARPEGGIO_MAJOR				1
#define ARPEGGIO_MINOR				2
#define ARPEGGIO_DOMINANT_SEVENTH	3
#define ARPEGGIO_OCTAVE				4

// Instrument parameters
struct Instrument
{
	uint8_t  waveform;
	uint8_t  attack;
	uint8_t  decay;
	uint8_t  sustain;
	uint8_t  release;
	uint8_t  pulseWidth;
	uint8_t  pulseWidthSpeed;
	uint8_t  vibratoDepth;    // 0-127
	uint8_t  vibratoSpeed;    // 0-127
};

// Channel state, updated at 50hz by playroutine
struct Channel
{
	uint8_t   instrument;   // instrument playing
	uint8_t   note;         // note playing
	uint16_t  frequency;    // base frequency of the note adjusted with slide up/down
	uint8_t   vibratoPhase;
	uint16_t  pulseWidthPhase;

	// effects
	uint8_t	  effect;
	uint16_t  targetFrequency;	// for slide to effect
	uint8_t   arpPhase1;
	uint8_t   arpPhase2;
};

void initPlayroutine();
void updatePlayroutine();
void updateEffects();

extern uint8_t captureChannels;

#endif
