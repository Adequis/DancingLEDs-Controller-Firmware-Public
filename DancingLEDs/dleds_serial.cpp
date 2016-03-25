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

#include "dleds_serial.h"

DLEDsSerial::DLEDsSerial()  {
  goodImage = false;
  numImagePixels = 0;
  pixel = {0, 0, 0};
}

void DLEDsSerial::begin(void) {
  //Serial setup
  pinMode(ACTIVE, INPUT);
  Serial.setTimeout(200); // 50 was too short for reliable transfer via Processing
  Serial.begin(115200);
  delay(1000);  //DEBUG: 1 second delay to see the start up comments on the serial board
  
}

void DLEDsSerial::updateNumImagePixels(void) {
  int firstByte = Serial.read();
  int secondByte = Serial.read();
  numImagePixels = (secondByte << 8) | firstByte;
  
  //prepare for image read
  goodImage = true;
}

int DLEDsSerial::getNumImagePixels(void) {
  return numImagePixels;
}

boolean DLEDsSerial::getGoodImage(void) {
  return goodImage;
}

RGB_t DLEDsSerial::getNextPixel()
{
  count = 0;
  count = count + Serial.readBytes((char*) &pixel.r, 1);
  count = count + Serial.readBytes((char*) &pixel.g, 1);
  count = count + Serial.readBytes((char*) &pixel.b, 1);
  if (count != 3) {
    goodImage = false;
   }  
  return pixel;
}

void DLEDsSerial::answerQuery(void) {
    Serial.print(LED_WIDTH);
    Serial.write(',');
    Serial.print(LED_HEIGHT);
    Serial.write(',');
    Serial.print(LED_LAYOUT);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(VIDEO_XOFFSET);
    Serial.write(',');
    Serial.print(VIDEO_YOFFSET);
    Serial.write(',');
    Serial.print(VIDEO_WIDTH);
    Serial.write(',');
    Serial.print(VIDEO_HEIGHT);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.write(',');
    Serial.print(0);
    Serial.println();
}
