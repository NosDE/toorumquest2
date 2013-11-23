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
#include "audio.h"
#include "playroutine.h"
#include "tunedat.h"

// frequencies for first 8 octaves (notes C0-B7)
PROGMEM prog_uint16_t pitches[] =
{
	0x0043,0x0046,0x004b,0x004f,0x0054,0x0059,0x005e,0x0064,0x006a,0x0070,0x0077,0x007e,
	0x0086,0x008d,0x0096,0x009f,0x00a8,0x00b2,0x00bd,0x00c8,0x00d4,0x00e1,0x00ee,0x00fc,
	0x010b,0x011b,0x012c,0x013e,0x0151,0x0165,0x017a,0x0191,0x01a9,0x01c2,0x01dd,0x01f9,
	0x0217,0x0237,0x0259,0x027d,0x02a3,0x02cb,0x02f5,0x0322,0x0352,0x0385,0x03ba,0x03f3,
	0x042f,0x046f,0x04b2,0x04fa,0x0546,0x0596,0x05eb,0x0645,0x06a5,0x070a,0x0775,0x07e6,
	0x085f,0x08de,0x0965,0x09f4,0x0a8c,0x0b2c,0x0bd6,0x0c8a,0x0d49,0x0e14,0x0eea,0x0fcd,
	0x10bd,0x11bc,0x12ca,0x13e8,0x1517,0x1658,0x17ac,0x1915,0x1a93,0x1c27,0x1dd4,0x1f9a,
	0x217b,0x2378,0x2594,0x27d0,0x2a2e,0x2cb0,0x2f58,0x3229,0x3524,0x384d,0x3ba6,0x3f32,
};

// sinetable for vibrato effect
static PROGMEM prog_char sintab[] =
{
	0,6,12,17,22,26,29,31,31,31,29,26,22,17,12,6,0,-7,-13,-18,-23,-27,-30,-32,-32,-32,
	-30,-27,-23,-18,-13,-7
};

PROGMEM prog_uchar arpeggios[] = {
	0,4,7,-1,-1,
	0,3,7,-1,-1,	
	0,4,7,10,-1,
	0,12,-1,-1,-1,
};

Channel channel[OSCILLATORS];

uint8_t songpos = 0;
uint8_t trackpos = 0;
uint8_t trackcounter = 0;
uint8_t tempo = 5;    // 10 = 150bpm, smaller values = faster tempo
uint8_t captureChannels = 0;

void initPlayroutine()
{
	memset(channel, 0, sizeof(channel));
}

void playNote(uint8_t voice, uint8_t note, uint8_t instrNum)
{
	Instrument* instr = &instruments[instrNum];
	uint16_t freq = pgm_read_word_near(pitches + note);
	osc[voice].frequency = freq;
	osc[voice].ctrl = 1;
	osc[voice].waveform = pgm_read_byte_near(&instr->waveform);
	osc[voice].attack = pgm_read_byte_near(&instr->attack);
	osc[voice].decay = pgm_read_byte_near(&instr->decay);
	osc[voice].sustain = pgm_read_byte_near(&instr->sustain);
	osc[voice].release = pgm_read_byte_near(&instr->release);
	osc[voice].pulseWidth = pgm_read_byte_near(&instr->pulseWidth);
	channel[voice].instrument = instrNum;
	channel[voice].note = note;
	channel[voice].frequency = freq;
	channel[voice].vibratoPhase = 0;
	channel[voice].effect = 0;
	channel[voice].arpPhase1 = 0;
	channel[voice].arpPhase2 = 0;
	channel[voice].pulseWidthPhase = 0;

	// reseed noise
	if(osc[voice].waveform == NOISE)
	{
		noise = 0xACE1;
		osc[voice].phase = 0;
	}
}

void noteOff(uint8_t voice)
{
	osc[voice].ctrl = 0;

	// in case the oscillator was in attack phase, the amplitude may have been higher than sustain amplitude
	// with low sustain and long release, this produces odd behavior so we'll clamp oscillator amplitude
	// to sustain level when note is released
	osc[voice].amp = min(osc[voice].amp, osc[voice].sustain<<8);
}

// Updates track playback. Called at 50hz.
void updatePlayroutine()
{
	trackcounter++;
	if(trackcounter < tempo)
		return;

	// process track row on all channels
	for(int i = 0; i < CHANNELS; i++) 
	{
		if((captureChannels & (1<<i)) == 0) {
			uint8_t trindex = pgm_read_byte_near(song + songpos * CHANNELS + i);
			uint8_t trackline = pgm_read_byte_near(tracks + trindex * 64 + trackpos);
			prog_uchar* trackdata = &tracklines[trackline*3];
			uint8_t note = pgm_read_byte_near(trackdata);
			uint8_t instrument = pgm_read_byte_near(trackdata+1);
			uint8_t effect = pgm_read_byte_near(trackdata+2) >> 4;

			if(note < 0x80)
			{
				if(effect != SLIDE_NOTE)
					playNote(i, note, instrument);
			}
			else if(note == 0xfe)
			{
				noteOff(i);
			}

			// set channel effect
			if(effect)
			{
				channel[i].effect = pgm_read_byte_near(trackdata+2);

				if(effect == SLIDE_NOTE)
				{
					if(note < 0x80)
						channel[i].targetFrequency = pgm_read_word_near(pitches + note);
					else
						channel[i].effect = 0;	// no note to slide to
				}
				else if(effect == DRUM)
				{
					noise = 0xACE1;
					osc[i].waveform = NOISE;
					osc[i].phase = 0;
				}
				else if(effect == VOLUME_DOWN || effect == VOLUME_UP)
				{
					osc[i].ctrl = 3;	// manual volume control
				}
			}
		}
	}

	// increment track pos
	trackpos = (trackpos+1);
	trackcounter = 0;

	// track finished?
	if(trackpos == TRACK_LENGTH)
	{
		songpos++;
		// restart song?
		uint8_t trindex = pgm_read_byte_near(song + songpos * CHANNELS);
		if(trindex == 0xff)
			songpos = 0;
		trackpos = 0;
	}
}

// called at 50hz
void updateEffects()
{
	for(int i=0; i<CHANNELS; i++)
	{
		if((captureChannels & (1<<i)) == 0) {
			Channel* chan = &channel[i];
			Instrument* instr = &instruments[chan->instrument];

			uint16_t f = chan->frequency;

			// vibrato
			uint8_t vibscaler = f>>4; 
			//f += ((sintab[chan->vibratoPhase>>3] * instr->vibratoDepth) >> 5) * vibscaler >> 3;
			int8_t sine = pgm_read_byte_near(sintab + (chan->vibratoPhase>>3));
			int8_t vibDepth = pgm_read_byte_near(&instr->vibratoDepth);
			f += ((sine * vibDepth) >> 5) * vibscaler >> 3;
			chan->vibratoPhase += pgm_read_byte_near(&instr->vibratoSpeed);

			// pulse width
			uint8_t pulseWidthSpeed = pgm_read_byte_near(&instr->pulseWidthSpeed);
			if(pulseWidthSpeed != 0)
			{
				chan->pulseWidthPhase += ((uint16_t)pulseWidthSpeed)<<4;
				uint8_t phase = chan->pulseWidthPhase>>8;
				if(phase < 128)
					osc[i].pulseWidth = phase*2;
				else
					osc[i].pulseWidth = (255 - phase)*2;
				// remap pulse width from [0,255] to [10,245]
				osc[i].pulseWidth = ((osc[i].pulseWidth * (245-10))>>8) + 10;
			}

			// effect
			uint8_t effect = chan->effect>>4;
			uint8_t param = chan->effect & 0xf;
			uint8_t transpose;
			switch(effect)
			{
			case SLIDEUP:
				chan->frequency += param*4;
				break;

			case SLIDEDOWN:
				chan->frequency = max((int16_t)chan->frequency - param*4, 0);
				break;

			case ARPEGGIO:
				transpose = pgm_read_byte_near(arpeggios + (param-1) * 5 + chan->arpPhase2);
				if(transpose == 0xff) {
					chan->arpPhase2 = 0;
					transpose = 0;
				}
				if(++chan->arpPhase1 >= 2)	// TODO: arpeggio speed
				{
					chan->arpPhase1 = 0;
					chan->arpPhase2++;
				}
				f = pgm_read_word_near(pitches + chan->note + transpose);
				break;

			case DRUM:
				if(chan->arpPhase1++ == param+1)
					osc[i].waveform = pgm_read_byte_near(&instr->waveform);
				break;

			case SLIDE_NOTE:
				if(chan->targetFrequency > chan->frequency)
					chan->frequency = min(chan->frequency + param*2, chan->targetFrequency);
				else if(chan->targetFrequency < chan->frequency)
					chan->frequency = max(chan->frequency - param*2, chan->targetFrequency);
				break;

			case VOLUME_DOWN:
				osc[i].amp = max(osc[i].amp - param*32, 0);
				break;

			case VOLUME_UP:
				osc[i].amp = min(osc[i].amp + param*8, 0x7fff);
				break;
			}

			osc[i].frequency = f;
		}
	}
}
