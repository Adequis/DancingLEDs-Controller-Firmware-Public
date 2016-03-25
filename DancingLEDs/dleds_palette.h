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

#ifndef DLEDS_PALETTE_H
#define DLEDS_PALETTE_H

#include <Arduino.h>
#include "defaultPalette.h"

#define MAX_PALETTE_SIZE 256

typedef enum SysPalette_t { dNone, dDefault, dChristmas, dValentines, dPatrick, dEaster, dJuly, dHalloween, dThanksgiving, dCandle, dStar, dRainbow, dSolid };

typedef struct RGB_t {
  byte r;
  byte g;
  byte b;
};

typedef struct Palette_t {
  RGB_t* colors;
  uint8_t nColors;
};

class DLEDsPalette {
private:
  RGB_t color[MAX_PALETTE_SIZE];
  uint8_t numColors;
  SysPalette_t systemPalette;
  boolean paletteSet;
  
  RGB_t getColorFromInt(int colorInt);

public:
  DLEDsPalette();
  
  void setNumColors(uint8_t numColors);
  uint8_t getNumColors(void);
  void setColor(RGB_t rgb, uint8_t index);
  RGB_t getColor(uint8_t index);
  RGB_t* getColors(void);
  Palette_t getPalette(void);
  Palette_t getSolidColorPalette(uint8_t index);
  boolean isPaletteSet(void);
  void setPaletteSet(boolean paletteSet);
  
  RGB_t getColorFromColor(RGB_t firstColor, RGB_t nextColor, int pct);
  void setSystemPalette(SysPalette_t systemPalette);
  SysPalette_t getSystemPalette(void);
  
};

#endif
