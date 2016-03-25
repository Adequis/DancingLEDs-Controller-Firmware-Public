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

/*
* Plasma Effect:
* PlazINT  -  Fast Plasma Generator using Integer Math Only
* Edmund "Skorn" Horn
* March 4,2013
* Version 1.0 adapted for OctoWS2811Lib (tested, working...)
*/

#ifndef DLEDS_PATTERN_H
#define DLEDS_PATTERN_H

#include <Arduino.h>

#include "dleds_palette.h"

//LED Definitions
#define MAXLEDSPERCHANNEL 500
#define MAXCHANNELS 8
#define MAXLEDS 4000

#define MIN_MARCH_DELAY 12
#define DEFAULT_MARCH_DELAY 24
#define MAX_MARCH_DELAY 44
#define MARCH_DELAY_INCREMENT 4

#define MIN_FADE_DELAY 8
#define DEFAULT_FADE_DELAY 20
#define MAX_FADE_DELAY 40
#define FADE_DELAY_INCREMENT 4

typedef enum LightMode_t { lmOff, lmFixed, lmSolidFixed, lmMarch, lmFade, lmFadeMarch, lmRandomMarch, lmRandomFade, lmCandle, lmStar, lmPlasma, lmFile, lmSerial, lmBle };

//Byte val 2PI Cosine Wave, offset by 1 PI 
//supports fast trig calcs and smooth LED fading/pulsing.
const uint8_t cos_wave[256] =  
{0,0,0,0,1,1,1,2,2,3,4,5,6,6,8,9,10,11,12,14,15,17,18,20,22,23,25,27,29,31,33,35,38,40,42,
45,47,49,52,54,57,60,62,65,68,71,73,76,79,82,85,88,91,94,97,100,103,106,109,113,116,119,
122,125,128,131,135,138,141,144,147,150,153,156,159,162,165,168,171,174,177,180,183,186,
189,191,194,197,199,202,204,207,209,212,214,216,218,221,223,225,227,229,231,232,234,236,
238,239,241,242,243,245,246,247,248,249,250,251,252,252,253,253,254,254,255,255,255,255,
255,255,255,255,254,254,253,253,252,252,251,250,249,248,247,246,245,243,242,241,239,238,
236,234,232,231,229,227,225,223,221,218,216,214,212,209,207,204,202,199,197,194,191,189,
186,183,180,177,174,171,168,165,162,159,156,153,150,147,144,141,138,135,131,128,125,122,
119,116,113,109,106,103,100,97,94,91,88,85,82,79,76,73,71,68,65,62,60,57,54,52,49,47,45,
42,40,38,35,33,31,29,27,25,23,22,20,18,17,15,14,12,11,10,9,8,6,6,5,4,3,2,2,1,1,1,0,0,0,0
};

//Gamma Correction Curve
const uint8_t exp_gamma[256] =
{0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,3,3,3,3,3,
4,4,4,4,4,5,5,5,5,5,6,6,6,7,7,7,7,8,8,8,9,9,9,10,10,10,11,11,12,12,12,13,13,14,14,14,15,15,
16,16,17,17,18,18,19,19,20,20,21,21,22,23,23,24,24,25,26,26,27,28,28,29,30,30,31,32,32,33,
34,35,35,36,37,38,39,39,40,41,42,43,44,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,
61,62,63,64,65,66,67,68,70,71,72,73,74,75,77,78,79,80,82,83,84,85,87,89,91,92,93,95,96,98,
99,100,101,102,105,106,108,109,111,112,114,115,117,118,120,121,123,125,126,128,130,131,133,
135,136,138,140,142,143,145,147,149,151,152,154,156,158,160,162,164,165,167,169,171,173,175,
177,179,181,183,185,187,190,192,194,196,198,200,202,204,207,209,211,213,216,218,220,222,225,
227,229,232,234,236,239,241,244,246,249,251,253,254,255
};

class DLEDsPattern {
private:
  int ledsTotal;
  
  LightMode_t lightMode;
  
  uint8_t numColors;
  
  boolean patternSet;
  
  byte intensity[MAXLEDS]; // Array for current 8-bit intensity value
  byte statusByte[MAXLEDS]; // Array for current status byte: 3 bit counter, 3 bit increment/decrement value, 2 bit state
  byte targetIntensity[MAXLEDS]; // Array for target intensity
  
    //Set intensity parameters
  uint8_t highDwellMin; // Minimum dwell time at high brightness 0-7 corresponds to 1-8 cycles
  uint8_t highDwellMax; // Maximum dwell time at high brightness 1-8 (random max is exclusive)
  uint8_t downStepMin;  // Minimum down step size 0-7 corresponds to step size of 1-8
  uint8_t downStepMax;  // Maximum down step size 1-8 (random max is exclusive)
  uint8_t lowBrtMin;    // Minimum low brightness 0-255
  uint8_t lowBrtMax;  // Maximum low brightness 0-255, LOWBRTMAX > LOWBRTMIN (random max is exclusive)
  uint8_t lowDwellMin;  // Minimum dwell time at low brightness 0-7 corresponds to 1-8 cycles
  uint8_t lowDwellMax;  // Maximum dwell time at low brightness 1-8 (random max is exclusive)
  uint8_t upStepMin;    // Minimum up step size 0-7 corresponds to step size of 1-8
  uint8_t upStepMax;    // Maximum up step size 1-8 (random max is exclusive)
  uint8_t highBrtMin;   // Minimum high brightness 0-255 (should be > LOWBRTMAX)
  uint8_t highBrtMax;   // Maximum high brightness 0-255, HIBRTMAX > HIBRTMIN (random max is exclusive)
  
  unsigned long plasmaCount;  // arbitrary seed to calculate the three time displacement variables t,t2,t3
  uint16_t t;
  uint16_t t2;
  uint16_t t3;
  
  uint8_t fadeDelay;
  uint8_t marchDelay;
  
  int patternDelay;
  
  int marchIndex;
  
  inline uint8_t fastCosineCalc( uint16_t preWrapVal );

public:
  DLEDsPattern(int ledsTotal);
  
  void begin(void);
  void setLightMode(LightMode_t lightMode);
  LightMode_t getLightMode(void);
  void setNumColors(uint8_t numColors);
  uint8_t getNumColors(void);
  boolean isPatternSet(void);
  void setPatternSet(boolean patternSet);
  void setFadeDelay(uint8_t fadeDelay);
  uint8_t getFadeDelay(void);
  void setMarchDelay(uint8_t marchDelay);
  uint8_t getMarchDelay(void);
  void setPatternDelay(int patternDelay);
  int getPatternDelay(void);
  int getMarchIndex(void);
  void setMarchIndex(int marchIndex);
  void updateMarchIndex(boolean marchForward);
  void updateTransitionSpeed(int mult);
  byte getIntensity(int i);
  void updateIntensity(int i);
  void setCandleParameters(LightMode_t lightMode);
  void setPlasmaParameters(void);
  RGB_t getPlasmaPixel(int x, int y);
  
};

#endif
