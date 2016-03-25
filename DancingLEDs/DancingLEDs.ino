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

#include <SPI.h>
#include <OctoWS2811.h>
#include <TimerThree.h>
#include <Time.h>
#include <TimeAlarms.h>

#include <lib_aci.h>
#include <aci_setup.h>
#include "uart_over_ble.h"

#define DANCINGLEDS_CONTROLLER_VERSION 3

#define MAXLEDSPERCHANNEL 500
#define MAXCHANNELS 8
#define MAXLEDS 4000

const int ledsPerChannel = MAXLEDSPERCHANNEL; // initialize to max
int ledsTotal = ledsPerChannel * MAXCHANNELS; // initialize to max

#define MAX_FRAME_INTERVAL 65535
#define THIRTIETH_SECOND 33333
#define SIXTIETH_SECOND 16667

#define COLS_LEDs 60  // all of the following params need to be adjusted for screen size
#define ROWS_LEDs 16  // LED_LAYOUT assumed 0 if ROWS_LEDs > 8
#define LEDS_PER_STRIP (COLS_LEDs * (ROWS_LEDs / 6))

// display and drawing memory are 24 bytes (6 ints) per channel
DMAMEM int displayMemory[ledsPerChannel*6];
int drawingMemory[ledsPerChannel*6];

//const int config = WS2811_GRB | WS2811_800kHz;
const int config = WS2811_RGB | WS2811_800kHz;

//Variables for gamma correction of color values
extern const uint8_t exp_gamma[]; //defined in dleds_pattern.h
boolean applyGamma = false;

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
*/

//Dancing LEDs Class Header Files
#include "dleds_aci.h"
#include "dleds_button.h"
#include "dleds_command.h"
#include "dleds_flash.h"
#include "dleds_palette.h"
#include "dleds_pattern.h"
#include "dleds_serial.h"

//Class Initializations

OctoWS2811 leds(ledsPerChannel, displayMemory, drawingMemory, config);

DLEDsACI aci;
DLEDsCommand command(ledsTotal);
DLEDsSerial ser;

//Volatile Variables for Timer3 IRCs
volatile int frameUnits = MAX_FRAME_INTERVAL;
volatile int nextFrameUnits;
volatile int holdFrames = 1;
volatile int nextHoldFrames;
volatile int holdCount = 0;
volatile boolean flashTransferPending = false;
volatile boolean updateFrameFlag = false;
volatile int skipCount = 0;
volatile boolean videoStarted = false;
int videoAddr;
byte readBuf[4];
 
volatile boolean marchStarted = false;
volatile boolean fadeStarted = false;
volatile boolean delayFinished = false;

volatile int marchCount = 0;
volatile int fadeCount = 0;
volatile int delayCount = 0;

//ACI Required Function definition
/* Define how assert should function in the BLE library */
void __ble_assert(const char *file, uint16_t line)
{
  Serial.print("ERROR ");
  Serial.print(file);
  Serial.print(": ");
  Serial.print(line);
  Serial.print("\n");
  while(1);
}

//Initialize the Controller
void setup(void)
{
  //Serial setup
  ser.begin();
  
  //Flash setup
  command.flash.begin();
  
  //Setup Button Hardware
  command.button.begin();
  
  //Wait until the serial port is available (useful only for the Leonardo)
  //As the Leonardo board is not reseted every time you open the Serial Monitor
  #if defined (__AVR_ATmega32U4__)
    while(!Serial)
    {}
    delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
  #elif defined(__PIC32MX__)
    delay(1000);
  #endif
  
   //Initialize OctoWS2811
  leds.begin();
  leds.show();

  Serial.println(F("Dancing LEDs Controller Setup..."));

  //Setup nRF8001 Services
  aci.begin();
  
  Serial.println(F("Set up done!"));
  
  //Scan Flash must be performed during setup to set the write address to the correct value
  command.flash.scanFlash();

  //Start the Timer3 instance
  Timer3.initialize(MAX_FRAME_INTERVAL);
  
  //Initialize Pattern Variables
  command.pattern.begin();

}

void loop() {
  
  // Check 32MHz transactions to the nor Flash
  checkSerialEvt();

  //Otherwise process any ACI commands or events
  if (!flashTransferPending) {
    aci.checkAciEvt();
  }
  
  // Interpret any light command from aci_loop or serial_loop
  if (aci.isCommandReceived()) 
  {
    //Timer3.detachInterrupt();
    
    //Reset delay finished if new command is received.
    delayFinished = false;
    videoStarted = false; // Need to indicate video is stopped so timer will be restarted
    
    command.readAciBuffer(aci.getUartBuffer(), aci.getUartBufferLen());
    aci.clearUartBuffer();
    aci.setCommandReceived(false);
  }
  
  if (command.isCommandFinished()) {
    //Serial.println(F("Command Finished!"));
    
    command.checkPinCommand();
    
    if(command.isClientPinSet()) {
      aci.setClientPin(command.getClientPin());
      command.setClientPinSet(false);
    }
    
    if(command.isServerPinSet()) {
      boolean checkPin = true;
      uint8_t* serverPin = command.getServerPin();
      uint8_t* clientPin = aci.getClientPin();
      for(int i=0; i < MAX_PIN_LEN; i++) {
        if (serverPin[i] != clientPin[i]) {
          checkPin = false;
        }
      }
      if (checkPin == false) {
        //Do we need to disconnect or is it enough to ignore commands?
        //aci.uart_disconnect();
        while(command.flash.busy()) {
          delay(1);
        }
        aci.sendChar(']');
        return;
      }
    }
    
    command.checkCommandType();
    
    if (command.isPaletteValid()) {
      startLightTimer();
      command.setPaletteValid(false);
    }
    
    if(command.getReturnChar() != '\0') {
      //Return the acknowledge character
      while(command.flash.busy()) {
        delay(1);
      }
      aci.sendChar(command.getReturnChar());
    }
    
  }
  
  if(command.flash.isAciPageReceived()) {
    while(command.flash.busy()) {
      delay(1);
    }
    aci.sendChar(':');
    command.flash.setAciPageReceived(false);
  }
  
  //Check IRCs to see if any events need to be processed
  lightTimerTrigger();
  
  //Update button status
  command.button.updateButtonStatus();
  if(command.button.isButtonChanged()) 
  {
    //Call buttonCommand in response to button change
    command.buttonCommand(command.button.getButtonPressed());
    startLightTimer();
  }
  
  if(command.isTurnLightsOff()) {
    lightsOff();
    command.setTurnLightsOff(false);
  }

  // For Sequence Delay Commands
  if (command.isDelaySet()) {
    delayTimerTrigger();
  }
  
  //For scheduled on/off update AlarmTimer
  if (command.isTimeSet()) {
    //Set the current time into the timer.
    setTime((time_t) command.getCurrentTime());
  
    //Print the current time
    printTime(now());
    
    command.setTimeSet(false);
  }
  
  if (command.isOnOffSet()) {
    setAlarms();
  }
  
  //Response to Version Query Commands
  if (command.isVersionQuery()) {
    while(command.flash.busy()) {
      delay(1);
    }
    //aci.sendChar((char) DANCINGLEDS_CONTROLLER_VERSION);
    sendVersionResponse();

    command.setVersionQuery(false);
  }
  
  //Response to File Number Query Command
  if (command.isFileNumQuery()) {
    while(command.flash.busy()) {
      delay(1);
    }
    
    if (command.flash.getFlashFiles() != 0) {
      //Return number of flash files 0-255 maps 1-256
      //aci.sendChar((char) (command.flash.getFlashFiles() - 1));
      sendNumFlashFiles();
    }
    else {
      //Removed no response to simplify connection code
      //There should be no command that has no acknowledgement
      aci.sendChar('F');
    }
    command.setFileNumQuery(false);
  }
  
  //Response to File Query Command
  if (command.isFileQuery()) {
    sendFlashFileName(command.flash.getCurrentFlashFile());
    command.setFileQuery(false);
  }
  
  //Response to Set Broadcast Name Command
  if(command.isNameSet()) {
    aci.setName(command.getBroadcastName(), command.getNameLen());
    command.setNameSet(false);
  }
  
  //Alarm.delay updates the alarm triggers. Alarm.delay(0) seemed to give inconsistent start times.
  Alarm.delay(1);
  
  //For ChipKit you have to call the function that reads from Serial
  #if defined (__PIC32MX__)
    if (Serial.available())
    {
      serialEvent();
    }
  #endif
  
}

// startLightTimer initializes the pattern/image IRCs and sets the initial pixels
// of the lights if necessary
// startLightTimer is called only if a valid command/button event has been received
void startLightTimer() {
  LightMode_t lightMode;
  
  Palette_t singleColor;
  
  //Set IRC flags back to default
  restartPattern();
  
  lightMode = command.pattern.getLightMode();
  
  switch(lightMode) {
  case lmOff :
    lightsOff();
    break;
  case lmFixed :
    Serial.println(F("Fixed!"));
    Timer3.detachInterrupt();
    updateLightPalette(command.palette.getPalette(), command.pattern.getMarchIndex());
    break;
  case lmSolidFixed :
    Serial.println(F("Fixed Solid!"));
    Timer3.detachInterrupt();
    singleColor = command.palette.getSolidColorPalette(command.pattern.getMarchIndex());
    updateLightPalette(singleColor, 0);
    break;
  case lmMarch :
    Serial.println(F("March!"));
    Timer3.detachInterrupt();
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(marchIRC);
    updateLightPalette(command.palette.getPalette(), command.pattern.getMarchIndex());
    break;
  case lmFadeMarch :
    Serial.println(F("Fade March!"));
    Timer3.detachInterrupt();
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(fadeIRC);
    break;
  case lmFade :
    Serial.println(F("Fade!"));
    Timer3.detachInterrupt();
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(fadeIRC);
    break;
  case lmRandomMarch :
    Serial.println(F("Random March!"));
    Timer3.detachInterrupt();
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(marchIRC);
    updateLightPaletteRandom(command.palette.getPalette(), command.pattern.getMarchIndex());
    break;
  case lmRandomFade :
    Serial.println(F("Random Fade!"));
    Timer3.detachInterrupt();
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(fadeIRC);
    break;
  case lmCandle :
    Serial.println(F("Candle Glow!"));
    Timer3.detachInterrupt();
    command.pattern.setCandleParameters(lightMode);
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(candleGlowIRC);
    break;
  case lmStar :
    Serial.println(F("Star Twinkle!"));
    Timer3.detachInterrupt();
    command.pattern.setCandleParameters(lightMode);
    Timer3.setPeriod(THIRTIETH_SECOND);
    Timer3.attachInterrupt(candleGlowIRC);
    break;  
  case lmPlasma :
    Serial.println(F("Plasma Glow!"));
    Timer3.detachInterrupt();
    Timer3.setPeriod(SIXTIETH_SECOND);
    Timer3.attachInterrupt(plasmaIRC);
    break;
  case lmFile:
    Serial.println(F("Flash!"));
    Timer3.detachInterrupt();
    //Start the initial file play command
    playLightsFromFile(command.flash.getCurrentFlashFAT());
    break;
  case lmSerial:
    // TODO
    break;
  case lmBle:
    // TODO
    break;
  default :
    Timer3.detachInterrupt();
    break;
  }

}

// lightTimerTrigger is called on every iteration of loop()
// It checks the Timer3 IRC values and updates the patterns/images
// if an update is required
void lightTimerTrigger() {
  LightMode_t lightMode;
  
  Palette_t singleColor;
  
  boolean marchStartedCopy;
  boolean fadeStartedCopy;
  boolean updateFrameFlagCopy;
  
  int fadeCountCopy;
  
  int percent;
  
  lightMode = command.pattern.getLightMode();
  
  noInterrupts();
  marchStartedCopy = marchStarted;
  fadeStartedCopy = fadeStarted;
  fadeCountCopy = fadeCount;
  updateFrameFlagCopy = updateFrameFlag;
  interrupts();
  
  switch (lightMode) {
  case lmMarch : 
    if (marchStartedCopy) {
      //Serial.println("M");
      command.pattern.updateMarchIndex(true);
      updateLightPalette(command.palette.getPalette(), command.pattern.getMarchIndex());
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
  case lmFadeMarch : 
    if (fadeStartedCopy) {
      percent = (int) 99*fadeCountCopy/command.pattern.getFadeDelay() + 1;
      //Serial.print("F! %: ");
      //Serial.println(percent);
      command.updateFadePalette(percent, true);
      updateLightPalette(command.fadePalette.getPalette(), command.pattern.getMarchIndex());
      noInterrupts();
      fadeStarted = false;
      interrupts();
    }
    else if (marchStartedCopy) {
      //Serial.println("M2");
      command.pattern.updateMarchIndex(true);
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
    case lmFade : 
    if (fadeStartedCopy) {
      percent = (int) 99*fadeCountCopy/command.pattern.getFadeDelay() + 1;
      //Serial.print("F2! %: ");
      //Serial.println(percent);
      command.updateFadePalette(percent, false);
      singleColor = command.fadePalette.getSolidColorPalette(command.pattern.getMarchIndex());
      updateLightPalette(singleColor, 0);
      noInterrupts();
      fadeStarted = false;
      interrupts();
    }
    else if (marchStartedCopy) {
      //Serial.println("M3");
      command.pattern.updateMarchIndex(false);
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
    case lmRandomMarch : 
    if (marchStartedCopy) {
      //Serial.println("M4");
      command.pattern.updateMarchIndex(true);
      updateLightPaletteRandom(command.palette.getPalette(), command.pattern.getMarchIndex());
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
    case lmRandomFade : 
    if (fadeStartedCopy) {
      percent = (int) 99*fadeCountCopy/command.pattern.getFadeDelay() + 1;
      //Serial.print("F3! %: ");
      //Serial.println(percent);
      command.updateFadePaletteRandom(percent, true);
      updateLightPalette(command.fadePalette.getPalette(), command.pattern.getMarchIndex());
      noInterrupts();
      fadeStarted = false;
      interrupts();
    }
    else if (marchStartedCopy) {
      //Serial.println("M5");
      command.pattern.updateMarchIndex(true);
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
  case lmCandle :
    if (marchStartedCopy) {
      //Serial.println("C");
      candleGlow();
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
  case lmStar :
    if (marchStartedCopy) {
      //Serial.println("C");
      candleGlow();
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
  case lmPlasma :
    if (marchStartedCopy) {
      //Serial.println("P");
      plasmaGlow();
      noInterrupts();
      marchStarted = false;
      interrupts();
    }
    break;
  case lmFile :
    if (command.getCurrentFileType() == ftVideo) {
      if (videoStarted && updateFrameFlagCopy) {
        videoAddr = playFlashVideo(videoAddr);
        noInterrupts();
        updateFrameFlag = false;
        interrupts();
      }
      // Do nothing if it's not time for a frame update
    }
    else { //If file is not a video
      noInterrupts();
      updateFrameFlag = false;
      videoStarted = false;
      interrupts();
    }
    break;
  case lmSerial :
      // TODO       
    break;
  case lmBle :
      // TODO
    break;
  default:
    break;
  }
  
}

// This trigger controls the delays required for sequences
// It waits for the period specified by the delay command
// then sends the '^' response to the app to trigger the
// next pattern command in the sequence
void delayTimerTrigger() {
  
  boolean delayFinishedCopy;
  
  noInterrupts();
  delayFinishedCopy = delayFinished;
  interrupts();
  
  if (delayFinishedCopy) {
    //Send Delay Finished Acknowledgement to client
    while(command.flash.busy()) {
      delay(1);
    }
    aci.sendChar('^');
    
    noInterrupts();
    delayFinished = false;
    interrupts();
    command.setDelaySet(false);
  }
  
}

//Print the time/alarm time
void printTime(time_t time) {
  TimeElements tm;
  breakTime(time, tm);
  Serial.print("Time is: ");
  Serial.print(tm.Hour,DEC);
  Serial.print(":");
  if(tm.Minute < 10) Serial.print("0");
  Serial.print(tm.Minute,DEC);
  Serial.print(" ");
  Serial.print(tm.Month,DEC);
  Serial.print("/");
  Serial.print(tm.Day,DEC);
  Serial.print("/");
  // offset from 1970
  Serial.println(tm.Year - 30,DEC);
}

// Set the time to turn lights on and off
// Currently only one alarm set is supported
//TODO: allow multiple alarms to be set and cleared
void setAlarms() {
  TimeElements tm;
    
  //Clear current Scheduled times
  //May not be necessary after sending clear alarms from app
  for (int i=0; i < dtNBR_ALARMS; i++) {
    Alarm.disable(i);
    Alarm.free(i);
  }
    
  Serial.print(F("On "));
  printTime((time_t) command.getTimeOn());
  
  breakTime((time_t) command.getTimeOn() ,tm);
  Alarm.alarmRepeat(tm.Hour, tm.Minute, 0, lightsOnAlarm);
  
  Serial.print(F("Off "));
  printTime((time_t) command.getTimeOff());
  
  breakTime((time_t) command.getTimeOff() ,tm);
  Alarm.alarmRepeat(tm.Hour, tm.Minute, 0, lightsOffAlarm);
    
  command.setOnOffSet(false);
}

void lightsOnAlarm() {
  Serial.println(F("Turning Lights On!"));
  printTime(now());
  if(!command.palette.isPaletteSet() && !command.pattern.isPatternSet()) {
    //If there is no previous setting default to plasma
    LightMode_t lightMode = lmPlasma;
    command.pattern.setLightMode(lightMode);
  }
  startLightTimer();
}

void lightsOffAlarm() {
  Serial.println(F("Turning Lights Off!"));
  printTime(now());
  lightsOff();
}

void lightsOff() {
  RGB_t colorOff = {0,0,0};
  Palette_t paletteOff = {&colorOff,0};
  updateLightPalette(paletteOff, 0);
  
  //Turn off patterns
  Timer3.detachInterrupt();
  restartPattern();
  
  //Reset March Index to 0
  command.pattern.setMarchIndex(0);
}

void restartPattern()
{
  noInterrupts();
  marchCount = 0;
  fadeCount = 0;
  delayCount = 0;
  holdCount = 0;
  
  marchStarted = false;
  fadeStarted = false;
  delayFinished = false;
  videoStarted = false;
  interrupts();
}

// Update the light pixels with an indexed color from the given palette
void updateLightPalette(struct Palette_t palette, int startIdx) {
  int currentColorIdx = startIdx;
  RGB_t color;
  
  for(int i=0; i < ledsTotal; i++) {
    if(currentColorIdx == palette.nColors + 1) { //0-255 maps 1-256
      currentColorIdx = 0;
    }
    color = palette.colors[currentColorIdx++];
    
    if (applyGamma == true) {
      color.r = exp_gamma[color.r];
      color.g = exp_gamma[color.g];
      color.b = exp_gamma[color.b];
    }
    
    leds.setPixel(i, color.r, color.g, color.b);
  }
  
  if(!leds.busy()) {
    leds.show();
  }
}

// Update the light pixels with a random color from the given palette
void updateLightPaletteRandom(struct Palette_t palette, int startIdx) {
  int currentColorIdx; 
  RGB_t color;
 
  for(int i=0; i < ledsTotal; i++) {
    //0-255 maps 1-256
    currentColorIdx = random(0, palette.nColors);
    color = palette.colors[currentColorIdx];
    
    if (applyGamma == true) {
      color.r = exp_gamma[color.r];
      color.g = exp_gamma[color.g];
      color.b = exp_gamma[color.b];
    }
    
    leds.setPixel(i, color.r, color.g, color.b);
  }
  
  if(!leds.busy()) {
    leds.show();
  }
}

//updateLightImage sends an image received from Serial to the lights
//This is untested in v2 controller firmware
void updateLightImage() {

  RGB_t color;
  
  for(int i=0; i < ser.getNumImagePixels(); i++) {
    color = ser.getNextPixel();
    
    if (applyGamma == true) {
      color.r = exp_gamma[color.r];
      color.g = exp_gamma[color.g];
      color.b = exp_gamma[color.b];
    }
    
    leds.setPixel(i, color.r, color.g, color.b);
  }
  if(ser.getGoodImage()) {
    if(!leds.busy()) {
      leds.show();
    }
  }
}

//IRC for Color March Timer
void marchIRC(void)
{
  marchCount++;
  if(marchCount >= command.pattern.getMarchDelay()) {
    marchStarted = true;
    marchCount = 0;
  } 
  //Delay incrementer
  delayCount++;
  if(delayCount >= command.pattern.getPatternDelay()) {
    delayFinished = true;
    delayCount = 0;
  } 
}

//IRC for Color Fade Timer
void fadeIRC(void)
{
  fadeCount++;
  if(fadeCount <= command.pattern.getFadeDelay()) {
    fadeStarted = true;
  }
  else {
    marchCount++;
  }
  if(marchCount >= command.pattern.getMarchDelay()) {
    marchStarted = true;
    marchCount = 0;
    fadeCount = 0;
  }
  //Delay incrementer
  delayCount++;
  if(delayCount >= command.pattern.getPatternDelay()) {
    delayFinished = true;
    delayCount = 0;
  } 
}

//IRC for Candle Glow Timer
void candleGlowIRC(void)
{
  //Trigger CandleGlow on every update of Timer.
  marchStarted = true;
  //Delay incrementer
  delayCount++;
  if(delayCount >= command.pattern.getPatternDelay()) {
    delayFinished = true;
    delayCount = 0;
  } 
}

//IRC for Plasma Timer
void plasmaIRC(void)
{
  //Trigger PlasmaGlow on every update of Timer.
  marchStarted = true;
  
  //Delay incrementer
  delayCount++;
  if(delayCount >= command.pattern.getPatternDelay()) {
    delayFinished = true;
    delayCount = 0;
  } 
}

void candleGlow(void) {
  RGB_t color;
  for (int i = 0; i < ledsTotal; i++) {
     command.pattern.updateIntensity(i);
     color = command.palette.getColor(command.pattern.getIntensity(i));
     
     if(applyGamma == true) {
       color.r = exp_gamma[color.r];
       color.g = exp_gamma[color.g];
       color.b = exp_gamma[color.b];
     }
     
     leds.setPixel(i, color.r, color.g, color.b);
  }
  
  if (!leds.busy()) {
    leds.show();
  }
}

void plasmaGlow() {
  RGB_t color;
  
  //Set t, t2 and t3 values for the Plasma calculation
  command.pattern.setPlasmaParameters();

  for (uint8_t y = 0; y < ROWS_LEDs; y++) {
    int left2Right, pixelIndex;
    if (((y % (ROWS_LEDs/8)) & 1) == 0) {
      left2Right = 1;
      pixelIndex = y * COLS_LEDs;
    } else {
      left2Right = -1;
      pixelIndex = (y + 1) * COLS_LEDs - 1;
    }
    for (uint8_t x = 0; x < COLS_LEDs ; x++) {
      
      color = command.pattern.getPlasmaPixel(x, y);
      
      if (applyGamma == true) {
        color.r = exp_gamma[color.r];
        color.g = exp_gamma[color.g];
        color.b = exp_gamma[color.b];
      }
      
      leds.setPixel(pixelIndex, color.r, color.g, color.b);
      pixelIndex += left2Right;
    }
  }
  if (!leds.busy()) {
    leds.show();
  }
}

void sendVersionResponse() {
  uint8_t responseLen = 2 * sizeof(char);
  char *response = (char*) malloc(responseLen);
  response[0] = 'V';
  response[1] = (char) DANCINGLEDS_CONTROLLER_VERSION;
  
  while(command.flash.busy()) {
    delay(1);
  }
  aci.sendString(response, 2);
}

void sendNumFlashFiles() {
  uint8_t responseLen = 2 * sizeof(char);
  char *response = (char*) malloc(responseLen);
  response[0] = 'F';
  //0-255 maps 1-256
  response[1] = (char) (command.flash.getFlashFiles() - 1);
  
  while(command.flash.busy()) {
    delay(1);
  }
  aci.sendString(response, 2);
}

void sendFlashFileName(int fileNum) {
  
  byte fileName[MAX_FILENAME_SIZE];
  uint8_t nameLen;
  
  nameLen = command.flash.getFlashFileName(fileNum, fileName);

  while(command.flash.busy()) {
    delay(1);
  }
  aci.sendString((char*) fileName, nameLen);
  
}

void playLightsFromFile(int fileStartAddr) {

   Serial.print(F("playLightsFromFile: "));
   Serial.print(command.flash.getCurrentFlashFile(), DEC);
   Serial.print(", Addr: ");
   Serial.println(fileStartAddr, DEC);
   
   Timer3.detachInterrupt();
   videoStarted = false;
   
   FileType_t fileType = command.flash.checkFileType(fileStartAddr);
   command.setCurrentFileType(fileType); // assign global variable for use in light_loop
   Serial.print("File Type: ");
   if (fileType == ftImage) {
     Serial.println("Image");
     showFlashImage(fileStartAddr);  
   }
   else if (fileType == ftCompImage) {
     Serial.println("Comp Image");
     showCompImage(fileStartAddr);
   }
   else if (fileType == ftVideo) {
     Serial.println("Video");
     videoAddr = playFlashVideo(fileStartAddr);
   }
}

byte reverseBits (byte inByte) {
    static const uint8_t reverse_lookup[] = { 0, 8,  4, 12, 2, 10, 6, 14,1, 9, 5, 13,3, 11, 7, 15 };  
    byte outByte = (((reverse_lookup[(inByte & 0x0F)]) << 4) + reverse_lookup[((inByte & 0xF0) >> 4)]);
    return(outByte);
}
// Problems encountered running nRF8001 with new SPI library, so going back to original
// All bit reversal done in wrapper function so Flash won't have to be rewritting if I ever
// get the new SPI library working without "ACI Evt Pipe Error: Pipe #8 Pipe Error Code 0x83"

/*
byte norSPItransfer(byte sendByte) {
  byte tempByte;
  tempByte = SPI.transfer(reverseBits(sendByte));
  return(reverseBits(tempByte));
}
*/

void checkSerialEvt()
{
   if (Serial.available() > 0) {
      // read the incoming byte:
      int firstByte = Serial.read();
      if (firstByte == '(') // start of image line
      {
        if (command.pattern.getLightMode() != lmOff) { // turn off patterns in progress
          lightsOff();
        }
        int secondByte = Serial.read();
        switch (secondByte) {
           case '!': 
             ser.updateNumImagePixels();
             updateLightImage();
             break;        
           case '*': 
             //TODO
             //serialGetFrame();
             break;
           case '@': 
             //TODO
             //serialGetCompImage();
             break;
           case '&': 
             //TODO
             //serialGetPartFrame();
             break;
           case '^': 
             //TODO
             //serialGetHoldTime(); 
             break; 
           case '%': 
             //TODO
             //serialGetPalette(); 
             break;
           case '(': 
             //TODO
             //flash.serialFlashFileOpen(); 
             break;
           case '$': 
             command.flash.serialFlashPageWrite(); 
             break;
           case ')': 
             //TODO
             //flash.serialFlashFileClose();
             break;
           case 'x': 
             //TODO
             //flash.serialEraseFlash();
             break;
           case 's': 
             //TODO
             //serialScanFlash(); 
             break;
           case 'r': 
             //TODO
             //serialReadFlash(); 
             break;
           default:
             break;
        } // end of switch (secondByte)
        int lastByte = Serial.read(); // r
      } // end of if firstByte '('
      else
      {
        if (firstByte == '?')
        {
          ser.answerQuery();
        } 
      }
   }  
}

void showFlashImage(int fileStartAddr) {
   int tAddr = fileStartAddr;
   //byte readBuf[4];
   boolean goodImage = true;
   int RGB_value;
   byte thisByte;
   int numLights;
  
   Serial.println (F("showFlashImage"));    
   command.flash.readFlash(tAddr, 1, readBuf);
   thisByte = readBuf[0];
   while (thisByte != '!') { // advance to actual image start
      if (thisByte == '%') { // update palette
         tAddr = command.flashUpdatePalette(tAddr);
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
      else {
//         Serial.print("+");
         tAddr++;
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }   
   }
   tAddr++; // While escapes at '!' character, need to increment past it
   
   // Now comes 2 bytes for number of lights

   command.flash.readFlash(tAddr, 2, readBuf);
   tAddr += 2; // Need to increment address
   numLights = (readBuf[1] << 8) | readBuf[0];
   // Need to use leds.setPixel since drawingMemory is 8 channels per byte
   // Updating drawing memory directly would be an inefficient use of bandwidth for controllers with < 8 active channels
   for (int i=0; i<numLights; i++) {
      command.flash.readFlash(tAddr, 3, readBuf);

      if(applyGamma == true) {
        RGB_value = exp_gamma[readBuf[0]] | (exp_gamma[readBuf[1]] << 8) | (exp_gamma[readBuf[2]] << 16);
      }
      else {
        RGB_value = readBuf[0] | (readBuf[1] << 8) | (readBuf[2] << 16);
      }
      
      leds.setPixel(i,RGB_value);
      tAddr += 3;
   }
   leds.show();   
}

void showCompImage(int fileStartAddr) {
   int tAddr = fileStartAddr;
   //byte readBuf[4];
   boolean goodImage = true;
   RGB_t color;
   byte thisByte;
   int numLights;
  
   Serial.println(F("showCompImage"));    
   command.flash.readFlash(tAddr, 1, readBuf);
   thisByte = readBuf[0];
   while (thisByte != '@') { // advance to actual image start
      if (thisByte == '%') { // update palette
         tAddr = command.flashUpdatePalette(tAddr);
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
      else {
//         Serial.print("+");
         tAddr++;
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }   
   }
   tAddr++; // While escapes at '@' character, need to increment past it
   
   // Now comes 2 bytes for number of lights

   command.flash.readFlash(tAddr, 2, readBuf);
   tAddr += 2; // Need to increment address
   numLights = (readBuf[1] << 8) | readBuf[0];
   // Need to use leds.setPixel since drawingMemory is 8 channels per byte
   // Updating drawing memory directly would be an inefficient use of bandwidth for controllers with < 8 active channels
   for (int i=0; i<numLights; i++) {
      command.flash.readFlash(tAddr, 1, readBuf);
      color = command.palette.getColor(readBuf[0]);
      leds.setPixel(i,color.r, color.g, color.b);
      tAddr++;
   }
   leds.show();   
}

int playFlashVideo(int startAddr) {
// playFlashVideo uses Timer3 interrupts to control the timing of the video playback.
// Updates to the DMA RAM are made in the main program loop, but the leds.show() function
// is triggerred in the Timer3 ISR - with the exception of the very first frame of video.
// Provision is made for corner cases where faster frame rate is requested than can be
// physically supported.  The intended response to this scenario is to drop frames until
// the player can catch up.  This response was chosen to allow a loss in frame rate while
// still preserving synchronization in time.
// Function returns address of next frame and includes wrap-back to start of file
   int tAddr = startAddr;
   //byte readBuf[4];
   boolean goodImage = true;
   RGB_t color;
   byte thisByte;
   int skipCountCopy;
   int numLights;
   
//   Serial.println ("playFlashVideo");
   flashTransferPending = true;
   command.flash.readFlash(tAddr, 1, readBuf);
   thisByte = readBuf[0];
   while (thisByte != '*') { // advance to actual video start
      if (thisByte == '%') { // update palette
         //Serial.println("u");
         tAddr = command.flashUpdatePalette(tAddr);
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
      else if (thisByte == ')') { // end of file reached, jump back to start
         // This is an ugly use of a global variable, but it works
         tAddr =  command.flash.getCurrentFlashFAT() + 1; // No need to read '(' at start of file
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0]; 
      }
      else {
         //Serial.print("+");
         tAddr++;
         command.flash.readFlash(tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
   }
   tAddr++; // while escapes at '*' character, need to increment past it
   // Now comes 2 bytes for number of lights
   command.flash.readFlash(tAddr, 2, readBuf);
   tAddr += 2; // Need to increment address
   numLights = (readBuf[1] << 8) | readBuf[0];
   //Serial.print("numLights = ");
   //Serial.println(numLights,DEC);
    // Now comes 2 bytes for frame units in microseconds and 2 bytes for holdFrames
   command.flash.readFlash(tAddr, 4, readBuf);
   tAddr += 4; // Need to increment address
   nextFrameUnits = (readBuf[1] << 8) | readBuf[0];
   nextHoldFrames = (readBuf[3] << 8) | readBuf[2];
   noInterrupts();
   skipCountCopy = skipCount;
   interrupts();
   if (skipCountCopy > 0) {
      tAddr = tAddr + numLights;
      skipCount--; //Frame skipped, decrement skipCount
      //Serial.println('-');
   }
   else {
      // Wait until any previous update is complete to update RAM
      while (leds.busy()); // Will have extra 50us of delay beyond DMA transfer time 
      // Now transfer the frame to RAM
      // TODO: Use more efficient multi-byte readFlash
      for (int i=0; i<numLights; i++) {
         command.flash.readFlash(tAddr, 1, readBuf);
         color = command.palette.getColor((uint8_t) readBuf[0]);
         leds.setPixel(i, color.r, color.g, color.b); // Video uses 8-bit palette
         tAddr++;
         //Serial.println('+');
      }
   }
   flashTransferPending = false;

   // Start the Timer interrupt for video playback if necessary
   if (!videoStarted) {
      videoStarted = true;
      frameUnits = nextFrameUnits;
      Timer3.setPeriod(frameUnits);
      Timer3.attachInterrupt(videoISR);
      leds.show(); // show first frame at start of video.  Rest are shown from ISR
   }
   //Serial.println(tAddr, HEX);
   return(tAddr);      
}

// The videoISR runs every time the frame interval timer generates an interrupt.  This could be as often as 100
// times per second in the case of an animated GIF.  
void videoISR(void) {
   holdCount++;
   if (holdCount >= holdFrames) {
      // Reset holdCount and update holdFrames regardless of whether or not frame needs to be skipped
      holdCount = 0;
      holdFrames = nextHoldFrames;
      if ( (flashTransferPending) || leds.busy()) { // check to see that the lights are ready and the new data is ready to show
         skipCount++; // Indicate that the next frame needs to be skipped because we've gotten behind schedule
      }
      else {
         leds.show(); // Trigger DMA transfer of RAM to lights
         //Serial.println('~');
         updateFrameFlag = true;  // Transfer of RAM to lights has started so readout of next frame can begin soon
         // will still need to wait for leds.busy to be cleared before updating RAM
      }
   }
   // Do nothing if there are more holdFrames
}

/*
 COMMENT ONLY FOR ARDUINO
 SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 Serial Event is NOT compatible with Leonardo, Micro, Esplora
 */
/*void serialEvent() {

  while(Serial.available() > 0){
    // get the new byte:
    dummychar = (uint8_t)Serial.read();
    if(!stringComplete)
    {
      if (dummychar == '\n')
      {
        // if the incoming character is a newline, set a flag
        // so the main loop can do something about it
        stringIndex--;
        stringComplete = true;
      }
      else
      {
        if(stringIndex > 19)
        {
          Serial.println("Serial input truncated");
          stringIndex--;
          stringComplete = true;
        }
        else
        {
          // add it to the uart_buffer
          uart_buffer[stringIndex] = dummychar;
          stringIndex++;
        }
      }
    }
  }
}*/
