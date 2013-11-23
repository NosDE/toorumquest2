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

#ifndef GAMEPAD_H
#define GAMEPAD_H

#define BUTTON_A		(1<<7)
#define BUTTON_B		(1<<6)
#define BUTTON_SELECT	(1<<5)
#define BUTTON_START	(1<<4)
#define BUTTON_UP		(1<<3)
#define BUTTON_DOWN		(1<<2)
#define BUTTON_LEFT		(1<<1)
#define BUTTON_RIGHT	(1<<0)

void initController();
void updateController();

extern uint8_t controllerState;
extern uint8_t prevControllerState;

#endif
