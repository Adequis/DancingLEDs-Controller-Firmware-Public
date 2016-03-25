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

#include "dleds_pattern.h"

DLEDsPattern::DLEDsPattern(int ledsTotal) {
  this->ledsTotal = ledsTotal;
  lightMode = lmOff;
  numColors = 0;
  patternSet = false;
  
  fadeDelay = DEFAULT_FADE_DELAY;
  marchDelay = DEFAULT_MARCH_DELAY;
  patternDelay = 0;
  
  marchIndex = 0;

}

void DLEDsPattern::begin(void) {

  for (int i=0; i < ledsTotal; i++) {
    statusByte[i] = 0x38;
    intensity[i] = 0;
    targetIntensity[i] = 255; // Start all lights rising towards max
  }
  
  //Set intensity parameters
  highDwellMin = 0; // Minimum dwell time at high brightness 0-7 corresponds to 1-8 cycles
  highDwellMax = 4; // Maximum dwell time at high brightness 1-8 (random max is exclusive)
  downStepMin = 5;  // Minimum down step size 0-7 corresponds to step size of 1-8
  downStepMax = 8;  // Maximum down step size 1-8 (random max is exclusive)
  lowBrtMin = 0;    // Minimum low brightness 0-255
  lowBrtMax = 99;  // Maximum low brightness 0-255, LOWBRTMAX > LOWBRTMIN (random max is exclusive)
  lowDwellMin = 0;  // Minimum dwell time at low brightness 0-7 corresponds to 1-8 cycles
  lowDwellMax = 2;  // Maximum dwell time at low brightness 1-8 (random max is exclusive)
  upStepMin = 3;    // Minimum up step size 0-7 corresponds to step size of 1-8
  upStepMax = 7;    // Maximum up step size 1-8 (random max is exclusive)
  highBrtMin = 160;   // Minimum high brightness 0-255 (should be > LOWBRTMAX)
  highBrtMax = 255;   // Maximum high brightness 0-255, HIBRTMAX > HIBRTMIN (random max is exclusive)
  
  plasmaCount = 25500;  // arbitrary seed to calculate the three time displacement variables t,t2,t3
  t = fastCosineCalc((42 * plasmaCount)/100);  //time displacement - fiddle with these til it looks good...
  t2 = fastCosineCalc((35 * plasmaCount)/100); 
  t3 = fastCosineCalc((38 * plasmaCount)/100);

}

void DLEDsPattern::setLightMode(LightMode_t lightMode) {
  this->lightMode = lightMode;
}

LightMode_t DLEDsPattern::getLightMode(void) {
  return lightMode;
}

void DLEDsPattern::setNumColors(uint8_t numColors) {
  this->numColors = numColors;
}

uint8_t DLEDsPattern::getNumColors(void) {
  return numColors;
}

boolean DLEDsPattern::isPatternSet(void) {
  return patternSet;
}

void DLEDsPattern::setPatternSet(boolean patternSet) {
  this->patternSet = patternSet;
}

void DLEDsPattern::setFadeDelay(uint8_t fadeDelay) {
  this->fadeDelay = fadeDelay;
}
uint8_t DLEDsPattern::getFadeDelay(void) {
  return fadeDelay;
}

void DLEDsPattern::setMarchDelay(uint8_t marchDelay) {
  this->marchDelay = marchDelay;
}

uint8_t DLEDsPattern::getMarchDelay(void) {
  return marchDelay;
}

void DLEDsPattern::setPatternDelay(int patternDelay) {
  this->patternDelay = patternDelay;
}

int DLEDsPattern::getPatternDelay(void) {
  return patternDelay;
}

int DLEDsPattern::getMarchIndex(void) {
  return marchIndex;
}

void DLEDsPattern::setMarchIndex(int marchIndex) {
  this->marchIndex = marchIndex;
}

void DLEDsPattern::updateMarchIndex(boolean marchForward) {
  
  // marchForward == false means the lights march toward the controller
  // marchForward == true means the lights march away from the controller
  
  if (marchForward == false) {  
     marchIndex++;
     if (marchIndex > numColors) {
        marchIndex = 0; 
     }
  }
  else {
     if (marchIndex == 0) {
        marchIndex = numColors;
     }
     else {
        marchIndex--;
     }  
  }
}

void DLEDsPattern::updateTransitionSpeed(int mult) {
     marchDelay = marchDelay + mult * MARCH_DELAY_INCREMENT;
     fadeDelay = fadeDelay + mult * FADE_DELAY_INCREMENT;
 
     if (marchDelay > MAX_MARCH_DELAY) {
       marchDelay = MIN_MARCH_DELAY; // wrap around
     }
     else if (marchDelay < MIN_MARCH_DELAY) {
       marchDelay = MAX_MARCH_DELAY; // wrap around
     }
     if (fadeDelay > MAX_FADE_DELAY) {
       fadeDelay = MIN_FADE_DELAY; // wrap around
     }
     else if (fadeDelay < MIN_FADE_DELAY) {
       fadeDelay = MAX_FADE_DELAY; // wrap around
     }
}

byte DLEDsPattern::getIntensity(int i) {
  return intensity[i];
}

// For candleGlow effect
// This is the state machine for updating the intensity for every pixel on the string
// Only update this code if you truly understand how it works.  A wide range of effects
// can be achieved by changing the parameters and palette defined above.
void DLEDsPattern::updateIntensity (int i)
{
  int newtest;
  byte curstate = (statusByte[i] & 0xC0) >> 6;
  byte incdec = (statusByte[i] & 0x30) >> 3;
  byte dcnt = (statusByte[i] & 0x7);
  
  switch (curstate)
  {
     case 0: // Rising intensity
     {
        newtest = intensity[i] + incdec + 1;
        if (newtest >= targetIntensity[i]) { // maximum intensity reached
           intensity[i] = targetIntensity[i];
           // Update state, pick high dwell time and reset count
           curstate = 1;
           incdec = random (highDwellMin,highDwellMax) & 0x7; 
           dcnt = 0;
           statusByte[i] = (curstate << 6) | (incdec << 3) | dcnt;
        }
        else {
           intensity[i] = byte(newtest);
        }
        break;
     }
     case 1: // Dwelling at max intensity
     {
        dcnt++;
        if (dcnt > incdec) {
        // Dwell at max over.  Update state, pick ramp down target and step
           curstate = 2;
           incdec = random (lowDwellMin, lowDwellMax) & 0x7;
           dcnt = 0;
           targetIntensity[i] = random (lowBrtMin, lowBrtMax + 1);
        }
        statusByte[i] = (curstate << 6) | (incdec << 3) | dcnt;
        break;
     }
     case 2: // Falling intensity
     {
        newtest = intensity[i] - incdec - 1;
        if (newtest <= targetIntensity[i]) { // minimum intensity reached
           intensity[i] = targetIntensity[i];
           // Update state, pick low dwell time and reset count
           curstate = 3;
           incdec = random (lowDwellMin, lowDwellMax) & 0x7;
           dcnt = 0;
           statusByte[i] = (curstate << 6) | (incdec << 3) | dcnt;
        }
        else {
           intensity[i] = byte(newtest);
        }
        break;       
     }
     case 3: // Dwelling at min intensity
     {
        dcnt++;
        if (dcnt > incdec) {
        // Dwell at min over.  Update state, pick ramp up target and step
           curstate = 0;
           incdec = random (upStepMin, upStepMax) & 0x7;
           dcnt = 0;
           targetIntensity[i] = random (highBrtMin, highBrtMax + 1);
        }
        statusByte[i] = (curstate << 6) | (incdec << 3) | dcnt;
        break;       
     }
     default:
        intensity[i] = i*4;
  }
  
}

void DLEDsPattern::setCandleParameters(LightMode_t lightMode) {
  for (int i=0; i < ledsTotal; i++) {
    statusByte[i] = 0x38;
    intensity[i] = 0;
    targetIntensity[i] = 255; // Start all lights rising towards max
  }
  switch(lightMode){
  case lmCandle :
    //Set intensity parameters
    highDwellMin = 0; // Minimum dwell time at high brightness 0-7 corresponds to 1-8 cycles
    highDwellMax = 4; // Maximum dwell time at high brightness 1-8 (random max is exclusive)
    downStepMin = 5;  // Minimum down step size 0-7 corresponds to step size of 1-8
    downStepMax = 8;  // Maximum down step size 1-8 (random max is exclusive)
    lowBrtMin = 0;    // Minimum low brightness 0-255
    lowBrtMax = 99;  // Maximum low brightness 0-255, LOWBRTMAX > LOWBRTMIN (random max is exclusive)
    lowDwellMin = 0;  // Minimum dwell time at low brightness 0-7 corresponds to 1-8 cycles
    lowDwellMax = 2;  // Maximum dwell time at low brightness 1-8 (random max is exclusive)
    upStepMin = 3;    // Minimum up step size 0-7 corresponds to step size of 1-8
    upStepMax = 7;    // Maximum up step size 1-8 (random max is exclusive)
    highBrtMin = 160;   // Minimum high brightness 0-255 (should be > LOWBRTMAX)
    highBrtMax = 255;   // Maximum high brightness 0-255, HIBRTMAX > HIBRTMIN (random max is exclusive)
    break;
  case lmStar :
    //Set intensity parameters
    highDwellMin = 0; // Minimum dwell time at high brightness 0-7 corresponds to 1-8 cycles
    highDwellMax = 2; // Maximum dwell time at high brightness 1-8 (random max is exclusive)
    downStepMin = 1;  // Minimum down step size 0-7 corresponds to step size of 1-8
    downStepMax = 3;  // Maximum down step size 1-8 (random max is exclusive)
    lowBrtMin = 0;    // Minimum low brightness 0-255
    lowBrtMax = 31;  // Maximum low brightness 0-255, LOWBRTMAX > LOWBRTMIN (random max is exclusive)
    lowDwellMin = 0;  // Minimum dwell time at low brightness 0-7 corresponds to 1-8 cycles
    lowDwellMax = 8;  // Maximum dwell time at low brightness 1-8 (random max is exclusive)
    upStepMin = 1;    // Minimum up step size 0-7 corresponds to step size of 1-8
    upStepMax = 3;    // Maximum up step size 1-8 (random max is exclusive)
    highBrtMin = 128;   // Minimum high brightness 0-255 (should be > LOWBRTMAX)
    highBrtMax = 255;   // Maximum high brightness 0-255, HIBRTMAX > HIBRTMIN (random max is exclusive)
    break;
  }
}

void DLEDsPattern::setPlasmaParameters(void) {
  plasmaCount++;
  t = fastCosineCalc((42 * plasmaCount)/100);  //time displacement - fiddle with these til it looks good...
  t2 = fastCosineCalc((35 * plasmaCount)/100); 
  t3 = fastCosineCalc((38 * plasmaCount)/100);
}

RGB_t DLEDsPattern::getPlasmaPixel(int x, int y) {
  RGB_t color;
  //Calculate 3 seperate plasma waves, one for each color channel
  color.r = (byte) fastCosineCalc(((x << 3) + (t >> 1) + fastCosineCalc((t2 + (y << 3)))));
  color.g = (byte) fastCosineCalc(((y << 3) + t + fastCosineCalc(((t3 >> 2) + (x << 3)))));
  color.b = (byte) fastCosineCalc(((y << 3) + t2 + fastCosineCalc((t + x + (color.g >> 2)))));
  
  //Gamma correction now done in main code
  //uncomment the following to enable gamma correction
//  color.r = exp_gamma[color.r];  
//  color.g= exp_gamma[color.g];
//  color.b= exp_gamma[color.b];
  
  return color;
}

//Private: For Plasma effect
inline uint8_t DLEDsPattern::fastCosineCalc( uint16_t preWrapVal )
{
  uint8_t wrapVal = (preWrapVal % 255);
  if (wrapVal<0) wrapVal=255+wrapVal;
  return cos_wave[wrapVal]; 
}

