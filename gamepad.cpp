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
#include "gamepad.h"

const int latch = 12; // set the latch pin
const int clock = 10; // set the clock pin
const int datin = 13;// set the data in pin

uint8_t controllerState = 0;
uint8_t prevControllerState = 0;

void initController() {
	pinMode(latch, OUTPUT);
	pinMode(clock, OUTPUT);
	pinMode(datin, INPUT);

	digitalWrite(latch, HIGH);
	digitalWrite(clock, HIGH);
}

void updateController() {
	prevControllerState = controllerState;
	
	controllerState = 0;
	digitalWrite(latch, LOW);
	digitalWrite(clock, LOW);

	digitalWrite(latch, HIGH);
	delayMicroseconds(2);
	digitalWrite(latch, LOW);

	controllerState = digitalRead(datin);

	for (int i = 1; i <= 7; i ++) {
		digitalWrite(clock, HIGH);
		delayMicroseconds(2);
		controllerState = controllerState << 1;
		controllerState = controllerState + digitalRead(datin);
		delayMicroseconds(4);
		digitalWrite(clock,LOW);
	}

	controllerState ^= 0xff;
}
