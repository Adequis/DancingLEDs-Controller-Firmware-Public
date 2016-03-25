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

#include "dleds_command.h"

DLEDsCommand::DLEDsCommand(int ledsTotal) : palette(), fadePalette(), pattern(ledsTotal), button(NUMBUTTONMODES), flash() {
  this->ledsTotal = ledsTotal;
  
  bufferIndex = 0;
  bufferSize = 0;
  
  returnChar = '\0';
  
  turnLightsOff = false;
  
  versionQuery = false;
  fileQuery = false;
  fileNumQuery = false;
  
  receiving = false;
  commandFinished = false;
  paletteValid = false;
  
  timeSet = false;
  onOffSet = false;
  delaySet = false;
  
  scheduled = false;
  
  name_len = 0;
  nameSet = false;
 
  serverPinSet = false;
  
  clientPinSet = false;
}

char DLEDsCommand::getReturnChar(void) {
  return returnChar;
}

boolean DLEDsCommand::isTurnLightsOff(void) {
  return turnLightsOff;
}

void DLEDsCommand::setTurnLightsOff(boolean turnLightsOff) {
  this->turnLightsOff = turnLightsOff;
}

boolean DLEDsCommand::isCommandFinished(void) {
  return commandFinished;
}

boolean DLEDsCommand::isPaletteValid(void) {
  return paletteValid;
}

void DLEDsCommand::setPaletteValid(boolean paletteValid) {
  this->paletteValid = paletteValid;
}

boolean DLEDsCommand::isTimeSet(void) {
  return timeSet;
}

void DLEDsCommand::setTimeSet(boolean timeSet) {
  this->timeSet = timeSet;
}

boolean DLEDsCommand::isOnOffSet(void) {
  return onOffSet;
}

void DLEDsCommand::setOnOffSet(boolean onOffSet) {
  this->onOffSet = onOffSet;
}

boolean DLEDsCommand::isScheduled(void) {
  return scheduled;
}

boolean DLEDsCommand::isDelaySet(void) {
  return delaySet;
}

void DLEDsCommand::setDelaySet(boolean delaySet) {
  this->delaySet = delaySet;
}

boolean DLEDsCommand::isVersionQuery(void) {
  return versionQuery;
}

void DLEDsCommand::setVersionQuery(boolean versionQuery) {
  this->versionQuery = versionQuery;
}

boolean DLEDsCommand::isFileQuery(void) {
  return fileQuery;
}

void DLEDsCommand::setFileQuery(boolean fileQuery) {
  this->fileQuery = fileQuery;
}

boolean DLEDsCommand::isFileNumQuery(void) {
  return fileNumQuery;
}

void DLEDsCommand::setFileNumQuery(boolean fileNumQuery) {
  this->fileNumQuery = fileNumQuery;
}

boolean DLEDsCommand::isNameSet(void) {
  return nameSet;
}

void DLEDsCommand::setNameSet(boolean nameSet) {
  this->nameSet = nameSet;
}

boolean DLEDsCommand::isServerPinSet(void) {
  return serverPinSet;
}

boolean DLEDsCommand::isClientPinSet(void) {
  return clientPinSet;
}

void DLEDsCommand::setClientPinSet(boolean clientPinSet) {
  this->clientPinSet = clientPinSet;
}

int DLEDsCommand::getCurrentTime(void) {
  return currentTime;
}

int DLEDsCommand::getTimeOn(void) {
  return timeOn;
}

int DLEDsCommand::getTimeOff(void) {
  return timeOff;
}

uint8_t* DLEDsCommand::getBroadcastName(void) {
  return name;
}

uint8_t DLEDsCommand::getNameLen(void) {
  return name_len;
}

uint8_t* DLEDsCommand::getServerPin(void) {
  return serverPin;
}

uint8_t* DLEDsCommand::getClientPin(void) {
  return clientPin;
}

FileType_t DLEDsCommand::getCurrentFileType(void) {
  return currentFileType;
}

void DLEDsCommand::setCurrentFileType(FileType_t fileType) {
  this->currentFileType = fileType;
}

void DLEDsCommand::buttonCommand(ButtonPressed_t buttonPressed) {
  switch(buttonPressed) {
  case bUp :
  case bDown :
    buttonIndexCommand(button.getButtonIndex());
    break;
  case bLeft :
    if (pattern.getLightMode() == lmSolidFixed) {
      pattern.updateMarchIndex(true);
    }
    else {
      //Slow Down transition speed
      pattern.updateTransitionSpeed(1);
    }
    break;
  case bRight :
    if (pattern.getLightMode() == lmSolidFixed) {
      pattern.updateMarchIndex(false);
    }
    else {
      //Speed Up transition speed
      pattern.updateTransitionSpeed(-1);
    }
    break;
  default :
    return;
  }
}

void DLEDsCommand::buttonIndexCommand(int index) {
  SysPalette_t systemPalette;
  ButtonIndex_t buttonIndex = (ButtonIndex_t) index;
  
  switch (buttonIndex) {
   case bOff: //lights off
     turnLightsOff = true;
     break;
   case bChristmasFade:  // lights on Christmas colors
     systemPalette = dChristmas;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bValentinesFade:  // lights on Valentines colors
     systemPalette = dValentines;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bPatrickFade:  // lights on St. Patricks colors
     systemPalette = dPatrick;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bEasterFade:  // lights on Easter colors
     systemPalette = dEaster;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bJulyFade:  // lights on 4th of July colors
     systemPalette = dJuly;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bHalloweenFade:  // lights on Halloween colors
     systemPalette = dHalloween;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bThanksgivingFade:  // lights on Thanksgiving colors
     systemPalette = dThanksgiving;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     break;
   case bChristmasMarch:  // lights on Christmas colors
     systemPalette = dChristmas;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bValentinesMarch:  // lights on Valentines colors
     systemPalette = dValentines;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bPatrickMarch:  // lights on St. Patricks colors
     systemPalette = dPatrick;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bEasterMarch:  // lights on Easter colors
     systemPalette = dEaster;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bJulyMarch:  // lights on 4th of July colors
     systemPalette = dJuly;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bHalloweenMarch:  // lights on Halloween colors
     systemPalette = dHalloween;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bThanksgivingMarch:  // lights on Thanksgiving colors
     systemPalette = dThanksgiving;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmMarch);
     break;
   case bSolidFixed:  // lights on Solid colors
     systemPalette = dSolid;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmSolidFixed);
     break;
   case bRainbowMarch:  // lights on Rainbow colors
     systemPalette = dRainbow;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmRandomMarch);
     break;
   case bSolidFade:  // lights on Solid colors
     systemPalette = dSolid;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFade);
     break;
   case bRainbowFade:  // lights on Rainbow colors
     systemPalette = dRainbow;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmRandomFade);
     break;
   case bCandle:  // lights on Candle Glow
     systemPalette = dCandle;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmCandle);
     break;
   case bStar:  // lights on Star Twinkle
     systemPalette = dStar;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmStar);
     break;
   case bCandleRainbow:  // lights on Candle Glow
     systemPalette = dRainbow;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmCandle);
     break;
   case bPlasma:  // lights on Plasma
     systemPalette = dSolid;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmPlasma);
     break;
   default:
     return;
  }
  //Update the number of colors in the pattern class.
  pattern.setNumColors(palette.getNumColors());
  pattern.setPatternSet(true);
  
}

void DLEDsCommand::readAciBuffer(uint8_t* uart_buffer, uint8_t uart_buffer_len)
{
  
  delaySet = false;
    
  char firstByte = (char) uart_buffer[0];
  char lastByte = (char) uart_buffer[uart_buffer_len - 1];
  // If receiving is false, this is the first data packet in the command
  if(!receiving) {
    // Double check to make sure that the command structure is correct.
    if(firstByte == '(' && uart_buffer[1] == '%') {
      //Serial.println(F("Start of Command..."));
      bufferSize = 3 * ((int) uart_buffer[2] + 1) + 8;
      //Serial.println(F("Expected Buffer Size:"));
      //Serial.printf("%d\n", bufferSize);
      //Serial.println(F("Buffer Index:"));
      //numColors 0-255 maps 1-256
      //if number of colors is 256, then the uart_buffer for numColors is 0
//      if (bufferSize == 8) {
//        bufferSize = 3 * 256 + 8;
//      }
      bufferIndex = 0;
      receiving = true;
    }
    else if(firstByte == '(' && uart_buffer[1] == '$') {
      //Serial.println(F("Start of Flash Page..."));
      bufferSize = ((int) uart_buffer[2] + 1) + 4; //flash page size, plus 4 bytes for '(', '$', flashPageSize and ')'
      bufferIndex = 0;
      receiving = true;
    }
    else if(firstByte == '(' && uart_buffer[1] == '#') {
      //Serial.println(F("Start of Flash Name..."));
      bufferSize = ((int) uart_buffer[2]) + 4; //flash name size, plus 4 bytes for '(', '#', flashNameSize and ')'
      bufferIndex = 0;
      receiving = true;
    }
    else {
      //Single packet command (delay or single character)
      bufferSize = uart_buffer_len;
    }

  }
    
  for (int i=0; i < uart_buffer_len; i++) {
    command_buffer[bufferIndex + i] = uart_buffer[i];
  }
  bufferIndex = bufferIndex + uart_buffer_len;
  //Serial.printf("%d\n", bufferIndex);
    
  if(lastByte == ')') {
    // If bufferIndex is not close to bufferSize it means there is an errant ")" within the color palette
    // 5 accounts for the variability in the command length at the end of the command string
    if(bufferSize - bufferIndex < 5) {
      //Serial.println(F("Last Byte"));
      //Serial.println(F("End of Command."));
      commandFinished = true;
      receiving = false;
      bufferIndex = 0;
    }
  }
    
  if(bufferIndex > bufferSize) {
    Serial.println(F("Error receiving command"));
    receiving = false;
    bufferIndex = 0; //May be the reason the controller does not respond to further commands
  }
    
}

void DLEDsCommand::checkPinCommand(void) {

  //Reset commandFinished
  commandFinished = false;
  
  if(command_buffer[1] == '[') {
    setClientPin();
  }   
}

void DLEDsCommand::checkCommandType(void) {

  //Reset commandFinished
  commandFinished = false;
  
  char controlChar = command_buffer[1]; //Ignore the starting '(' character.
    
  //These are commands that have data attached
  switch (controlChar) {
    case '%' : paletteCommand(); break;
    case '^' : delayCommand(); break;
    case '*' : setTimeCommand(); break;
    case '@' : setTimeOnOff(); break;
    case 'N' : setBroadcastName(); break;
    case 'P' : setServerPin(); break;
    case '[' : break; //Client PIN already set, so ignore the command
    case 'f' : setFlashFileNum(); break;
    case '#' : flash.writeFlashFileName(command_buffer); returnChar = '#'; break;
    case '$' : flash.aciFlashPageWrite(command_buffer); break;
    //If no data attached
    default  : characterCommand(controlChar); break;
  }   
}

void DLEDsCommand::characterCommand(char controlChar)
{
  
  SysPalette_t systemPalette;
  
  switch (controlChar) {
   case 'o': //lights off
     turnLightsOff = true;
     returnChar = 'o';
     break;
   case 'd':  // lights on Default colors
     systemPalette = dDefault;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     returnChar = '!';
     break;
   case 'c':  // lights on Christmas colors
     systemPalette = dChristmas;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     returnChar = '!';
     break;
   case 'v':  // lights on Valentines colors
     systemPalette = dValentines;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     returnChar = '!';
     break;
   case 'p':  // lights on St. Patricks colors
     systemPalette = dPatrick;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     paletteValid = true;
     returnChar = '!';
     break;
   case 'e':  // lights on Easter colors
     systemPalette = dEaster;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     returnChar = '!';
     break;
   case 'j':  // lights on 4th of July colors
     systemPalette = dJuly;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     returnChar = '!';
     break;
   case 'h':  // lights on Halloween colors
     systemPalette = dHalloween;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     returnChar = '!';
     break;
   case 't':  // lights on Thanksgiving colors
     systemPalette = dThanksgiving;
     palette.setSystemPalette(systemPalette);
     pattern.setLightMode(lmFadeMarch);
     //Update the number of colors in the pattern class.
     pattern.setNumColors(palette.getNumColors());
     paletteValid = true;
     break;
   case '<': //Slow down color march/fade
     pattern.updateTransitionSpeed(1);
     returnChar = '<';
     break;
   case '>': //Speed up color march/fade
     pattern.updateTransitionSpeed(-1);
     returnChar = '>';
     break;
   case '=': //Stop color march/fade
     pattern.setLightMode(lmFixed);
     break;
   case 'x': // erase Flash - temporary command
     flash.eraseFlashFiles();
     returnChar = 'x';
     break;
   case 'r': //Print a section of flash memory (debugging only)
     flash.printFlashSector();
     returnChar = 'r';
     break;
   case 's': //Scan available flash files
     flash.scanFlash(); 
     returnChar = 's';
     break;
   case '!' : //Start playing flash file at currentFlashFile location
     pattern.setLightMode(lmFile);
     paletteValid = true;
     returnChar = '!';
     break;
   case '+': // Move to next image or sequence file and play
     if(pattern.getLightMode() == lmFile) {
       flash.updateCurrentFlashFile(true);
     }
     else {
       pattern.setLightMode(lmFile);
     }
     returnChar = '+';
     break;
   case '-':
     if(pattern.getLightMode() == lmFile) {
       flash.updateCurrentFlashFile(false);
     }
     else {
       pattern.setLightMode(lmFile);
     }
     returnChar = '-';
     break;
   case 'V' : //Query the version number of the firmware
     versionQuery = true;
     returnChar = '\0';
     break;
   case 'F' : //Query the number of available flash files
     fileNumQuery = true;
     returnChar = '\0';
     break;
   case '?' : //Query Name of flash file at currentFlashFile location
     fileQuery = true;
     returnChar = '\0';
     break;
   default:
     paletteValid = false;
     returnChar = '\0';
     return;
  }
}

void DLEDsCommand::paletteCommand(void)
{
  RGB_t color;
  LightMode_t lightMode;
  SysPalette_t systemPalette;
  
  //Skip first two characters
  int idx = 2;
  
  //Workaround for numColors mapping
//  uint8_t nColors = command_buffer[idx++];
//  if(nColors == 0) {
//    nColors = 255;
//  }
//  else {
//    nColors = nColors - 1;
//  }
//  
//  palette.setNumColors(nColors);
  //End Workarond
  palette.setNumColors(command_buffer[idx++]); // 0-255 maps to 1-256

  //Serial.print("Number of Colors = ");
  //Serial.println(palette.getNumColors() + 1,DEC);
  
  for (int i=0; i <= palette.getNumColors(); i++)
  {
    color.r = command_buffer[idx++];
    color.g = command_buffer[idx++];
    color.b = command_buffer[idx++];
    palette.setColor(color, i);
  }
  
  char command = (char) command_buffer[idx++];
  switch (command) {
    case '~': 
      break;
    case ')': 
      paletteValid = true;
      returnChar = '!';
      pattern.setLightMode(lmFixed);
      return;
    default : 
      paletteValid = false;
      returnChar = '\0';
      return;
  }
  
  palette.setPaletteSet(true);
  
  command = (char) command_buffer[idx++];
  
  switch (command) {
    case 'l' :
      lightMode = lmFixed; break;
    case 'm' :
      lightMode = lmMarch; break;
    case 'f' :
      lightMode = lmFadeMarch; break;
    case 'g' :
      lightMode = lmFade; break;
    case 'r' :
      lightMode = lmRandomMarch; break;
    case 'R' :
      lightMode = lmRandomFade; break;
    case 'c' :
      lightMode = lmCandle; 
      if (palette.getNumColors() != 255) {
        systemPalette = dCandle;
        palette.setSystemPalette(systemPalette);
      }
      break;
    case 's' :
      lightMode = lmStar; 
      if (palette.getNumColors() != 255) {
        systemPalette = dStar;
        palette.setSystemPalette(systemPalette);
      }
      break;
    case 'p' :
      lightMode = lmPlasma; break;
    default:
      lightMode = lmOff;
      paletteValid = false;
      returnChar = '\0';
      return;
  }
  
  if(lightMode == lmFixed) {
    command = (char) command_buffer[idx++];
  }
  else {
//    Serial.print("Fade Delay: 0x");
//    Serial.println(command_buffer[idx],HEX);
    pattern.setFadeDelay(command_buffer[idx++]);
//    Serial.print("March Delay: 0x");
//    Serial.println(command_buffer[idx],HEX);
    pattern.setMarchDelay(command_buffer[idx++]);
    command = (char) command_buffer[idx++];
  }
  
  if (command != ')') {
    Serial.println(F("Command is not complete!"));
    paletteValid = false;
    returnChar = '\0';
    return;
  }
  else {
    pattern.setNumColors(palette.getNumColors());
    pattern.setMarchIndex(0);
    pattern.setLightMode(lightMode);
    pattern.setPatternSet(true);
    paletteValid = true;
    returnChar = '!';
  }
  
}

void DLEDsCommand::delayCommand(void)
{
  //Skip first two characters
  int idx = 2;
  
  byte byteArray[4];
  int msDelay;
  
  int timeDelay;
  
  for (int i=0; i < sizeof(int); i++) {
    byteArray[i] = command_buffer[idx++];
  }
  msDelay = ((byteArray[0] << 24)|(byteArray[1] << 16)|(byteArray[2] << 8)|(byteArray[3]));

  if (msDelay < 0)
  {
    Serial.println(F("Delay Time not correct!"));
    return;
  }
  
  // (3/90) = 0.03s = THIRTIETH_SECOND scaling for IRC
  timeDelay = (int)(msDelay * 3 / 90);
  pattern.setPatternDelay(timeDelay);
  delaySet = true;
  //No response until delay is finished
  returnChar = '\0';
  
}

void DLEDsCommand::setTimeCommand(void)
{
  //Skip first two characters
  int idx = 2;
  
  byte byteArray[4];
  
  for (int i=0; i < sizeof(time_t); i++) {
    byteArray[i] = command_buffer[idx++];
  }
  currentTime = ((byteArray[0] << 24) | (byteArray[1] << 16) | (byteArray[2] << 8) | (byteArray[3]));

  if (currentTime < 0)
  {
    Serial.println(F("Current Time not correct!"));
    return;
  }
  
  timeSet = true;
  returnChar = '*';
  
}

void DLEDsCommand::setTimeOnOff(void)
{
  //Skip first two characters
  int idx = 2;
  byte byteArray[4];
  
  for (int i=0; i < sizeof(time_t); i++) {
    byteArray[i] = command_buffer[idx++];
  }
  timeOn = ((byteArray[0] << 24) | (byteArray[1] << 16) | (byteArray[2] << 8) | (byteArray[3]));

  for (int i=0; i < sizeof(time_t); i++) {
    byteArray[i] = command_buffer[idx++];
  }
  timeOff = ((byteArray[0] << 24) | (byteArray[1] << 16) | (byteArray[2] << 8) | (byteArray[3]));

  if (timeOn == 0 && timeOff == 0) {
    onOffSet = false;
    scheduled = false;
    return;
  }
  
  onOffSet = true;
  scheduled = true;
  returnChar = '@';
}

void DLEDsCommand::setBroadcastName(void)
{
  //Skip first 2 command characters
  int idx = 2;
  int count = 0;
  
  // 8 is max length of broadcast name
  for (int i=0; i < MAX_NAME_LEN; i++) {
    if((char) command_buffer[idx] != ')'){
      name[i] = command_buffer[idx++];
      count++;
    }
    else {
      break;
    }
  }
  name_len = count;
  nameSet = true;
  returnChar = 'N';
}

void DLEDsCommand::setServerPin(void)
{
  //Skip first 2 command characters
  int idx = 2;
  int count = 0;
  
  // 4 is max length of PIN
  for (int i=0; i < MAX_PIN_LEN; i++) {
    if((char) command_buffer[idx] != ')'){
      serverPin[i] = command_buffer[idx++];
      count++;
    }
    else {
      break;
    }
  }
  //If count is 0, reset pin
  if (count == 0) {
    serverPinSet = false;
    clientPinSet = false;
    returnChar = 'P';
  }
  else if (count != 4) {
    serverPinSet = false;
    returnChar = '\0';
  }
  else {
    serverPinSet = true;
    returnChar = 'P';
  }
}

void DLEDsCommand::setClientPin(void)
{
  //Skip first 2 command characters
  int idx = 2;
  int count = 0;
  
  // 4 is max length of broadcast name
  for (int i=0; i < MAX_PIN_LEN; i++) {
    if((char) command_buffer[idx] != ')'){
      clientPin[i] = command_buffer[idx++];
      count++;
    }
    else {
      break;
    }
  }
  if (count != 4) {
    clientPinSet = false;
    returnChar = '\0';
  }
  else {
    clientPinSet = true;
    returnChar = '[';
  }
}

void DLEDsCommand::setFlashFileNum()
{
  //Skip starting '(' and 'f' characters
  int idx = 2;
  int fileIndex = command_buffer[idx++];
  //Serial.print("Set Flash Index to: ");
  //Serial.println(fileIndex,DEC);
  if (command_buffer[idx] != ')') {
    //If command is not formed correctly ignore it
    return;
  }
  
  flash.setCurrentFlashFile(fileIndex);
  returnChar = 'f';
}

void DLEDsCommand::updateFadePalette(int percent, boolean marchForward) {
  RGB_t firstColor;
  RGB_t nextColor;
  RGB_t fadeColor;
  
  for (int i=0; i <= palette.getNumColors(); i++) {
    firstColor = palette.getColor(i);
    if (i == palette.getNumColors()) {
      nextColor = palette.getColor(0);
    }
    else {
      nextColor = palette.getColor(i+1);
    }
    if (marchForward == true) {
      fadeColor = palette.getColorFromColor(nextColor, firstColor, percent);
    }
    else {
      fadeColor = palette.getColorFromColor(firstColor, nextColor, percent);
    }
    fadePalette.setColor(fadeColor, i);
  }
  fadePalette.setNumColors(palette.getNumColors());  
}

void DLEDsCommand::updateFadePaletteRandom(int percent, boolean marchForward) {
  RGB_t firstColor;
  RGB_t nextColor;
  RGB_t fadeColor;
  
  int seed = 3919;
  int currentColorIdx;
  
  //Use fixed seed so that fade palette index is constant during fade
  randomSeed(seed);
  
  //Start with initial random index, next color will be chosen next to the previous one
  currentColorIdx = random(0, palette.getNumColors());
  
  for (int i=0; i <= palette.getNumColors(); i++) {
    firstColor = palette.getColor(currentColorIdx);
    currentColorIdx = random(0, palette.getNumColors());
    nextColor = palette.getColor(currentColorIdx);
    if (marchForward == true) {
      fadeColor = palette.getColorFromColor(nextColor, firstColor, percent);
    }
    else {
      fadeColor = palette.getColorFromColor(firstColor, nextColor, percent);
    }
    fadePalette.setColor(fadeColor, i);
  }
  fadePalette.setNumColors(palette.getNumColors());  
}

int DLEDsCommand::flashUpdatePalette(int startAddr) {
  int tAddr = startAddr;
  byte readBuf[3];
  RGB_t color;

  tAddr++; // function enterred at '%' character
  flash.readFlash(tAddr, 1, readBuf);
  palette.setNumColors((uint8_t) readBuf[0]); // read number of bytes 0-255 maps to 1-256
  pattern.setNumColors((uint8_t) readBuf[0]);
  tAddr++; // Advance to start of palette array

  int i;
  for (i=0; i <= palette.getNumColors(); i++) {
    flash.readFlash(tAddr, 3, readBuf);
    color.r = readBuf[0];
    color.g = readBuf[1];
    color.b = readBuf[2];
    palette.setColor(color, (uint8_t) i);
    tAddr = tAddr + 3;  
  }
  while (i < 256) {
      color = {0, 0, 0};
      palette.setColor(color, (uint8_t) i); // Pad rest of palette with black
      i++;  
   }
   //Serial.println("P");
   return(tAddr);
}
  

