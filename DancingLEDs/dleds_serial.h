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

#ifndef DLEDS_SERIAL_H
#define DLEDS_SERIAL_H

#include <Arduino.h>
#include "dleds_palette.h"
#include "dleds_flash.h"

#define ACTIVE 31

#define LED_WIDTH      50   // number of LEDs horizontally
#define LED_HEIGHT     16   // number of LEDs vertically (must be multiple of 8)
#define LED_LAYOUT     0    // 0 = even rows left->right, 1 = even rows right->left
#define VIDEO_XOFFSET  0
#define VIDEO_YOFFSET  50       // display entire image
#define VIDEO_WIDTH    100
#define VIDEO_HEIGHT   100

class DLEDsSerial {
private:
  boolean goodImage;
  
  int numImagePixels;
  int count;
  
  RGB_t pixel;

public:
  DLEDsSerial();

  void begin(void);
  
  void updateNumImagePixels(void);
  RGB_t getNextPixel(void);
  int getNumImagePixels(void);
  boolean getGoodImage(void);
  void answerQuery(void);

};
#endif
