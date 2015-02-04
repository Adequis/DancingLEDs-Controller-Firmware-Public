

/* Copyright (c) 2014, Dancing LEDs, L.L.C.
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


/** @defgroup ble_uart_project_template ble_uart_project_template
@{
@ingroup projects
@brief Empty project that can be used as a template for new projects.

@details
This project is a firmware template for new projects.
The project will run correctly in its current state.
It can send data on the UART TX characteristic
It can receive data on the UART RX characteristic.
With this project you have a starting point for adding your own application functionality.

The following instructions describe the steps to be made on the Windows PC:

  -# Install the Master Control Panel on your computer. Connect the Master Emulator
    (nRF2739) and make sure the hardware drivers are installed.

  -# You can use the nRF UART app in the Apple iOS app store and Google Play for Android 4.3 for Samsung Galaxy S4
    with this UART template app

  -# You can send data from the Arduino serial monitor, maximum length of a string is 19 bytes
    Set the line ending to "Newline" in the Serial monitor (The newline is also sent over the air

 *
 * Click on the "Serial Monitor" button on the Arduino IDE to reset the Arduino and start the application.
 * The setup() function is called first and is called only once for each reset of the Arduino.
 * The loop() function as the name implies is called in a loop.
 *
 * The setup() and loop() function are called in this way.
 * main()
 *  {
 *   setup();
 *   while(1)
 *   {
 *     loop();
 *   }
 * }
 *
   Required Connections
  --------------------
    pin 2:  LED Strip #1    OctoWS2811 drives 8 LED Strips.
    pin 14: LED strip #2    All 8 are the same length.
    pin 7:  LED strip #3
    pin 8:  LED strip #4    A 100 ohm resistor should used
    pin 6:  LED strip #5    between each Teensy pin and the
    pin 20: LED strip #6    wire to the LED strip, to minimize
    pin 21: LED strip #7    high frequency ringining & noise.
    pin 5:  LED strip #8
    pin 15 & 16 - Connect together, but do not use
    pin 4 - Do not use
    pin 3 - Do not use as PWM.  Normal use is ok.
    pin 9: RDY_N pin of nRF8001
    pin 10: REQ_N pin of nRF8001
    pin 11: MOSI pin of nRF8001 and Flash memory
    pin 12: MISO pin of nRF8001 and Flash memory
    pin 13: SCK pin of nRF8001 and Flash memory
    pin 17: RESET pin of nRF8001
    pin 22: CS_N pin of Flash memory
    pin 31: ACT pin of nRF8001
    pin 28: UP switch
    pin 30: DOWN switch
    pin 29: LEFT switch
    pin 27: RIGHT switch
 */
 
#include <SPI.h>
#include <OctoWS2811.h>
#include <TimerThree.h>
#include <lib_aci.h>
#include <aci_setup.h>
//#include <Bounce.h>
#include "uart_over_ble.h"
#include "norflash.h"
#include "defaultPalette.h"
#define MEMCS_N 22
#define UPPIN 28
#define DOWNPIN 30
#define LEFTPIN 29
#define RIGHTPIN 27
#define ACTIVE 31

#define MAXLEDSPERCHANNEL 500
#define MAXCHANNELS 8

const int ledsPerChannel = MAXLEDSPERCHANNEL; // initialize to max
int ledsTotal = ledsPerChannel * MAXCHANNELS; // initialize to max

// Placeholders to get copied VideoDisplay code working
#define LED_WIDTH      50   // number of LEDs horizontally
#define LED_HEIGHT     16   // number of LEDs vertically (must be multiple of 8)
#define LED_LAYOUT     0    // 0 = even rows left->right, 1 = even rows right->left
#define VIDEO_XOFFSET  0
#define VIDEO_YOFFSET  50       // display entire image
#define VIDEO_WIDTH    100
#define VIDEO_HEIGHT   100

byte lightMode;
boolean videoStarted = false;
#define LM_OFF 0
#define LM_PLAY_FROM_FILE 1
#define LM_PLAY_FROM_SERIAL 2
#define LM_PLAY_FROM_BLE 3
#define LM_COLOR_LOOP 4
#define LM_COLOR_MARCH 5
#define LM_COLOR_FADE 6
#define LM_PALETTE_FADE 7
#define LM_RANDOM 8

#define NUMBUTTONMODES 19
int buttonIndex = 0;
int solidColorIndex = 0;

boolean upButtonPressed = false;
boolean downButtonPressed = false;
boolean leftButtonPressed = false;
boolean rightButtonPressed = false;

boolean marchStarted = false;
boolean marchStartedCopy;
boolean fadeStarted = false;
boolean fadeStartedCopy;


byte currentFileType;
#define FILE_TYPE_NONE 0
#define FILE_TYPE_IMAGE 1
#define FILE_TYPE_COMP_IMAGE 2
#define FILE_TYPE_VIDEO 4
#define FILE_TYPE_CORRUPT 255

#define MAX_FRAME_INTERVAL 65535
//#define SIXTEENTH_SECOND 62500
#define THIRTIETH_SECOND 33333

#define MIN_LIGHT_SPEED 8
#define DEFAULT_LIGHT_SPEED 20
#define MAX_LIGHT_SPEED 40
#define LIGHT_SPEED_INCREMENT 4

#define MIN_LIGHT_DWELL 12
#define DEFAULT_LIGHT_DWELL 24
#define MAX_LIGHT_DWELL 44
#define LIGHT_DWELL_INCREMENT 4

#define MIN_MARCH_HOLD 6
#define DEFAULT_MARCH_HOLD 30
#define MAX_MARCH_HOLD 70
#define MARCH_HOLD_INCREMENT 8


#define FLASH_MEMORY_SIZE 4194304 // 4Mbyte total Flash size
#define FLASH_RSVD_SIZE 524288 // 512Kbyte reserved
#define FLASH_USEABLE_SIZE (FLASH_MEMORY_SIZE - FLASH_RSVD_SIZE)
#define MAX_FLASH_FILES 256
#define FLASH_PAGE_SIZE 256

int flashFAT[MAX_FLASH_FILES];
int flashFiles; // Number of files in flash
int currentFlashFile=0;
int flashWriteAddress; // where next flash write will start
//char flashPageBuf[256];
byte flashPageBuf[256];
int videoAddr;

// display and drawing memory are 24 bytes (6 ints) per channel
DMAMEM int displayMemory[ledsPerChannel*6];
int drawingMemory[ledsPerChannel*6];

// Next variables are shared with Timer3 interrupt and must be volatile to avoid dangerous compiler optimizations
volatile int frameUnits = MAX_FRAME_INTERVAL;
volatile int nextFrameUnits;
volatile int holdFrames = 1;
volatile int nextHoldFrames;
volatile int holdCount = 0;
volatile boolean flashTransferPending = false;
volatile boolean updateFrameFlag = false;
volatile int skipCount = 0;
volatile int lightSpeed = DEFAULT_LIGHT_SPEED;
volatile int lightDwell = DEFAULT_LIGHT_DWELL;
volatile int marchHold = DEFAULT_MARCH_HOLD;
volatile int marchIndex = 0;
volatile boolean marchReverse = false;

volatile int dwellCount = 0;
volatile int fadeCount = 0;
volatile int dwellDelay;
volatile int fadeDelay;

const int config = WS2811_RGB | WS2811_800kHz;

// temp variables that get used often
int count;

int numLights;
byte scratch[20];
byte readBuf[4];


SPISettings norFlash_SPISettings(32000000, MSBFIRST, SPI_MODE0);

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
*/


#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

/* Store the setup for the nRF8001 in the flash of the AVR to save on RAM */
static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

// aci_struct that will contain
// total initial credits
// current credit
// current state of the aci (setup/standby/active/sleep)
// open remote pipe pending
// close remote pipe pending
// Current pipe available bitmap
// Current pipe closed bitmap
// Current connection interval, slave latency and link supervision timeout
// Current State of the the GATT client (Service Discovery)
// Status of the bond (R) Peer address
static struct aci_state_t aci_state;

/*
Temporary buffers for sending ACI commands
*/
static hal_aci_evt_t  aci_data;
//static hal_aci_data_t aci_cmd;

/*
Timing change state variable
*/
static bool timing_change_done          = false;

/*
Used to test the UART TX characteristic notification
*/
static uart_over_ble_t uart_over_ble;
static uint8_t         uart_buffer[20];
static uint8_t         uart_buffer_len = 0;
static uint8_t         dummychar = 0;

/*
Initialize the radio_ack. This is the ack received for every transmitted packet.
*/
//static bool radio_ack_pending = false;




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
/*
Description:

In this template we are using the BTLE as a UART and can send and receive packets.
The maximum size of a packet is 20 bytes.
When a command it received a response(s) are transmitted back.
Since the response is done using a Notification the peer must have opened it(subscribed to it) before any packet is transmitted.
The pipe for the UART_TX becomes available once the peer opens it.
See section 20.4.1 -> Opening a Transmit pipe
In the master control panel, clicking Enable Services will open all the pipes on the nRF8001.

The ACI Evt Data Credit provides the radio level ack of a transmitted packet.
*/
int startColorIdx = 0;
int palette[256];
int numColors;

OctoWS2811 leds(ledsPerChannel, displayMemory, drawingMemory, config);


boolean buttonArmed = false;
boolean buttonChange = false;

byte buttonByte = 0x00;
byte lastButtonByte = 0x00;
byte origButtonByte = 0x00;

int buttonCount = 0;
const int buttonDebounceCount = 1;

const byte leftButtonMask = 0x08;
const byte upButtonMask = 0x04;
const byte downButtonMask = 0x02;
const byte rightButtonMask = 01;

void setup(void)
{
  pinMode(MEMCS_N, OUTPUT);
  pinMode(UPPIN, INPUT_PULLUP);
  pinMode(DOWNPIN, INPUT_PULLUP);
  pinMode(LEFTPIN, INPUT_PULLUP);
  pinMode(RIGHTPIN, INPUT_PULLUP);
  digitalWrite(MEMCS_N, HIGH);   // JRE 10/19/14 hold the memory CS_N high to avoid bus contention
  pinMode(ACTIVE, INPUT);
  memset(palette, 0, 256);
  Serial.setTimeout(200); // 50 was too short for reliable transfer via Processing
  Serial.begin(115200);
  delay(1000);  //DEBUG: 1 second delay to see the start up comments on the serial board
  leds.begin();
  leds.show();
  //Wait until the serial port is available (useful only for the Leonardo)
  //As the Leonardo board is not reseted every time you open the Serial Monitor
  #if defined (__AVR_ATmega32U4__)
    while(!Serial)
    {}
    delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
  #elif defined(__PIC32MX__)
    delay(1000);
  #endif

  Serial.println(F("Arduino setup"));
  Serial.println(F("Set line ending to newline to send data from the serial monitor"));

  /**
  Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
  */
  if (NULL != services_pipe_type_mapping)
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
  }
  else
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
  aci_state.aci_setup_info.setup_msgs         = setup_msgs;
  aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;

  /*
  Tell the ACI library, the MCU to nRF8001 pin connections.
  The Active pin is optional and can be marked UNUSED
  */
  aci_state.aci_pins.board_name = BOARD_DEFAULT; //See board.h for details REDBEARLAB_SHIELD_V1_1 or BOARD_DEFAULT
//  aci_state.aci_pins.reqn_pin   = 9; //SS for Nordic board, 9 for REDBEARLAB_SHIELD_V1_1
  aci_state.aci_pins.reqn_pin   = 10; //JRE 10/19/14
//  aci_state.aci_pins.rdyn_pin   = 8; //3 for Nordic board, 8 for REDBEARLAB_SHIELD_V1_1
  aci_state.aci_pins.rdyn_pin   = 9; //JRE 10/19/14
  aci_state.aci_pins.mosi_pin   = MOSI;
  aci_state.aci_pins.miso_pin   = MISO;
  aci_state.aci_pins.sck_pin    = SCK;

  aci_state.aci_pins.spi_clock_divider      = SPI_CLOCK_DIV8;//SPI_CLOCK_DIV8  = 2MHz SPI speed
                                                             //SPI_CLOCK_DIV16 = 1MHz SPI speed
  
//  aci_state.aci_pins.reset_pin              = 4; //4 for Nordic board, UNUSED for REDBEARLAB_SHIELD_V1_1
  aci_state.aci_pins.reset_pin              = 17; //JRE 10/19/14
  aci_state.aci_pins.active_pin             = UNUSED;
  aci_state.aci_pins.optional_chip_sel_pin  = UNUSED;

  aci_state.aci_pins.interface_is_interrupt = false; //Interrupts still not available in Chipkit
  aci_state.aci_pins.interrupt_number       = 1;

  //We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
  //If the RESET line is not available we call the ACI Radio Reset to soft reset the nRF8001
  //then we initialize the data structures required to setup the nRF8001
  //The second parameter is for turning debug printing on for the ACI Commands and Events so they be printed on the Serial
  lib_aci_init(&aci_state, false);
  Serial.println(F("Set up done"));
  
  scanFlash(flashFAT);
  
  Timer3.initialize(MAX_FRAME_INTERVAL);
}

void uart_over_ble_init(void)
{
  uart_over_ble.uart_rts_local = true;
}

bool uart_tx(uint8_t *buffer, uint8_t buffer_len)
{
  bool status = false;

  if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) &&
      (aci_state.data_credit_available >= 1))
  {
    status = lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, buffer, buffer_len);
    if (status)
    {
      aci_state.data_credit_available--;
    }
  }

  return status;
}

bool uart_process_control_point_rx(uint8_t *byte, uint8_t length)
{
  bool status = false;
  aci_ll_conn_params_t *conn_params;

  if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_CONTROL_POINT_TX) )
  {
    Serial.println(*byte, HEX);
    switch(*byte)
    {
      /*
      Queues a ACI Disconnect to the nRF8001 when this packet is received.
      May cause some of the UART packets being sent to be dropped
      */
      case UART_OVER_BLE_DISCONNECT:
        /*
        Parameters:
        None
        */
        lib_aci_disconnect(&aci_state, ACI_REASON_TERMINATE);
        status = true;
        break;


      /*
      Queues an ACI Change Timing to the nRF8001
      */
      case UART_OVER_BLE_LINK_TIMING_REQ:
        /*
        Parameters:
        Connection interval min: 2 bytes
        Connection interval max: 2 bytes
        Slave latency:           2 bytes
        Timeout:                 2 bytes
        Same format as Peripheral Preferred Connection Parameters (See nRFgo studio -> nRF8001 Configuration -> GAP Settings
        Refer to the ACI Change Timing Request in the nRF8001 Product Specifications
        */
        conn_params = (aci_ll_conn_params_t *)(byte+1);
        lib_aci_change_timing( conn_params->min_conn_interval,
                                conn_params->max_conn_interval,
                                conn_params->slave_latency,
                                conn_params->timeout_mult);
        status = true;
        break;

      /*
      Clears the RTS of the UART over BLE
      */
      case UART_OVER_BLE_TRANSMIT_STOP:
        /*
        Parameters:
        None
        */
        uart_over_ble.uart_rts_local = false;
        status = true;
        break;


      /*
      Set the RTS of the UART over BLE
      */
      case UART_OVER_BLE_TRANSMIT_OK:
        /*
        Parameters:
        None
        */
        uart_over_ble.uart_rts_local = true;
        status = true;
        break;
    }
  }

  return status;
}

void serialGetImage()
{
      boolean goodImage = true;
      int RGB_value;
      char rgb[3];
      int firstByte = Serial.read();
      int secondByte = Serial.read();
      numLights = (secondByte << 8) | firstByte;
      // Need to use leds.setPixel since drawingMemory is 8 channels per byte
      for (int i=0; i<numLights; i++) {
         count = Serial.readBytes(rgb, 3);
         if (count == 3) {
            RGB_value = rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
            leds.setPixel(i,RGB_value);
         }
         else {
            goodImage = false;
         }  
      }
      if (goodImage) {
         leds.show();   
      }
}

void serialGetFrame()
{
// TODO 
}

void serialGetCompImage()
{
// TODO  
}

void serialGetPartFrame ()
{
// TODO  
}

void serialGetHoldTime ()
{
// TODO  
}

void serialGetPalette ()
{
// TODO  
}

//void serialFlashFileOpen () {
// TODO  
//}

void serialFlashPageWrite () {
   int bytesOnPage = Serial.read() + 1; // 0-255 maps into 1-256 bytes
   count = Serial.readBytes((char *)flashPageBuf, bytesOnPage);
   if (count == bytesOnPage) {
      while (bytesOnPage < FLASH_PAGE_SIZE) {
         flashPageBuf[bytesOnPage] = '.';  // Used to pad last page in file
         bytesOnPage++;   
      }
 
      while (checkFlashStatus() & 0x01) {
         delay(1);
      } // Wait for Flash to be ready for a write        
      writeFlash( flashWriteAddress, FLASH_PAGE_SIZE, flashPageBuf);
      //flashFAT[flashFiles] = flashWriteAddress;
      //flashFiles++;
      flashWriteAddress += FLASH_PAGE_SIZE; // always write full page
      Serial.write(':'); // Acknowledge page received
    }
   else {
      if (count > bytesOnPage) {
         Serial.write('>'); // Too many bytes
      }
      else {
         Serial.write('<'); // Too few bytes
      }
   }  
}

/*
void serialEraseFlash() {
     byte flashStatus;
     flashStatus = checkFlashStatus();
     if (flashStatus & 0x01) { 
        eraseFlashChip();
        int delayCount=0;
        boolean eraseTimeout = false;

        Serial.write('.');
        while ((checkFlashStatus() & 0x01) && !eraseTimeout) {
           Serial.write('.');
           delay(1000);
           delayCount++;
           if (delayCount > 60) {
              eraseTimeout = true;
           } 
        }
        if (eraseTimeout) {
           Serial.println(F("Error: Flash Erase Timeout"));
        }
        else {
           Serial.print(F("Flash erase complete in ~"));
           Serial.print(delayCount);
           Serial.println(F(" seconds"));
           //Here
        }
     } // end if checkFlashStatus...
     else {
        Serial.print("Error: Flash status = ");
        Serial.print(flashStatus);
        Serial.println(" prior to erase");     
     }
//     Serial.flush(); // Make sure all serial data transmits before leaving function  
}
*/
//void serialFlashFileClose () {
// TODO  
//}

void serialAnswerQuery() {
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

void light_loop() {
   int fadeCountCopy;
   int mult;
   int percent;
   boolean updateFrameFlagCopy;


   
   switch (lightMode) {
      case LM_PLAY_FROM_FILE: {
         if (currentFileType == FILE_TYPE_VIDEO) {
            noInterrupts();
            updateFrameFlagCopy = updateFrameFlag;
            interrupts();
            if (videoStarted && updateFrameFlagCopy) {
               videoAddr = playFlashVideo(videoAddr);
               updateFrameFlag = false;
            }

            // Do nothing if it's not time for a frame update
         }
         else {
            updateFrameFlag = false;
            videoStarted = false;
            //Timer3.stop();
            //Timer3.detachInterrupt();  
         }
      }
      break;
      case LM_PLAY_FROM_SERIAL: {
         // TODO       
      }
      break;
      case LM_PLAY_FROM_BLE: {
         // TODO      
      }
      break;
      case LM_COLOR_MARCH: {
        noInterrupts();
        marchStartedCopy = marchStarted;
        interrupts();
        if (marchStartedCopy) {
          Serial.println("M");
          marchColors();
          marchStarted = false;
        }
        
      }
      break;
      case LM_COLOR_FADE: {
        noInterrupts();
        fadeStartedCopy = fadeStarted;
        fadeCountCopy = fadeCount;
        marchStartedCopy = marchStarted;
        interrupts();
        if(fadeStartedCopy) {
          percent = (int) 99*fadeCountCopy/fadeDelay + 1;
          Serial.print("F! %: ");
          Serial.println(percent);
          fadeColors(percent);
          fadeStarted = false;
        }
        else if (marchStartedCopy) {
          Serial.println("M2");
          if (marchIndex == 0) {
            marchIndex = numColors - 1;
          }
          else {
            marchIndex--;
          }  
          marchStarted = false;
        }
        
      }
      break;
      case LM_PALETTE_FADE: {
        noInterrupts();
        fadeStartedCopy = fadeStarted;
        fadeCountCopy = fadeCount;
        marchStartedCopy = marchStarted;
        interrupts();
        if(fadeStartedCopy) {
          percent = (int) 99*fadeCountCopy/fadeDelay + 1;
          Serial.print("F2! %: ");
          Serial.println(percent);
          fadePalette(percent);
          fadeStarted = false;
        }
        else if (marchStartedCopy) {
          Serial.print("M3 Idx: ");
          Serial.println(marchIndex);
          marchIndex++;
          if (marchIndex >= numColors) {
            marchIndex = 0; 
          }
          marchStarted = false;
        }
        
      }
      break;
      case LM_RANDOM: {
        
      }
   }
  
}

void serial_loop()
{

   if (Serial.available() > 0) {
      // read the incoming byte:
      int firstByte = Serial.read();
      if (firstByte == '(') // start of image line
      {
        if (lightMode != LM_OFF) { // turnoff patterns if any in progress
          palette[0] = 0;
          colorGroup (palette, 0, 1);
          restartPattern();
          lightMode = LM_OFF;  
        }
        int secondByte = Serial.read();
        switch (secondByte) { 
           case '!': serialGetImage(); break;        
           case '*': serialGetFrame(); break;
           case '@': serialGetCompImage(); break;
           case '&': serialGetPartFrame(); break;
           case '^': serialGetHoldTime(); break; 
           case '%': serialGetPalette(); break;
 //        case '(': serialFlashFileOpen(); break;
           case '$': serialFlashPageWrite(); break;
//         case ')': serialFlashFileClose(); break;
//           case '?': serialAnswerQuery(); break;
//         case 'x': serialEraseFlash(); break;
//         case 's': serialScanFlash(); break;
//         case 'r': serialReadFlash(); break;
        } // end of switch (secondByte)
        int lastByte = Serial.read(); // r
      } // end of if firstByte '('
      else
      {
        if (firstByte == '?')
        {
          serialAnswerQuery();
        } 
      }
   }  
}

void button_debounce()
{
  buttonChange = 0; // assume no change unless specific conditions are met
  
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
  
  // Now figure out it the buttons have changed
  if (buttonByte != lastButtonByte)
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
        buttonChange = true;
        buttonArmed = false;
        buttonCount = 0;
      }
    } // end if buttonArmed
  } // end buttonByte == lastButtonByte
 
  
  lastButtonByte = buttonByte; // need to update lastButtonByte each time through   
}

void restartPattern()
{
  fadeDelay = lightSpeed;
  dwellDelay = lightDwell;
  dwellCount = 0;
  fadeCount = 0;        
  marchStarted = false;
  fadeStarted = false;
}

void changeButtonIndex(int newButtonIndex)
{
  Timer3.detachInterrupt();
  // Individual case statements used because index of pointers was causing weird behavior even when
  // function wasn't being called
  switch (newButtonIndex) {
    case 0: 
      {
        palette[0] = 0;
        colorGroup (palette, 0, 1);
        restartPattern();
        lightMode = LM_OFF;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);      
      }
      break;
    case 1:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, ChristmasColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();     
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);    
      }    
      break;
    case 2:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, ChristmasColors, numColors*4);
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);           
      }    
      break;
    case 3:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, ValentinesColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();      
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);           
      }    
      break;
    case 4:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, ValentinesColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();     
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }    
      break;
    case 5:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, StPatricksColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);           
      }    
      break;
    case 6:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, StPatricksColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();     
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;
    case 7:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, EasterColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);           
      }
      break;
    case 8:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, EasterColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;
    case 9:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, July4thColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);           
      }
      break;
    case 10:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, July4thColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();      
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;
    case 11:
      {
        numColors = 3;
        marchIndex = 0;
        memcpy (palette, HalloweenColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);           
      }
      break;
    case 12:
      {
        numColors = 3;
        marchIndex = 0;
        memcpy (palette, HalloweenColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;
    case 13:
      {
        numColors = 4;
        marchIndex = 0;
        memcpy (palette, ThanksgivingColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();      
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);           
      }
      break;
    case 14:
      {
        numColors = 4;
        marchIndex = 0;
        memcpy (palette, ThanksgivingColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();      
        lightMode = LM_COLOR_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;
    case 15:
      {
        palette[0] = solidColorArray[solidColorIndex];
        colorGroup (palette, 0, 1); 
        restartPattern();       
        lightMode = LM_COLOR_LOOP;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);          
      }
      break;      
    case 16:
      {
        numColors = 256;
        marchIndex = 0;
        memcpy (palette, rainbowPalette, numColors*4);
        colorGroup (palette, marchIndex, numColors);
        restartPattern();      
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);            
      }
      break;
    case 17:
      {
        numColors = 8;
        marchIndex = 0;
        memcpy (palette, solidColorArray, numColors*4);
        colorGroup (palette, marchIndex, numColors);
        restartPattern();       
        lightMode = LM_PALETTE_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;
    case 18:
      {
        numColors = 256;
        marchIndex = 0;
        memcpy (palette, rainbowPalette, numColors*4);
        colorGroup (palette, marchIndex, numColors);
        restartPattern();      
        lightMode = LM_PALETTE_FADE;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(fadeIRC);            
      }
      break;

    default:
      {
        numColors = 5;
        marchIndex = 0;
        memcpy (palette, ChristmasColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
        colorGroup (palette, marchIndex, numColors);
        restartPattern();     
        lightMode = LM_COLOR_MARCH;
        Timer3.setPeriod(THIRTIETH_SECOND);
        Timer3.attachInterrupt(marchIRC);            
      }
      break;
  }  
}

void button_change() 
{
  
  upButtonPressed = ((buttonByte & upButtonMask) == upButtonMask);
  if (upButtonPressed) {
    if (buttonIndex < (NUMBUTTONMODES - 1)) {
      buttonIndex++; 
    }
    else {
      buttonIndex = 0; 
    }
    
    // TODO: Create function to duplicate what is done in downbutton
    changeButtonIndex(buttonIndex);
    Serial.println("Up button");
      
  }
  else {
    downButtonPressed = ((buttonByte & downButtonMask) == downButtonMask);
    if (downButtonPressed) {
      // Down button change  
      // Down button pressed
      if (buttonIndex > 0) {
        buttonIndex--; 
      }
      else {
        buttonIndex = NUMBUTTONMODES-1; 
      }
      
      // TODO: Create function to duplicate what is done in upButton
      changeButtonIndex(buttonIndex);
      Serial.println("Down button");      

    }
    else {
      leftButtonPressed = ((buttonByte & leftButtonMask) == leftButtonMask);
      if (leftButtonPressed) {
        Serial.println("Left button");  
        // Left button change  
        // Left button pressed, slow down fade/march display
        // TODO: Move into function to share with '<'


        //Serial.print("lightDwell = ");
        //Serial.println(lightDwell);
        if (lightMode == LM_COLOR_LOOP) {
          if (solidColorIndex > 0) {
            solidColorIndex--;
          }
          else {
            solidColorIndex = (SOLIDCOLORARRAYSIZE - 1);
          }
          palette[0] = solidColorArray[solidColorIndex];
          colorGroup (palette, 0, 1);  
        }
        else // this moved to else so timing not updated for solid color
        {
          lightDwell = lightDwell + LIGHT_DWELL_INCREMENT;
          lightSpeed = lightSpeed + LIGHT_SPEED_INCREMENT;
          marchHold = marchHold + MARCH_HOLD_INCREMENT;
          if (lightDwell > MAX_LIGHT_DWELL) {
            lightDwell = MIN_LIGHT_DWELL; // wrap around
          }
          if (lightSpeed > MAX_LIGHT_SPEED) {
            lightSpeed = MIN_LIGHT_SPEED; // wrap around
          }
          if (marchHold > MAX_MARCH_HOLD) {
            marchHold = MIN_MARCH_HOLD;  // wrap around
          }          
        }
        restartPattern();
      } // end if leftButton pressed
      else {
        rightButtonPressed = ((buttonByte & rightButtonMask) == rightButtonMask);
        if (rightButtonPressed) {
          Serial.println("Right button");  
          // Right button change
          // Right button pressed, speed up fade/march display
          // TODO: move into function to share with '>'


          //Serial.print("lightDwell = ");
          //Serial.println(lightDwell);
          if (lightMode == LM_COLOR_LOOP) {
            if (solidColorIndex < (SOLIDCOLORARRAYSIZE-1)) {
              solidColorIndex++;
            }
            else {
              solidColorIndex = 0;
            }
            palette[0] = solidColorArray[solidColorIndex];
            colorGroup (palette, 0, 1);  
          }
          else // moved to else to not change timing for solid color updates
          {
            lightDwell = lightDwell - LIGHT_DWELL_INCREMENT;
            lightSpeed = lightSpeed - LIGHT_SPEED_INCREMENT;
            marchHold = marchHold - MARCH_HOLD_INCREMENT;
            if (lightDwell < MIN_LIGHT_DWELL) {
              lightDwell = MAX_LIGHT_DWELL; // wrap around
            }
            if (lightSpeed < MIN_LIGHT_SPEED) {
              lightSpeed = MAX_LIGHT_SPEED; // wrap around
            }
            if (marchHold < MIN_MARCH_HOLD) {
              marchHold = MAX_MARCH_HOLD; // wrap around
            }            
          }
          restartPattern();          
        } // end if rightButton pressed 
      } // end else not leftButton pressed
    } // end else not downButton pressed   
  } // end else not upButton pressed
  
} // end button_change

// ACI and loop variables
bool commandReceived = false;  // whether the command is received
bool commandFinished = false;
uint8_t bufferIndex = 0;       //Initialize the index to store incoming chars
char command_buffer[776];      //"Full" size of command Start + Control + Num + Palette (3*256) + control + Mode + Speed + Dwell + End

void aci_loop()
{
  static bool setup_required = false;

  // We enter the if statement only when there is a ACI event available to be processed
  if (lib_aci_event_get(&aci_state, &aci_data))
  {
    aci_evt_t * aci_evt;
    aci_evt = &aci_data.evt;
    switch(aci_evt->evt_opcode)
    {
      /**
      As soon as you reset the nRF8001 you will get an ACI Device Started Event
      */
      case ACI_EVT_DEVICE_STARTED:
      {
        aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
        switch(aci_evt->params.device_started.device_mode)
        {
          case ACI_DEVICE_SETUP:
            /**
            When the device is in the setup mode
            */
            Serial.println(F("Evt Device Started: Setup"));
            setup_required = true;
            break;

          case ACI_DEVICE_STANDBY:
            Serial.println(F("Evt Device Started: Standby"));
            //Looking for an iPhone by sending radio advertisements
            //When an iPhone connects to us we will get an ACI_EVT_CONNECTED event from the nRF8001
            if (aci_evt->params.device_started.hw_error)
            {
              delay(20); //Handle the HW error event correctly.
            }
            else
            {
              lib_aci_connect(0/* in seconds : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
              Serial.println(F("Advertising started!"));
            }

            break;
        }
      }
      break; //ACI Device Started Event

      case ACI_EVT_CMD_RSP:
        //If an ACI command response event comes with an error -> stop
        if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)
        {
          //ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
          //TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
          //all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
          Serial.print(F("ACI Command "));
          Serial.println(aci_evt->params.cmd_rsp.cmd_opcode, HEX);
          Serial.print(F("Evt Cmd respone: Status "));
          Serial.println(aci_evt->params.cmd_rsp.cmd_status, HEX);
        }
        if (ACI_CMD_GET_DEVICE_VERSION == aci_evt->params.cmd_rsp.cmd_opcode)
        {
          //Store the version and configuration information of the nRF8001 in the Hardware Revision String Characteristic
          lib_aci_set_local_data(&aci_state, PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET,
            (uint8_t *)&(aci_evt->params.cmd_rsp.params.get_device_version), sizeof(aci_evt_cmd_rsp_params_get_device_version_t));
        }
        break;

      case ACI_EVT_CONNECTED:
        Serial.println(F("Evt Connected"));
        uart_over_ble_init();
        timing_change_done              = false;
        aci_state.data_credit_available = aci_state.data_credit_total;

        /*
        Get the device version of the nRF8001 and store it in the Hardware Revision String
        */
        lib_aci_device_version();
        break;

      case ACI_EVT_PIPE_STATUS:
        Serial.println(F("Evt Pipe Status"));
        if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) && (false == timing_change_done))
        {
          lib_aci_change_timing_GAP_PPCP(); // change the timing on the link as specified in the nRFgo studio -> nRF8001 conf. -> GAP.
                                            // Used to increase or decrease bandwidth
          timing_change_done = true;

          //char hello[]="Hello World, works";
          //uart_tx((uint8_t *)&hello[0], strlen(hello));
          //Serial.print(F("Sending :"));
          //Serial.println(hello);
        }
        break;

      case ACI_EVT_TIMING:
        Serial.println(F("Evt link connection interval changed"));
        lib_aci_set_local_data(&aci_state,
                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET,
                                (uint8_t *)&(aci_evt->params.timing.conn_rf_interval), /* Byte aligned */
                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET_MAX_SIZE);
        break;

      case ACI_EVT_DISCONNECTED:
        Serial.println(F("Evt Disconnected/Advertising timed out"));
        lib_aci_connect(0/* in seconds  : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started."));
        break;

      case ACI_EVT_DATA_RECEIVED:
        Serial.print(F("Pipe Number: "));
        Serial.println(aci_evt->params.data_received.rx_data.pipe_number, DEC);

        if (PIPE_UART_OVER_BTLE_UART_RX_RX == aci_evt->params.data_received.rx_data.pipe_number)
          {

            Serial.print(F(" Data(Hex) : "));
            for(int i=0; i<aci_evt->len - 2; i++)
            {
              Serial.print("0x");
              Serial.print(aci_evt->params.data_received.rx_data.aci_data[i],HEX);
              uart_buffer[i] = aci_evt->params.data_received.rx_data.aci_data[i];
              Serial.print(F(" "));
            }
            Serial.println();
            uart_buffer_len = aci_evt->len - 2;
            
            //Tell "loop()" that a command has been received.
            commandReceived = true;
            
        }
        if (PIPE_UART_OVER_BTLE_UART_CONTROL_POINT_RX_1 == aci_evt->params.data_received.rx_data.pipe_number)
        {
          uart_process_control_point_rx(&aci_evt->params.data_received.rx_data.aci_data[0], aci_evt->len - 2); //Subtract for Opcode and Pipe number
        }
        break;

      case ACI_EVT_DATA_CREDIT:
        aci_state.data_credit_available = aci_state.data_credit_available + aci_evt->params.data_credit.credit;
        break;

      case ACI_EVT_PIPE_ERROR:
        //See the appendix in the nRF8001 Product Specication for details on the error codes
        Serial.print(F("ACI Evt Pipe Error: Pipe #:"));
        Serial.print(aci_evt->params.pipe_error.pipe_number, DEC);
        Serial.print(F("  Pipe Error Code: 0x"));
        Serial.println(aci_evt->params.pipe_error.error_code, HEX);

        //Increment the credit available as the data packet was not sent.
        //The pipe error also represents the Attribute protocol Error Response sent from the peer and that should not be counted
        //for the credit.
        if (ACI_STATUS_ERROR_PEER_ATT_ERROR != aci_evt->params.pipe_error.error_code)
        {
          aci_state.data_credit_available++;
        }
        break;

      case ACI_EVT_HW_ERROR:
        Serial.print(F("HW error: "));
        Serial.println(aci_evt->params.hw_error.line_num, DEC);

        for(uint8_t counter = 0; counter <= (aci_evt->len - 3); counter++)
        {
          Serial.write(aci_evt->params.hw_error.file_name[counter]); //uint8_t file_name[20];
        }
        Serial.println();
        lib_aci_connect(0/* in seconds, 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started. Tap Connect on the nRF UART app"));
        break;

    }
  }
  else
  {
    //Serial.println(F("No ACI Events available"));
    // No event in the ACI Event queue and if there is no event in the ACI command queue the arduino can go to sleep
    // Arduino can go to sleep now
    // Wakeup from sleep from the RDYN line
  }

  /* setup_required is set to true when the device starts up and enters setup mode.
   * It indicates that do_aci_setup() should be called. The flag should be cleared if
   * do_aci_setup() returns ACI_STATUS_TRANSACTION_COMPLETE.
   */
  if(setup_required)
  {
    if (SETUP_SUCCESS == do_aci_setup(&aci_state))
    {
      setup_required = false;
    }
  }
}

void loop() {

  //Process any ACI commands or events
  aci_loop();
  // Otherwise use 32MHz transactions to the nor Flash
  serial_loop();
  
  // Interpret any light command from aci_loop or serial_loop
  if (commandReceived) 
  {
    Serial.println(F("Data Received!"));
    //Timer3.stop();
    Timer3.detachInterrupt();
    videoStarted = false; // Need to indicate video is stopped so timer will be restarted
    
    int firstByte = (char) uart_buffer[0];
    int lastByte = (char) uart_buffer[uart_buffer_len - 1];
    
    if(firstByte == '(') {
      Serial.println(F("Start of Command..."));
      bufferIndex = 0;
    }
    
    for (int i=0; i < uart_buffer_len; i++) {
      command_buffer[bufferIndex + i] = uart_buffer[i];
    }
    bufferIndex = bufferIndex + uart_buffer_len;
    
    if(lastByte == ')') {
      Serial.println(F("End of Command."));
      
      commandFinished = true;
      bufferIndex = 0;
    }
    
    // clear the uart_buffer:
    for (int i=0; i < uart_buffer_len; i++)
    {
      uart_buffer[i] = ' ';
    }
    
    commandReceived = false;
  }
  
  if (commandFinished) {
    Serial.println(F("Command Finished! Sending to Lights!"));
    char controlChar = command_buffer[1]; //Ignore the starting '(' character.
      
    switch (controlChar) {
      case '%': lightPaletteCommandInterpreter((char*) command_buffer); break;
      default : lightCommandInterpreter(controlChar); break;
    }
    
    //Values of 40 and 10 are chosen arbitrarily to adjust speed of transition
    //They can be adjusted, but fadeDelay must be a multiple of 20 for integer math to work out.
    //fadeDelay = 40*lightSpeed;
    //dwellDelay = 10*lightDwell;
    fadeDelay = lightSpeed;
    dwellDelay = lightDwell;
    dwellCount = 0;
    fadeCount = 0;
    marchStarted = false;
    fadeStarted = false;
      
    switch (lightMode) {
    case LM_COLOR_MARCH:
      Serial.println("Color March!");
      Timer3.setPeriod(THIRTIETH_SECOND);
      //Timer3.start();
      Timer3.attachInterrupt(marchIRC);
      break;
    case LM_COLOR_FADE:
      Serial.println("Color Fade!");
      Timer3.setPeriod(THIRTIETH_SECOND);
      //Timer3.start();
      Timer3.attachInterrupt(fadeIRC);
      break;
    case LM_PALETTE_FADE:
      Serial.println("Palette Fade!");
      Timer3.setPeriod(THIRTIETH_SECOND);      
      //Timer3.start();
      Timer3.attachInterrupt(fadeIRC);   
      break;
    case LM_COLOR_LOOP:
      Serial.println("Color Loop!");
      break;
    default:
      Serial.println("Stop!");
    break;
    }
    commandFinished = false;
  }
  
  button_debounce();
  if(buttonChange) 
  {
    button_change(); 
  }
  
  light_loop();
  
  //For ChipKit you have to call the function that reads from Serial
  #if defined (__PIC32MX__)
    if (Serial.available())
    {
      serialEvent();
    }
  #endif
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

void lightCommandInterpreter (char lightCommand)
{

   int delayCycles=0;
// TODO:
// 4) Need to add mechanism to write to Flash over BLE
// 5) Security needs to be considered to prevent light-jacking
   switch (lightCommand) {
    case 'o': //lights off
      palette[0] = 0;
      colorGroup (palette, 0, 1);
      lightMode = LM_OFF;
      break;
    case 'c':  // lights on Christmas colors
      numColors = 5;
      marchIndex = 0;
      memcpy (palette, ChristmasColors, numColors*4); // need 4x to copy ints.  memcpy is in bytes
      colorGroup (palette, marchIndex, numColors);
      lightMode = LM_COLOR_MARCH;
      break;
   case 'd': // default color palette
     numColors = 256;
     marchIndex = 0;
     memcpy (palette, rainbowPalette, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_PALETTE_FADE;
     break;
   case 'D': // Christmas color palette fade
     numColors = 5;
     marchIndex = 0;
     memcpy (palette, ChristmasColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_FADE;
     break;
   case 'v': // Valentines
     numColors = 5;
     marchIndex = 0;
     memcpy (palette, ValentinesColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_MARCH;
     break;
   case 'p': // St Patricks
     numColors = 5;
     marchIndex = 0;
     memcpy (palette, StPatricksColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_MARCH;
     break;
   case 'j': // July 4th colors
     numColors = 5;
     marchIndex = 0;
     memcpy (palette, July4thColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_MARCH;
     break;
   case 'h': // Halloween colors
     numColors = 3;
     marchIndex = 0;
     memcpy (palette, HalloweenColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_MARCH;
     break;
   case 't': // Thanksgiving colors
     numColors = 4;
     marchIndex = 0;
     memcpy (palette, ThanksgivingColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_MARCH;
     break;
   case 'e': // Easter colors
     numColors = 5;
     marchIndex = 0;
     memcpy (palette, EasterColors, numColors*4);
     colorGroup (palette, marchIndex, numColors);
     lightMode = LM_COLOR_MARCH;
     break;
   case '<': //Slow down color march/fade
     lightDwell = lightDwell + LIGHT_DWELL_INCREMENT;
     lightSpeed = lightSpeed + LIGHT_SPEED_INCREMENT;
     marchHold = marchHold + MARCH_HOLD_INCREMENT;
     if (lightDwell > MAX_LIGHT_DWELL) {
       lightDwell = MIN_LIGHT_DWELL; // wrap around
     }
     if (lightSpeed > MAX_LIGHT_SPEED) {
       lightSpeed = MIN_LIGHT_SPEED; // wrap around
     }
     if (marchHold > MAX_MARCH_HOLD) {
       marchHold = MIN_MARCH_HOLD;  // wrap around
     }
     break;
   case '>': //Speed up color march/fade
     lightDwell = lightDwell - LIGHT_DWELL_INCREMENT;
     lightSpeed = lightSpeed - LIGHT_SPEED_INCREMENT;
     marchHold = marchHold - MARCH_HOLD_INCREMENT;
     if (lightDwell < MIN_LIGHT_DWELL) {
       lightDwell = MAX_LIGHT_DWELL; // wrap around
     }
     if (lightSpeed < MIN_LIGHT_SPEED) {
       lightSpeed = MAX_LIGHT_SPEED; // wrap around
     }
     if (marchHold < MIN_MARCH_HOLD) {
       marchHold = MAX_MARCH_HOLD; // wrap around
     }
     break;
   case '=': //Stop color march/fade
     lightMode = LM_COLOR_LOOP;
     break;
   case 'x': // erase Flash - temporary command
     Serial.println("Erasing flash memory");
     eraseFlashChip();
     delayCycles=0;
     while (checkFlashStatus() & 0x01) {
        delay(10);
        delayCycles++;  
     }
     Serial.print("Flash erase complete in ");
     Serial.print(delayCycles, DEC);
     Serial.println(" delay(10) cycles");
     break;
   case 'z': // Erase block - temporary command
     Serial.println("Erasing first block of flash memory");
     eraseFlashBlock(0);
     delayCycles=0;
     while (checkFlashStatus() & 0x01) {
        delay(10);
        delayCycles++;  
     }
     Serial.print("Block erase complete in ");
     Serial.print(delayCycles, DEC);
     Serial.println(" delay(10) cycles");
     break;
   case 'r': 
     for (int tpage=0; tpage < 8; tpage++) {
        readFlash(256*tpage, 256, flashPageBuf);
        for (int i=0; i<16; i++) {
           Serial.print("Page: ");
           Serial.print(tpage, DEC);
           Serial.print(", Addr: ");
           int i16 = i*16;
           if (i16 < 10) {
             Serial.print("  "); 
           }
           else if (i16 < 100) {
             Serial.print(" "); 
           }
           Serial.print(i16, DEC);
           Serial.print(" || ");
           for (int j=0; j<16; j++) {
              byte tempDat = flashPageBuf[16*i + j];
              Serial.print("0x");
              if(tempDat < 0x10) {
                 Serial.print("0");
              }
              Serial.print(tempDat, HEX);
              Serial.print("  ");
           }
           Serial.println();
        }
     }
     break;
   case 's':
     scanFlash(flashFAT);  
     break;
   case '+': // Move to next image or sequence file and play
     if (lightMode == LM_PLAY_FROM_FILE) {
       currentFlashFile++;
       if (currentFlashFile == flashFiles) {
          currentFlashFile = 0;  
       }   
       playLightsFromFile(flashFAT[currentFlashFile]);
     }
     else {
        lightMode = LM_PLAY_FROM_FILE;
        playLightsFromFile(flashFAT[currentFlashFile]);  
     }
     break;
   case '-':
     if (lightMode == LM_PLAY_FROM_FILE) {
       if (currentFlashFile != 0) {
          currentFlashFile--;
       }
       else {
          currentFlashFile = flashFiles - 1;  
       }
       playLightsFromFile(flashFAT[currentFlashFile]);
     }
     else {
        lightMode = LM_PLAY_FROM_FILE;
        playLightsFromFile(flashFAT[currentFlashFile]);  
     }
     break;
   default:
     numColors = 216;
     memcpy (palette, defaultPalette, numColors*4);
     colorGroup (palette, 0, numColors);
     lightMode = LM_COLOR_LOOP;
     break;
  }
  
  //If new palette command, reset speed and dwell to defaults
  // 12/22/14 JRE - prefer to leave speed at previous setting
  //if (lightCommand != '<' && lightCommand != '>') {
    //Reset to default speed and dwell
  //  lightDwell = DEFAULT_LIGHT_DWELL;
  //  lightSpeed = DEFAULT_LIGHT_SPEED;
  //}
  
  if (lightCommand == 'o') {
    //Send a command received confirmation back to the client
    uint8_t reply = (uint8_t) 'o';
    uint8_t reply_len = 1;
    
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, &reply, reply_len))
    {
      Serial.println(F("Command input dropped"));
    }
  }
  else {
    //Send a command received confirmation back to the client
    uint8_t reply = (uint8_t) '!';
    uint8_t reply_len = 1;
   
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, &reply, reply_len))
    {
      Serial.println(F("Command input dropped"));
    }
  }
}

// lightPaletteCommandInterpreter GSA 121314
void lightPaletteCommandInterpreter (char* lightCommand)
{
  int red;
  int green;
  int blue;
  int idx = 0;
  
  uint8_t reply;
  uint8_t reply_len;
  
  //Check correct command format (% for color palette)
  int firstByte = (char) lightCommand[idx++];
  if (firstByte != '(')
  {
    Serial.println("Starting character not correct!");
    return;
  }

  char command = (char)lightCommand[idx++];
  if (command != '%')
  {
    Serial.println("Palette command not correct!");
    return;
  }

  //Check correct number of colors (greater than 0)
  numColors = (int)lightCommand[idx++];
  if (numColors < 1)
  {
    Serial.println("numColors not correct!");
    return;
  }

  int hexColor[numColors];
  Serial.print("Number of Colors = ");
  Serial.println(numColors,DEC);
  
  for (int i=0; i<numColors; i++)
  {
    command = (char)lightCommand[idx];
    if (command == '#')
    {
      idx++;
      Serial.println("Skipped: #");
    }
    
    red = (int)lightCommand[idx++];
    green = (int)lightCommand[idx++];
    blue = (int)lightCommand[idx++];
    hexColor[i] = (red << 16) | (green << 8) | blue;
  }
  memcpy (palette, hexColor, numColors*4); // need 4x to copy ints.  memcpy is in bytes
  
  command = (char)lightCommand[idx++];
  switch (command) {
    case '~': 
      break;
    case ')':
      //Command finished without a lightMode, i.e. single color 
      lightMode = LM_COLOR_LOOP;
      
      //Send a command received confirmation back to the client
      reply = (uint8_t) '!';
      reply_len = 1;
   
      if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, &reply, reply_len))
      {
        Serial.println(F("Command input dropped"));
      }
      
      //Send the color palette to the lights
      colorGroup (palette, 0, numColors);
      return;
    default:
      Serial.println("Command invalid!");
      lightMode = LM_COLOR_LOOP;
      return;
  }
  
  command = (char)lightCommand[idx++];
  
  switch (command) {
    case 'l' :
      Serial.println("Loop!");
      lightMode = LM_COLOR_LOOP;
      lightSpeed = DEFAULT_LIGHT_SPEED;
      lightDwell = DEFAULT_LIGHT_DWELL;
      command = (char)lightCommand[idx++];
      break;
    case 'm' :
      Serial.println("March!");
      lightMode = LM_COLOR_MARCH;
      idx++;
      marchHold = (int) lightCommand[idx++];     
      command = (char)lightCommand[idx++];
      break;
    case 'f' :
      Serial.println("Fade!");
      lightMode = LM_COLOR_FADE;
      lightSpeed = (int) lightCommand[idx++];
      lightDwell = (int) lightCommand[idx++];
      command = (char)lightCommand[idx++];
      break;
    case 'g' :
      Serial.println("Fade!");
      lightMode = LM_PALETTE_FADE;
      lightSpeed = (int) lightCommand[idx++];
      lightDwell = (int) lightCommand[idx++];
      command = (char)lightCommand[idx++];
      break;
    case 'r' :
      Serial.println("Random!");
      lightMode = LM_RANDOM;
      lightSpeed = (int) lightCommand[idx++];
      lightDwell = (int) lightCommand[idx++];
      command = (char)lightCommand[idx++];
      break;
    default:
      Serial.println("Default!");
      lightMode = LM_COLOR_LOOP;
      lightSpeed = DEFAULT_LIGHT_SPEED;
      lightDwell = DEFAULT_LIGHT_DWELL;
      command = (char)lightCommand[idx++];
      break;
  }
  
  // Check to make sure speed and dwell aren't set to something weird.
//  if (lightSpeed < MIN_LIGHT_SPEED) {
//    lightSpeed = MIN_LIGHT_SPEED;
//  }
//  if (lightDwell < MIN_LIGHT_DWELL) {
//    lightDwell = MIN_LIGHT_DWELL;
//  }
//  if (lightSpeed > MAX_LIGHT_SPEED) {
//    lightSpeed = MAX_LIGHT_SPEED;
//  }
//  if (lightDwell > MAX_LIGHT_DWELL) {
//    lightDwell = MAX_LIGHT_DWELL;
//  }
  
  if (command != ')') {
    Serial.println("Command is not complete!");
    return;
  }
  else {
    //Send a command received confirmation back to the client
    reply = (uint8_t) '!';
    reply_len = 1;
   
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, &reply, reply_len))
    {
      Serial.println(F("Command input dropped"));
    }
  }
  
  Serial.println("Update Lights!");
  //Send the color palette to the lights
  colorGroup (palette, 0, numColors);
}

void colorGroup(int *pColors, int startIdx, int totalColors)
{
  int currentColorIdx = startIdx;
  int i=0;
  
  while (i < ledsTotal) {
    leds.setPixel(i, pColors[currentColorIdx]);
      if (currentColorIdx == totalColors - 1) {
        currentColorIdx = 0;
      }
      else {
        currentColorIdx++; 
      }
    i++;
  }
  
  //Found bug where leds would be caught in an infinite loop if leds.show() was called before update_in_progress was released.
  if (!leds.busy()) {
    leds.show();
  }
}

//IRC for Color March Timer
void marchIRC(void)
{
  dwellCount++;
  if(dwellCount >= marchHold) {
    marchStarted = true;
    dwellCount = 0;
  } 
}

//IRC for Color Fade Timer
void fadeIRC(void)
{
  fadeCount++;
  if(fadeCount <= fadeDelay) {
    fadeStarted = true;
  }
  else {
    dwellCount++;
  }
  if(dwellCount >= dwellDelay) {
    marchStarted = true;
    dwellCount = 0;
    fadeCount = 0;
  }
}

void marchColors(void) {

  if (marchReverse == true) {  
     marchIndex++;
     if (marchIndex >= numColors) {
        marchIndex = 0; 
     }
  }
  else {
     if (marchIndex == 0) {
        marchIndex = numColors - 1;
     }
     else {
        marchIndex--;
     }  
  }
  
  //Send the color palette to the lights
  colorGroup(palette, marchIndex, numColors);
  
}

void intFadeColors (int currentColor, int nextColor)
{
  
}

void fadeColors(int percent) {
  int firstColor;
  int nextColor;

// TODO: numColors not known at compile time.  Be careful about dynamic memory allocation  
  int newColors[numColors];

  for (int i=0; i < numColors - 1; i++) {
    firstColor = palette[i];
    nextColor = palette[i+1];
    newColors[i] = colorFromColor(nextColor, firstColor, percent);
  }
  firstColor = palette[numColors - 1];
  nextColor = palette[0];
  newColors[numColors - 1] = colorFromColor(nextColor, firstColor, percent);
 
  //Send the color palette to the lights
  colorGroup(newColors, marchIndex, numColors);
  
}

void fadePalette(int percent) {
  int firstColor;
  int nextColor;
  
  int newColor;

  firstColor = palette[marchIndex];
  if(marchIndex >= numColors - 1){
    nextColor = palette[0];
  }
  else {
    nextColor = palette[marchIndex + 1];
  }
  newColor = colorFromColor(firstColor, nextColor, percent);
  
  //Send the color palette to the lights
  colorGroup(&newColor, 0, 1);
  
}


// Fade one RGB color into another based on percent
int colorFromColor (int firstColor, int nextColor, int pct) {
      
    int firstRed;
    int firstGreen;
    int firstBlue;
    int test;
  
    int nextRed;
    int nextGreen;
    int nextBlue;
    
    int toRed;
    int toGreen;
    int toBlue;
    
    float dec = pct/100.0;
    
    //Extract the color Bytes from the palette color
    firstBlue = (int) firstColor & 0xFF;
    firstGreen = (int) (firstColor >> 8) & 0xFF;
    firstRed = (int) (firstColor >> 16) & 0xFF;
    nextBlue = (int) nextColor & 0xFF;
    nextGreen = (int) (nextColor >> 8) & 0xFF;
    nextRed = (int) (nextColor >> 16) & 0xFF;
 
   //Fade individual colors into each other
   toRed = (nextRed - firstRed)*dec + firstRed;
   toGreen = (nextGreen - firstGreen)*dec + firstGreen;
   toBlue = (nextBlue - firstBlue)*dec + firstBlue;
   
  return ((toRed << 16) | (toGreen << 8) | toBlue);
   
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

void readFlash (int flashAddr, int flashReadBytes, byte* myReadBuf) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, 0);
  byte tempByte;
  byte addrByte;
  // Bit order and clock speed is now handled by SPI.beginTransaction;
  tempByte = SPI.transfer(FLASHOP_READ);
  
  addrByte = (flashAddr >> 16) & 0xFF;  
  tempByte = SPI.transfer(addrByte);
  
  addrByte = (flashAddr >> 8) & 0xFF;  
  tempByte = SPI.transfer(addrByte);
  
  addrByte = flashAddr & 0xFF;  
  tempByte = SPI.transfer(addrByte);
  
  for (int fi = 0; fi < flashReadBytes; fi++) {
     myReadBuf[fi] = SPI.transfer(0);  
  }
  
  digitalWrite(MEMCS_N, 1);
  SPI.endTransaction();
}

void writeFlash(int flashAddr, int flashWriteBytes, byte* writeBuf) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, 0);
  byte tempByte;
  byte addrByte;

  // Bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, 1);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, 0);
  tempByte = SPI.transfer(FLASHOP_WRPAGE);
  
  addrByte = (flashAddr >> 16) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = (flashAddr >> 8) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = flashAddr & 0xFF;
  tempByte = SPI.transfer(addrByte);

  for (int fi = 0; fi < flashWriteBytes; fi++) {
     tempByte = SPI.transfer(writeBuf[fi]);  
  }

  digitalWrite(MEMCS_N, 1);
  SPI.endTransaction();  
}

void eraseFlashChip() {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, 0);
  delayMicroseconds(1); // TODO: see if necessary
  byte tempByte;

  // bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, 1);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, 0);
  delayMicroseconds(1); // TODO: see if necessary
  tempByte = SPI.transfer(FLASHOP_CHPERS);
  
   digitalWrite(MEMCS_N, 1);
   SPI.endTransaction();
  
}

void eraseFlashSector(int flashAddr) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, 0);
  byte tempByte;
  byte addrByte;

  // bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, 1);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, 0);
  tempByte = SPI.transfer(FLASHOP_SECERS);
  
  addrByte = (flashAddr >> 16) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = (flashAddr >> 8) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = flashAddr & 0xFF;
  tempByte = SPI.transfer(addrByte);

  digitalWrite(MEMCS_N, 1);
  SPI.endTransaction();
  
}

void eraseFlashBlock(int flashAddr) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, 0);
  byte tempByte;
  byte addrByte;

  // bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, 1);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, 0);
  tempByte = SPI.transfer(FLASHOP_BLKERS);
  
  addrByte = (flashAddr >> 16) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = (flashAddr >> 8) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = flashAddr & 0xFF;
  tempByte = SPI.transfer(addrByte);

  digitalWrite(MEMCS_N, 1);
  SPI.endTransaction();
}

byte checkFlashStatus(){
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, 0);
  delayMicroseconds(1); // TODO: see if necessary
  byte tempByte;
  tempByte = SPI.transfer(FLASHOP_RDSR1);
  delayMicroseconds(1); // TODO: see if necessary   
  tempByte = SPI.transfer(0);
  digitalWrite(MEMCS_N, 1);
  SPI.endTransaction();
  return(tempByte); 
}

boolean checkMoreFiles (int checkAddr) {
   byte headers[4];
   boolean moreFiles = false;  
   readFlash (checkAddr, 1, headers);
   if (headers[0] == '(') {
      moreFiles = true;
   }
   return(moreFiles);   
}

int jumpToNextFile(int jAddr) {
   int nextFileAddr=0;
   int tAddr=jAddr;
   byte headers[4];

   while ((nextFileAddr == 0) && (tAddr < FLASH_USEABLE_SIZE)) {
      readFlash (tAddr, 1, headers);
      switch (headers[0]) {
         case '%': { // New palette
            tAddr++;
            readFlash (tAddr, 1, headers);
            tAddr = tAddr + ((headers[0] + 1) * 3) + 1; // Add (N+1)*3 for palette then 1 to point to next entry
         } break;
         case '!': { // Uncompressed image
            tAddr++;
            readFlash (tAddr, 2, headers);
            tAddr = tAddr + ((headers[0] | (headers[1] << 8)) * 3) + 2;
         } break;
         case '@': { // Compressed
            tAddr++;
            readFlash (tAddr, 2, headers);
            tAddr = tAddr + (headers[0] | (headers[1] << 8)) + 2;      
         } break;
         case '*': { // Video frame
            tAddr++;
            readFlash (tAddr, 2, headers); // read 2 bytes for number of lights
            // need to include 4 bytes for frame interval and number of frames
            tAddr = tAddr + 6 + (headers[0] | (headers[1] << 8));                 
         } break;
         case '&': { // Partial update
            tAddr++;
            readFlash (tAddr, 4, headers);
            tAddr = tAddr + (headers[2] | (headers[3] << 8)) + 4;        
         } break;
         case '^': { // 2 byte hold time
            tAddr = tAddr + 3;       
         } break;
 //        case '.': {
 //           tAddr++; // continuation character for 256 byte padding
 //        } break;
         case ')': {
            tAddr++;
            readFlash(tAddr, 1, headers);
            while(headers[0] == '.') {
               tAddr++;
               readFlash(tAddr, 1, headers);
            }
            nextFileAddr = tAddr;        
         } break;
         case '(': {
            tAddr++;        
         } break;
         default: {
            Serial.print("Unexpected 0x");
            Serial.print(headers[0], HEX);
            Serial.print(" at address 0x");
            Serial.println(tAddr, HEX);
            nextFileAddr = FLASH_USEABLE_SIZE; // skip to end for garbage data
         }
      }  
   }  
  
   return(nextFileAddr);  
}

void scanFlash(int* myFlashFAT){
   int tempAddr = 0;
   flashFiles=0;
   byte headers[4]; // up to 4 header bytes depending on entry type

   Serial.println("Scanning Flash memory");
   while(checkMoreFiles(tempAddr)) {
      if ((tempAddr < FLASH_USEABLE_SIZE) && (flashFiles < MAX_FLASH_FILES)) {
         myFlashFAT[flashFiles] = tempAddr;
         tempAddr = jumpToNextFile(tempAddr);
         flashFiles++;  
      }
   }
   if (tempAddr < FLASH_USEABLE_SIZE) {
     flashWriteAddress = tempAddr;
   }
   else {
     flashWriteAddress = 0;
   }
   Serial.print("Scan complete. ");
   Serial.print(flashFiles, DEC);
   Serial.println(" valid files found");   
   
}

byte checkFileType(int fileStartAddr) {
   int tAddr = fileStartAddr;
   byte fileType = FILE_TYPE_NONE;
   byte headers[4];
 
   Serial.print("checkFileType: ");
   Serial.println(fileStartAddr, DEC);  
   while ((fileType == FILE_TYPE_NONE) && (tAddr < FLASH_USEABLE_SIZE)) {
  
      readFlash (tAddr, 1, headers);
      switch (headers[0]) {
         case '(': {
            tAddr++;  // File start character, keep going
         } break;
         case '%': { // New palette, keep looking
            tAddr++;
            readFlash (tAddr, 1, headers);
            tAddr = tAddr + ((headers[0] + 1) * 3) + 1; // Add (N+1)*3 for palette then 1 to point to next entry
         } break;
         case '!': {
            fileType = FILE_TYPE_IMAGE;
         } break;
         case '@': {
            fileType = FILE_TYPE_COMP_IMAGE;  
         } break;
         case '*':
         case '&':
         case '^': {
            fileType = FILE_TYPE_VIDEO;
         } break;
         default: {
            fileType = FILE_TYPE_CORRUPT;
            Serial.println("Corrupt file type detected");  
         }
      }
   }
   return(fileType);  
}

void showFlashImage(int fileStartAddr) {
   int tAddr = fileStartAddr;
//   byte readBuf[4];
   boolean goodImage = true;
   int RGB_value;
   byte thisByte;
  
   Serial.println ("showFlashImage");    
   readFlash (tAddr, 1, readBuf);
   thisByte = readBuf[0];
   while (thisByte != '!') { // advance to actual image start
      if (thisByte == '%') { // update palette
         tAddr = flashUpdatePalette(tAddr);
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
      else {
//         Serial.print("+");
         tAddr++;
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }   
   }
   tAddr++; // While escapes at '!' character, need to increment past it
   
   // Now comes 2 bytes for number of lights

   readFlash(tAddr, 2, readBuf);
   tAddr += 2; // Need to increment address
   numLights = (readBuf[1] << 8) | readBuf[0];
   // Need to use leds.setPixel since drawingMemory is 8 channels per byte
   // Updating drawing memory directly would be an inefficient use of bandwidth for controllers with < 8 active channels
   for (int i=0; i<numLights; i++) {
      readFlash(tAddr, 3, readBuf);
      RGB_value = readBuf[0] | (readBuf[1] << 8) | (readBuf[2] << 16);
      leds.setPixel(i,RGB_value);
      tAddr += 3;
   }
   leds.show();   
}

void showCompImage(int fileStartAddr) {
   int tAddr = fileStartAddr;
//   byte readBuf[4];
   boolean goodImage = true;
   int RGB_value;
   byte thisByte;
  
   Serial.println ("showCompImage");    
   readFlash (tAddr, 1, readBuf);
   thisByte = readBuf[0];
   while (thisByte != '@') { // advance to actual image start
      if (thisByte == '%') { // update palette
         tAddr = flashUpdatePalette(tAddr);
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
      else {
//         Serial.print("+");
         tAddr++;
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }   
   }
   tAddr++; // While escapes at '@' character, need to increment past it
   
   // Now comes 2 bytes for number of lights

   readFlash(tAddr, 2, readBuf);
   tAddr += 2; // Need to increment address
   numLights = (readBuf[1] << 8) | readBuf[0];
   // Need to use leds.setPixel since drawingMemory is 8 channels per byte
   // Updating drawing memory directly would be an inefficient use of bandwidth for controllers with < 8 active channels
   for (int i=0; i<numLights; i++) {
      readFlash(tAddr, 1, readBuf);
      leds.setPixel(i,palette[readBuf[0]]);
      tAddr++;
   }
   leds.show();   
}


int flashUpdatePalette(int startAddr) {
   int tAddr = startAddr;
//   byte readBuf[3];
   int numColors; 

   tAddr++; // function enterred at '%' character
   readFlash (tAddr, 1, readBuf);
   numColors = readBuf[0] + 1; // read number of bytes
   tAddr++; // Advance to start of palette array

   int i;
   for (i=0; i < numColors; i++) {
      readFlash(tAddr, 3, readBuf);
      palette[i] = readBuf[0] | (readBuf[1] << 8) | (readBuf[2] << 16); // Little-endian
      //palette[i] = readBuf[2] | (readBuf[1] << 8) | (readBuf[0] << 16); // Big-endian
      tAddr = tAddr + 3;  
   }
   while (i < 256) {
      palette[i] = 0; // Pad rest of palette with black
      i++;  
   }
   //Serial.println("Palette updated");
   return(tAddr);
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
//   byte readBuf[4];
   boolean goodImage = true;
   int RGB_value;
   byte thisByte;
   int skipCountCopy;
   
//   Serial.println ("playFlashVideo");
   flashTransferPending = true;
   readFlash (tAddr, 1, readBuf);
   thisByte = readBuf[0];
   while (thisByte != '*') { // advance to actual video start
      if (thisByte == '%') { // update palette
         tAddr = flashUpdatePalette(tAddr);
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
      else if (thisByte == ')') { // end of file reached, jump back to start
         // This is an ugly use of a global variable, but it works
         tAddr =  flashFAT[currentFlashFile] + 1; // No need to read '(' at start of file
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0]; 
      }
      else {
//         Serial.print("+");
         tAddr++;
         readFlash (tAddr, 1, readBuf);
         thisByte = readBuf[0];
      }
   }
   tAddr++; // while escapes at '*' character, need to increment past it
   // Now comes 2 bytes for number of lights
   readFlash(tAddr, 2, readBuf);
   tAddr += 2; // Need to increment address
   numLights = (readBuf[1] << 8) | readBuf[0];

    // Now comes 2 bytes for frame units in microseconds and 2 bytes for holdFrames
   readFlash(tAddr, 4, readBuf);
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
         readFlash(tAddr, 1, readBuf);
         leds.setPixel(i,palette[readBuf[0]]); // Video uses 8-bit palette
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
      //Timer3.start();
   }
   //Serial.println(tAddr, HEX);
   return(tAddr);      
}

void playLightsFromFile(int fileStartAddr) {

   Serial.print("playLightsFromFile: ");
   Serial.print(currentFlashFile, DEC);
   Serial.print(", ");
   Serial.println(fileStartAddr, DEC);
   byte fileType = checkFileType(fileStartAddr);
   currentFileType = fileType; // assign global variable for use in light_loop
   if (fileType == FILE_TYPE_IMAGE) {
      showFlashImage(fileStartAddr);  
   }
   else if (fileType == FILE_TYPE_COMP_IMAGE) {
     showCompImage(fileStartAddr);
   }
   else if (fileType == FILE_TYPE_VIDEO) {
       videoAddr = playFlashVideo(fileStartAddr);
   }
}

// The videoISR runs every time the frame interval timer generates an interrupt.  This could be as often as 100
// times per second in the case of an animated GIF.  
void videoISR(void) {
   holdCount++;
   if (holdCount >= holdFrames) {
      // Reset holdCount and update holdFrames regardless of whether or not frame needs to be skipped
      holdCount = 0;
      holdFrames = nextHoldFrames;
      if ((flashTransferPending)|| leds.busy()) { // check to see that the lights are ready and the new data is ready to show
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



