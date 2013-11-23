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
#include "audio.h"

Oscillator osc[OSCILLATORS];
volatile uint8_t audioBuffer[263*2];
extern volatile uint8_t* audioBufferWritePtr = audioBuffer;
extern volatile uint8_t* audioBufferReadPtr = audioBuffer + 263;

uint16_t noise = 0xACE1;
int envelopeUpdateCounter = 0;
int tickCounter = 0;  // tick rate 16 Khz

/*
// generates a 440hz test tone
void testTone() {
	pinMode(11, OUTPUT);
	while(true) {
		digitalWrite(11, HIGH);
		delayMicroseconds(1136);	// 1000000/440/2
		digitalWrite(11, LOW);
		delayMicroseconds(1136); 	// 1000000/440/2
	}
}

// generates a 440hz test tones sampled at ~15700hz
void testTone2() {
	pinMode(11, OUTPUT);
	uint16_t phase = 0;
	while(true) {
		delayMicroseconds(64);	// 15700
		phase += 1832;
		digitalWrite(11, phase < 32768 ? 0 : 1);
	}
}
*/

void initAudio()
{
	//testTone();
	//testTone2();

	memset((void*)osc, 0, sizeof(osc));
	memset((void*)audioBuffer, 0, sizeof(audioBuffer));

	pinMode(11, OUTPUT);

	// Timer/Counter2 Control Register A
	// bits 7-6: Clear OC2A on Compare Match, set OC2A at BOTTOM (non-inverting mode)
	// bits 5-4: Clear OC2B on Compare Match, set OC2B at BOTTOM (non-inverting mode)
	// bits 1-0: enable Fast PWM
	//
	// Timer/Counter2 Control Register B
	// bits 2-0: No prescaling (full clock rate)

	// fast PWM mode (does not work)
	// messes up green color
	//TCCR2A = 0b10100011;
	TCCR2A = 0b10000011;

	//TCCR2A = _BV(WGM21) | _BV(COM2A0);	// TCCR2A = 0b01000010
	TCCR2B = _BV(CS20);					// TCCR2B = 0b00000001

	// generate 440hz test tone on pin 11
	/*
	while(true) {
	 	OCR2A = 255;
		delayMicroseconds(1136);	// 1000000/440/2
	 	OCR2A = 0;
		delayMicroseconds(1136);	// 1000000/440/2
	}
	*/
}

#if 1
void mixAudio(volatile uint8_t* buf, int numSamples)
{
	__asm__ __volatile__ (
		// Registers
		// r0			temp
		// r1			temp
		// r3:r2		phase 1
		// r5:r4		phase 2
		// r7:r6		phase 3
		// r9:r8		phase 4
		// r11:r10		frequency 1
		// r13:r12		frequency 2
		// r15:r14		frequency 3
		// r17:r16		frequency 4
		// r18			volume 1
		// r19			volume 2
		// r20			volume 3
		// r21			volume 4
		// r22			waveform bits (bits: 0-1 waveform1, 2-3 waveform2, 4-5 waveform3, 6-7 waveform4)
		// r23			waveform sample
		// r25:r24		sample accumulator
		// r27:r26 (X)	audio buffer (output)
		// r29:r28 (Y)	oscillators
		// r31:r30 (Z)	mix loop counter

		// load and pack waveform bits
		"ldd	r22, Y+42+4\n\t"
		"lsl	r22\n\t"
		"lsl	r22\n\t"
		"ldd	r0, Y+28+4\n\t"
		"or		r22, r0\n\t"
		"lsl	r22\n\t"
		"lsl	r22\n\t"
		"ldd	r0, Y+14+4\n\t"
		"or		r22, r0\n\t"
		"lsl	r22\n\t"
		"lsl	r22\n\t"
		"ldd	r0, Y+4\n\t"
		"or		r22, r0\n\t"

		// load channel params
		"ldd	r2, Y+0\n\t"
		"ldd	r3, Y+1\n\t"
		"ldd	r10, Y+2\n\t"
		"ldd	r11, Y+3\n\t"
		"ldd	r18, Y+7\n\t"
		"ldd	r4, Y+14\n\t"
		"ldd	r5, Y+15\n\t"
		"ldd	r12, Y+16\n\t"
		"ldd	r13, Y+17\n\t"
		"ldd	r19, Y+21\n\t"
		"ldd	r6, Y+28\n\t"
		"ldd	r7, Y+29\n\t"
		"ldd	r14, Y+30\n\t"
		"ldd	r15, Y+31\n\t"
		"ldd	r20, Y+35\n\t"
		"ldd	r8, Y+42\n\t"
		"ldd	r9, Y+43\n\t"
		"ldd	r16, Y+44\n\t"
		"ldd	r17, Y+45\n\t"
		"ldd	r21, Y+49\n\t"

		// init loop counter
		"push	r30\n\t"
		"push	r31\n\t"
		"ldi	r30, 7\n\t"
		"ldi	r31, 1\n\t"		// r31:r30 = 263 (=256+7)

	"mixloop:\n\t"
		// sample = 0
		"clr	r24\n\t"
		"clr	r25\n\t"

		// phase = phase + frequency
		"add	r2, r10\n\t"
		"adc	r3, r11\n\t"
		"add	r4, r12\n\t"
		"adc	r5, r13\n\t"
		"add	r6, r14\n\t"
		"adc	r7, r15\n\t"
		"add	r8, r16\n\t"
		"adc	r9, r17\n\t"

	".macro dochannel ch phase volume\n\t"

		// jump to waveform
		"mov	r23, r22\n\t"
		"andi	r23, 3<<(2*\\ch)\n\t"	// r0 = waveform bits & waveform mask
		"cpi	r23, 1<<(2*\\ch)\n\t"
		"breq	wf_pulse\\ch\n\t"
		"cpi	r23, 0\n\t"
		"breq	wf_triangle\\ch\n\t"
		"cpi	r23, 2<<(2*\\ch)\n\t"
		"breq	wf_sawtooth\\ch\n\t"
		"rjmp	wf_noise\\ch\n\t"

		// pulse waveform: value = (phase < pulseWidth ? -64 : 63)
	"wf_pulse\\ch:\n\t"
		"ldi	r23, -64\n\t"			// r23 = -64
		"ldd	r0, Y+\\ch*14+5\n\t"	// r0 = pulse width
		"cp		\\phase, r0\n\t"		// compare phase and pulse width, N = 1 if phase < pulse width
		"brmi	wf_done\\ch\n\t"			// branch if N = 1
		"ldi	r23, 63\n\t"
		"rjmp	wf_done\\ch\n\t"

		// triangle waveform: value = (phase < 128 ? phase - 64 : (255 - phase) - 63)
	"wf_triangle\\ch:\n\t"
		"ldi	r23, 128\n\t"
		"cp		\\phase, r23\n\t"
		"brpl	triangle1_\\ch\n\t"
		// phase < 128
		"ldi	r23, -64\n\t"
		"add	r23, \\phase\n\t"
		"rjmp	wf_done\\ch\n\t"
	"triangle1_\\ch:\n\t"
		// phase >= 128
		"ldi	r23, 255-63\n\t"
		"sub	r23, \\phase\n\t"
		"rjmp	wf_done\\ch\n\t"

		// sawtooth waveform: value = (phase>>1) - 64 = (phase - 128)>>1
	"wf_sawtooth\\ch:\n\t"
		"ldi	r23, -128\n\t"
		"sub	r23, \\phase\n\t"
		"asr	r23\n\t"
		"rjmp	wf_done\\ch\n\t"

		// noise waveform: 
		// Galois LFSR, see http://en.wikipedia.org/wiki/Linear_feedback_shift_register
		//     noise = (noise >> 1) ^ (-(noise & 1) & 0xB400u);
		//
		// <=> if(noise & 1)
		//         noise = (noise>>1)^0xB400u;
		//     else
		//         noise = noise>>1;
	"wf_noise\\ch:\n\t"
		"lds	r1, noise\n\t"
		"lds	r0, noise+1\n\t"	// r1:r0 = noise
		"mov	r23, r0\n\t"
		"lsr	r1\n\t"
		"ror	r0\n\t"				// r1:r0 = noise>>1
		"andi	r23, 1\n\t"			
		"breq	noise_\\ch\n\t"		// if noise & 1 == 0 -> jump to noiseX
		"ldi	r23, 0xb4\n\t"
		"eor	r1, r23\n\t"
	"noise_\\ch:\n\t"
		"sts	noise, r1\n\t"
		"sts	noise+1, r0\n\t"
		// if(phase > 64):
		//	  osc[i].noise = (int)(noise>>9) - 64;
		//	  osc[i].phase -= 16384;
		"ldi	r23,64\n\t"
		"cp		\\phase, r23\n\t"
		"brpl	noise2_\\ch\n\t"
		"lsr	r1\n\t"
		"sub	r1, r23\n\t"		// r1 = (noise>>9) - 64
		"std	Y+\\ch*14+8, r1\n\t"	// update osc->noise
		"sub	\\phase, r23\n\t"	// phase = phase - 16384
	"noise2_\\ch:\n\t"
		"ldd	r23, Y+\\ch*14+8\n\t"		

		// multiply waveform sample (r23) with channel volume and accumulate to r25:r24
	"wf_done\\ch:\n\t"
		"mulsu	r23, \\volume\n\t"		// r1:r0 = waveform * volume (mulsu: Rd signed, Rr unsigned)
		"add	r24, r0\n\t"	
		"adc	r25, r1\n\t"		// sample = sample + waveform * volume	
	".endm\n\t"

		"dochannel 0 r3 r18\n\t"
		"dochannel 1 r5 r19\n\t"
		"dochannel 2 r7 r20\n\t"
		"dochannel 3 r9 r21\n\t"

		// output sample = (sample>>8) + 128
		"ldi	r24, 128\n\t"
		"add	r25, r24\n\t"
		"st		X+, r25\n\t"

		// branch to mixloop
		"sbiw	r30, 1\n\t"
		//"brne	mixloop\n\t"		// Z=0 -> goto mixloop
		"breq	mixdone\n\t"		// Z=0 -> goto mixloop
		"rjmp	mixloop\n\t"
	"mixdone:\n\t"
		// store channel phase
		"std	Y+0, r2\n\t"
		"std	Y+1, r3\n\t"
		"std	Y+14, r4\n\t"
		"std	Y+15, r5\n\t"
		"std	Y+28, r6\n\t"
		"std	Y+29, r7\n\t"
		"std	Y+42, r8\n\t"
		"std	Y+43, r9\n\t"

		"pop	r31\n\t"
		"pop	r30\n\t"

		// without this line game freezes when 4th audio channel is used!
		// gcc does not preserve contents of r1?
		"clr	r1\n\t"
		:
		:
		"x" (buf),
		"y" (osc)
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16",
		  "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25"
	);

}
#endif

#if 0
void mixAudio(volatile uint8_t* buf, int numSamples)
{
	for(int s = 0; s < numSamples; s++) {
		int16_t output = 0;

		for(uint8_t i=0; i<OSCILLATORS; i++)
		{
			// update oscillator phase
			osc[i].phase += osc[i].frequency;
			uint8_t phase = osc[i].phase >> 8; // [0,255]

			int8_t value = 0;  // [-64,63]

			switch(osc[i].waveform)
			{
			case TRIANGLE: 
				if(phase < 128)
					value = phase - 64;
				else
					value = (255 - phase) - 63;
				break;

			case PULSE:
				value = (phase < osc[i].pulseWidth ? -64 : 63);
				break;

			case SAWTOOTH:
				value = (phase>>1) - 64;
				break;

			case NOISE:
				if(phase > 64)
				{
					// Galois LFSR, see http://en.wikipedia.org/wiki/Linear_feedback_shift_register
					noise = (noise >> 1) ^ (-(noise & 1) & 0xB400u);

					osc[i].noise = (int)(noise >> 9) - 64;
					osc[i].phase -= 16384;
				}
				value = osc[i].noise;
				break;
			}
			   
			int16_t amp = osc[i].amp>>8;  // [0,127]
			output += value * amp;  // [-8192,8191]
		}

		// update envelopes every 16th cycle
		// if(envelopeUpdateCounter == 0)
		// {
		// 	updateEnvelopes();
		//  	envelopeUpdateCounter = 16;
		// }
		// envelopeUpdateCounter--;
	 
#if OSCILLATORS == 4
		uint8_t sample = (output>>8) + 128;
#elif OSCILLATORS == 2
		uint8_t sample = (output>>7) + 128;
#elif OSCILLATORS == 1
		uint8_t sample = (output>>6) + 128;
#endif
		*buf++ = sample;
	}
}
#endif

void updateEnvelopes()
{
	for(int i=0; i<OSCILLATORS; i++)
	{
		if(osc[i].ctrl == 1)
		{
			// attack: fade to peak amplitude, start decay when peak amplitude reached
			uint16_t amp = osc[i].amp;
			//amp += ((uint16_t)osc[i].attack)<<2;
			amp += ((uint16_t)osc[i].attack)<<8;
			if(amp >= 0x7fff)
			{
				amp = 0x7fff;
				osc[i].ctrl = 2;
			}
			osc[i].amp = amp;
		}
		else if(osc[i].ctrl == 2)
		{
			// decay: fade to sustain amplitude
			int16_t amp = osc[i].amp;
			amp = max(amp - ((int16_t)osc[i].decay<<8), osc[i].sustain<<8);
			osc[i].amp = amp;
		}
		else if(osc[i].ctrl == 0)
		{
			// release: fade to zero amplitude
			int16_t amp = osc[i].amp;
			amp = max(amp - (osc[i].release<<4), 0);
			osc[i].amp = amp;
		}
	}
}

void resetAudioChannels()
{
	for(int i = 0; i < OSCILLATORS; i++)
	{
		osc[i].phase = 0;
		osc[i].frequency = 0;
		osc[i].ctrl = 0;
	}
}
