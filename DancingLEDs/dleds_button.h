/* Copyright (c) 2015, Dancing LEDs, L.L.C.
 * Copyright (c) 2014, Nordic Semiconductor ASA
 * Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef DLEDS_BUTTON_H
#define DLEDS_BUTTON_H

#include <Arduino.h>

#define UPPIN 28
#define DOWNPIN 30
#define LEFTPIN 29
#define RIGHTPIN 27

const int buttonDebounceCount = 1;
const byte leftButtonMask = 0x08;
const byte upButtonMask = 0x04;
const byte downButtonMask = 0x02;
const byte rightButtonMask = 01;

typedef enum ButtonPressed_t { bNone, bUp, bDown, bLeft, bRight };

class DLEDsButton {
private:
  int numButtonModes;
  int buttonIndex;
  int solidColorIndex;

  boolean upButtonPressed;
  boolean downButtonPressed;
  boolean leftButtonPressed;
  boolean rightButtonPressed;
  
  ButtonPressed_t buttonPressed;
  
  boolean buttonArmed;
  boolean buttonChanged;

  byte buttonByte;
  byte lastButtonByte;
  byte origButtonByte;

  int buttonCount;
  
  void updateButtonPressed(void);
  void updateButtonIndex(void);
  
public:
  DLEDsButton(int numButtonModes);
  
  void begin(void);
  void updateButtonStatus(void);
  
  boolean isButtonChanged(void);
  int getButtonIndex(void);
  ButtonPressed_t getButtonPressed(void);
  
};

#endif
