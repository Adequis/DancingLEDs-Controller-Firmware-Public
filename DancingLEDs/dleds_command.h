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

#ifndef DLEDS_COMMAND_H
#define DLEDS_COMMAND_H

#include <Arduino.h>
#include "dleds_palette.h"
#include "dleds_pattern.h"
#include "dleds_button.h"
#include "dleds_flash.h"

#define MAX_COMMAND_SIZE 776 //"Full" size of command Start + Control + Num + Palette (3*256) + control + Mode + Speed + Dwell + End

#define MAX_NAME_LEN 8

#define MAX_PIN_LEN 4

//If we update the modes in ButtonIndex_t make sure to update NUMBUTTONMODES as well.
//TODO: Use a "final entry" for the enum to count the entries? Not sure that's possible with the Button class initialization.
#define NUMBUTTONMODES 23
typedef enum ButtonIndex_t { bOff, bChristmasMarch, bChristmasFade, bValentinesMarch, bValentinesFade, bPatrickMarch, bPatrickFade, 
                             bEasterMarch, bEasterFade, bJulyMarch, bJulyFade, bHalloweenMarch, bHalloweenFade, bThanksgivingMarch, 
                             bThanksgivingFade, bSolidFixed, bRainbowMarch, bSolidFade, bRainbowFade, bCandle, bStar, bCandleRainbow, bPlasma };

class DLEDsCommand {
private:
  uint8_t command_buffer[MAX_COMMAND_SIZE];
  
  int ledsTotal;

  int bufferIndex;
  int bufferSize;
  
  char returnChar;
  
  boolean turnLightsOff;
  
  boolean versionQuery;
  boolean fileQuery;
  boolean fileNumQuery;
  
  boolean receiving;
  boolean commandFinished;
  boolean paletteValid;
  
  boolean timeSet;
  boolean onOffSet;
  boolean delaySet;
  
  boolean scheduled;
  
  boolean nameSet;
  boolean serverPinSet;
  boolean clientPinSet;
  
  int currentTime;
  int timeOn;
  int timeOff;
  
  FileType_t currentFileType;
  
  uint8_t name[MAX_NAME_LEN];
  uint8_t name_len;
  
  uint8_t serverPin[MAX_PIN_LEN];
  
  uint8_t clientPin[MAX_PIN_LEN];
 
public:
  DLEDsCommand(int ledsTotal);
  DLEDsPalette palette;
  DLEDsPalette fadePalette;
  DLEDsPattern pattern;
  DLEDsButton button;
  DLEDsFlash flash;
  
  char getReturnChar(void);
  boolean isTurnLightsOff(void);
  void setTurnLightsOff(boolean turnLightsOff);
  boolean isCommandFinished(void);
  boolean isPaletteValid(void);
  void setPaletteValid(boolean paletteValid);
  boolean isTimeSet(void);
  void setTimeSet(boolean timeSet);
  boolean isOnOffSet(void);
  void setOnOffSet(boolean onOffSet);
  boolean isScheduled(void);
  boolean isDelaySet(void);
  void setDelaySet(boolean delaySet);
  boolean isVersionQuery(void);
  void setVersionQuery(boolean versionQuery);
  boolean isFileQuery(void);
  void setFileQuery(boolean fileQuery); 
  boolean isFileNumQuery(void);
  void setFileNumQuery(boolean fileNumQuery);
  boolean isNameSet(void);
  void setNameSet(boolean nameSet);
  boolean isServerPinSet(void);
  boolean isClientPinSet(void);
  void setClientPinSet(boolean clientPinSet);
  
  int getCurrentTime(void);
  int getTimeOn(void);
  int getTimeOff(void);
  uint8_t* getBroadcastName(void);
  uint8_t getNameLen(void);
  
  uint8_t* getServerPin(void);
  uint8_t* getClientPin(void);
  
  FileType_t getCurrentFileType(void);
  void setCurrentFileType(FileType_t fileType);
  
  void readAciBuffer(uint8_t* uart_buffer, uint8_t uart_buffer_len);
  void checkPinCommand(void);
  void checkCommandType(void);
  
  void buttonCommand(ButtonPressed_t buttonPressed);
  void buttonIndexCommand(int index);
  
  void characterCommand(char controlChar);
  void paletteCommand(void);
  void delayCommand(void);
  void setTimeCommand(void);
  void setTimeOnOff(void);
  void setBroadcastName(void);
  void setServerPin(void);
  void setClientPin(void);
  void setFlashFileNum(void);
  
  void updateFadePalette(int percent, boolean marchForward);
  void updateFadePaletteRandom(int percent, boolean marchForward);
  
  int flashUpdatePalette(int startAddr);
  
};

#endif
