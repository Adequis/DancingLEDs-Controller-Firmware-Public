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

#include "dleds_palette.h"

DLEDsPalette::DLEDsPalette(){
  
  memset(&color, 0, MAX_PALETTE_SIZE * sizeof(RGB_t));
  numColors = 0;
  systemPalette = dNone;
  paletteSet = false;
  
}

void DLEDsPalette::setNumColors(uint8_t numColors) {
  this->numColors = numColors;
}

uint8_t DLEDsPalette::getNumColors(void) {
  return numColors;
}

void DLEDsPalette::setColor(RGB_t rgb, uint8_t index) {
  this->color[index] = rgb;
}

RGB_t DLEDsPalette::getColor(uint8_t index) {
  return color[index];
}

RGB_t* DLEDsPalette::getColors(void) {
  return color;
}

Palette_t DLEDsPalette::getPalette(void) {
  return {color, numColors};
}

Palette_t DLEDsPalette::getSolidColorPalette(uint8_t index) {
  return {&color[index], 0};
}

boolean DLEDsPalette::isPaletteSet(void) {
  return paletteSet;
}

void DLEDsPalette::setPaletteSet(boolean paletteSet) {
  this->paletteSet = paletteSet;
}

SysPalette_t DLEDsPalette::getSystemPalette(void) {
  return systemPalette;
}

void DLEDsPalette::setSystemPalette(SysPalette_t systemPalette) {
  this->systemPalette = systemPalette;
  
  switch(systemPalette) {
  case dDefault:
    numColors = sizeof(defaultPalette)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(defaultPalette[i]);
    }
    break;
  case dChristmas:
    numColors = sizeof(ChristmasColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(ChristmasColors[i]);
    }
    break;
  case dValentines:
    numColors = sizeof(ValentinesColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(ValentinesColors[i]);
    }
    break;
  case dPatrick:
    numColors = sizeof(StPatricksColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(StPatricksColors[i]);
    }
    break;
  case dEaster:
    numColors = sizeof(EasterColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(EasterColors[i]);
    }
    break;
  case dJuly:
    numColors = sizeof(July4thColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(July4thColors[i]);
    }
    break;
  case dHalloween:
    numColors = sizeof(HalloweenColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(HalloweenColors[i]);
    }
    break;
  case dThanksgiving:
    numColors = sizeof(ThanksgivingColors)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(ThanksgivingColors[i]);
    }
    break;
  case dCandle:
    numColors = sizeof(candlePalette)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(candlePalette[i]);
    }
    break;
  case dStar:
    numColors = sizeof(starPalette)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(starPalette[i]);
    }
    break;
  case dRainbow:
    numColors = sizeof(rainbowPalette)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(rainbowPalette[i]);
    }
    break;
  case dSolid:
    numColors = sizeof(solidColorArray)/sizeof(int) - 1;
    for (int i=0; i <= numColors; i++) {
      color[i] = getColorFromInt(solidColorArray[i]);
    }
    break;
    default:
      paletteSet = false;
      return;
  }
  paletteSet = true;
}

// Calculate one RGB color from another based on percent
RGB_t DLEDsPalette::getColorFromColor (RGB_t firstColor, RGB_t nextColor, int pct) {
      
  uint8_t firstRed;
  uint8_t firstGreen;
  uint8_t firstBlue;
  
  uint8_t nextRed;
  uint8_t nextGreen;
  uint8_t nextBlue;
    
  uint8_t toRed;
  uint8_t toGreen;
  uint8_t toBlue;
    
  float dec = pct/100.0;
    
  //Extract the color Bytes from the palette color
  firstRed = (uint8_t) firstColor.r;
  firstGreen = (uint8_t) firstColor.g;
  firstBlue = (uint8_t) firstColor.b;
  nextRed = (uint8_t) nextColor.r;
  nextGreen = (uint8_t) nextColor.g;
  nextBlue = (uint8_t) nextColor.b;
    
  //Fade individual colors into each other
  toRed = (nextRed - firstRed)*dec + firstRed;
  toGreen = (nextGreen - firstGreen)*dec + firstGreen;
  toBlue = (nextBlue - firstBlue)*dec + firstBlue;
  
  return {toRed, toGreen, toBlue};
   
}

RGB_t DLEDsPalette::getColorFromInt(int colorInt) {
  byte r = (colorInt >> 16) & 0xFF;
  byte g = (colorInt >> 8) & 0xFF;
  byte b = colorInt & 0xFF;
  
  return {r, g, b};
}
