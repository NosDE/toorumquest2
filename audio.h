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

#ifndef AUDIO_H
#define AUDIO_H

#define OSCILLATORS          4

// waveforms
#define TRIANGLE 0
#define PULSE    1
#define SAWTOOTH 2
#define NOISE    3

struct Oscillator // sizeof == 14
{
	// phase
	uint16_t  phase;
	uint16_t  frequency;

	// waveform
	uint8_t   waveform;
	uint8_t   pulseWidth;   // pulse width in range 0-255
	uint16_t  amp;          // hi8: amplitude in range 0-127, lo8: fractionals for envelope
	int8_t    noise;        // current noise value

	// envelope
	uint8_t   ctrl;         // 0=release, 1=attack, 2=decay, 3=manual volume
	uint8_t   attack;       // attack rate
	uint8_t   decay;        // decay rate
	uint8_t   sustain;      // sustain amplitude in range 0-127
	uint8_t   release;      // release rate
};

extern Oscillator osc[OSCILLATORS];
extern uint16_t noise;	// global noise state

extern volatile uint8_t audioBuffer[263*2];
extern volatile uint8_t* audioBufferWritePtr;
extern volatile uint8_t* audioBufferReadPtr;

void initAudio();
void mixAudio(volatile uint8_t* buf, int numSamples);
void updateEnvelopes();
void resetAudioChannels();

#endif
