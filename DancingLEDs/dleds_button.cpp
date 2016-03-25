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

#include "dleds_button.h"

DLEDsButton::DLEDsButton(int numButtonModes){
  this->numButtonModes = numButtonModes;
  buttonIndex = 0;
  solidColorIndex = 0;

  upButtonPressed = false;
  downButtonPressed = false;
  leftButtonPressed = false;
  rightButtonPressed = false;
  
  buttonPressed = bNone;
  
  buttonArmed = false;
  buttonChanged = false;

  buttonByte = 0x00;
  lastButtonByte = 0x00;
  origButtonByte = 0x00;

  buttonCount = 0;
  
}

void DLEDsButton::begin(void) {
  pinMode(UPPIN, INPUT_PULLUP);
  pinMode(DOWNPIN, INPUT_PULLUP);
  pinMode(LEFTPIN, INPUT_PULLUP);
  pinMode(RIGHTPIN, INPUT_PULLUP);
}

boolean DLEDsButton::isButtonChanged(void) {
  return buttonChanged;
}

int DLEDsButton::getButtonIndex(void) {
  return buttonIndex;
}

ButtonPressed_t DLEDsButton::getButtonPressed(void) {
  return buttonPressed;
}

void DLEDsButton::updateButtonStatus(void)
{
  buttonChanged = false; // assume no change unless specific conditions are met
  
  // First figure out what all buttons are currently pressed
  buttonByte = 0;
  if (!digitalRead(UPPIN))
  {
    buttonByte |= upButtonMask;
  }
  if (!digitalRead(DOWNPIN))
  {
    buttonByte |= downButtonMask; 
  }
  if (!digitalRead(LEFTPIN))
  {
    buttonByte |= leftButtonMask; 
  }
  if (!digitalRead(RIGHTPIN))
  {
    buttonByte |= rightButtonMask; 
  }
  
  // Now figure out if the buttons have changed
  if ( buttonByte != 0 && buttonByte != lastButtonByte)
  {
    if (!buttonArmed) // First transition, arm the button change
    {
      buttonArmed = true;
      buttonCount = 0;
      origButtonByte = lastButtonByte;
    }
    else // Button already armed
    {
      if (buttonByte == origButtonByte)
      {
        buttonArmed = false;
      }
      buttonCount = 0; // reset buttonCount if button changes after it's armed
    } 
  }
  else // buttonByte == lastButtonByte
  {
    if (buttonArmed)
    {
      buttonCount++;
      if (buttonCount >= buttonDebounceCount)
      {
        buttonChanged = true;
        buttonArmed = false;
        buttonCount = 0;
      }
    } // end if buttonArmed
  } // end buttonByte == lastButtonByte
 
  lastButtonByte = buttonByte; // need to update lastButtonByte each time through 
  
  if (buttonChanged == true) {
    updateButtonPressed();
  }
}

void DLEDsButton::updateButtonPressed(void) 
{
  
  upButtonPressed = ((buttonByte & upButtonMask) == upButtonMask);
  if (upButtonPressed) {
    Serial.println("Up button");
    buttonPressed = bUp;
    updateButtonIndex();
  }
  else {
    downButtonPressed = ((buttonByte & downButtonMask) == downButtonMask);
    if (downButtonPressed) {
      Serial.println("Down button");      
      buttonPressed = bDown;
      updateButtonIndex();
    }
    else {
      leftButtonPressed = ((buttonByte & leftButtonMask) == leftButtonMask);
      if (leftButtonPressed) {
        Serial.println("Left button");  
        buttonPressed = bLeft;
      } // end if leftButton pressed
      else {
        rightButtonPressed = ((buttonByte & rightButtonMask) == rightButtonMask);
        if (rightButtonPressed) {
          Serial.println("Right button");  
          buttonPressed = bRight;
        } // end if rightButton pressed 
        else {
          //Something went wrong, no buttons are pressed
          buttonPressed = bNone;
        } // end else not rightButton pressed
      } // end else not leftButton pressed
    } // end else not downButton pressed   
  } // end else not upButton pressed
  
} // end updateButtonPressed

//Button Index controls the number of categories that the Up/Down buttons cycle through
void DLEDsButton::updateButtonIndex(void) 
{
  
  switch(buttonPressed) {
  case bUp:
    if (buttonIndex < (numButtonModes - 1)) {
      buttonIndex++; 
    }
    else {
      buttonIndex = 0; 
    }
    break;
  case bDown:
    if (buttonIndex > 0) {
      buttonIndex--; 
    }
    else {
      buttonIndex = numButtonModes - 1; 
    }
    break;
  default:
    return;
    
  }
  
} // end updateButtonIndex

