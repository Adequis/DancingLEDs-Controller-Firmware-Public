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

#ifndef DLEDS_ACI_H
#define DLEDS_ACI_H

#include <Arduino.h>

#include <SPI.h>
#include <lib_aci.h>
#include <aci_setup.h>
#include "uart_over_ble.h"

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
*/

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

#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

/* Store the setup for the nRF8001 in the flash of the AVR to save on RAM */
static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

#define MAX_PACKET_SIZE 20

#define MAX_PIN_LEN 4

class DLEDsACI {
private:

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
struct aci_state_t aci_state;
/*
Temporary buffers for sending ACI commands
*/
hal_aci_evt_t  aci_data;
//static hal_aci_data_t aci_cmd;

/*
Timing change state variable
*/
bool timing_change_done;
/*
Used to test the UART TX characteristic notification
*/
uart_over_ble_t uart_over_ble;
uint8_t         uart_buffer[20];
uint8_t         uart_buffer_len;
uint8_t         dummychar;
//bool radio_ack_pending;

uint8_t* name;
uint8_t name_len;

uint8_t clientPin[MAX_PIN_LEN];

boolean commandReceived;

boolean connected;

public:
 DLEDsACI();
  
 void begin(void);
  
 void uart_over_ble_init(void);
 bool uart_tx(uint8_t *buffer, uint8_t buffer_len);
 bool uart_process_control_point_rx(uint8_t *byte, uint8_t length);
 void uart_disconnect(void);
  
 void checkAciEvt(void);
 boolean isCommandReceived(void);
 void setCommandReceived(boolean commandReceived);
 uint8_t* getUartBuffer(void);
 uint8_t getUartBufferLen(void);
 void clearUartBuffer(void);
 void sendChar(char response);
 void sendString(char* response, uint8_t strLen);
 void setName(uint8_t* name, uint8_t name_len);
 void setSecurityPin(uint8_t* securityPin, uint8_t pin_len);
 void resetClientPin(void);
 uint8_t* getClientPin(void);
 void setClientPin(uint8_t* clientPin);
  
};

#endif
