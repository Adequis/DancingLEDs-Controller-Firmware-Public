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

#include "dleds_aci.h"

DLEDsACI::DLEDsACI(){
  
  name = (uint8_t *) "DLEDsGA1";
  name_len = 8;
  
  timing_change_done = false;
  uart_buffer_len = 0;
  dummychar = 0;
  /*
  Initialize the radio_ack. This is the ack received for every transmitted packet.
  */
  //radio_ack_pending = false;
  commandReceived = false;
  connected = false;
  
  resetClientPin();
  
}

void DLEDsACI::begin(void) {
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
}

void DLEDsACI::uart_over_ble_init(void)
{
  uart_over_ble.uart_rts_local = true;
}

bool DLEDsACI::uart_tx(uint8_t *buffer, uint8_t buffer_len)
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

bool DLEDsACI::uart_process_control_point_rx(uint8_t *byte, uint8_t length)
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

void DLEDsACI::uart_disconnect(void) {
  lib_aci_disconnect(&aci_state, ACI_REASON_TERMINATE);
}

void DLEDsACI::checkAciEvt(void)
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
              //Set advertised name to something other than the default.
              lib_aci_set_local_data(&aci_state, PIPE_GAP_DEVICE_NAME_SET, name, name_len);
              
              lib_aci_connect(0/* in seconds : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
              Serial.println(F("Advertising started!"));
            }
            connected = false;

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
        connected = true;
        
        //Reset client pin
        resetClientPin();
        
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
        
        //Set advertised name to something other than the default.
        lib_aci_set_local_data(&aci_state, PIPE_GAP_DEVICE_NAME_SET, name, name_len);
        
        lib_aci_connect(0/* in seconds  : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started."));
        connected = false;
        break;

      case ACI_EVT_DATA_RECEIVED:
        //Serial.print(F("Pipe Number: "));
        //Serial.println(aci_evt->params.data_received.rx_data.pipe_number, DEC);

        if (PIPE_UART_OVER_BTLE_UART_RX_RX == aci_evt->params.data_received.rx_data.pipe_number)
          {

            //Serial.print(F(" Data(Hex) : "));
            for(int i=0; i<aci_evt->len - 2; i++)
            {
              //Serial.print("0x");
              //Serial.print(aci_evt->params.data_received.rx_data.aci_data[i],HEX);
              uart_buffer[i] = aci_evt->params.data_received.rx_data.aci_data[i];
              //Serial.print(F(" "));
            }
            //Serial.println();
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
        
        //Set advertised name to something other than the default.
        lib_aci_set_local_data(&aci_state, PIPE_GAP_DEVICE_NAME_SET, name, name_len);
              
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

boolean DLEDsACI::isCommandReceived(void) {
  return commandReceived;
}

void DLEDsACI::setCommandReceived(boolean commandReceived) {
  this->commandReceived = commandReceived;
}

uint8_t* DLEDsACI::getUartBuffer(void) {
  return uart_buffer;
}

uint8_t DLEDsACI::getUartBufferLen(void) {
  return uart_buffer_len;
}

void DLEDsACI::clearUartBuffer(void) {
  for (int i=0; i < uart_buffer_len; i++)
  {
    uart_buffer[i] = ' ';
  }
}

void DLEDsACI::sendChar(char response) {
  //Send a command received confirmation back to the client
  uint8_t data = (uint8_t) response;
  uint8_t data_len = sizeof(uint8_t);
  if (connected) {
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, &data, data_len))
    {
      Serial.println(F("Command input dropped"));
    }
  }
}

void DLEDsACI::sendString(char* response, uint8_t strLen) {
  uint8_t* dataPacket;
  uint8_t packetSize;
  uint8_t lastBytes;
  
  //Multiple response packets not yet supported
  //uint8_t numPackets = strLen / MAX_PACKET_SIZE;
  //uint8_t lastBytes = strLen % MAX_PACKET_SIZE;
  
  //Single Packet Response
  uint8_t numPackets = 0;
  if (strLen > MAX_PACKET_SIZE) {
    lastBytes = MAX_PACKET_SIZE;
  }
  else {
    lastBytes = strLen;
  }
  
//  //If numPackets = 0 this is skipped
//  for (int i=0; i < numPackets; i++) {
//    dataPacket = (uint8_t*) &response[i*MAX_PACKET_SIZE];
//    packetSize = MAX_PACKET_SIZE;
//    if (connected) {
//      if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, dataPacket, packetSize))
//      {
//        Serial.println(F("Command input dropped"));
//      }
//    }
//  }
  
  dataPacket = (uint8_t*) &response[numPackets*MAX_PACKET_SIZE];
  packetSize = lastBytes;
 
  if (connected) {
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, dataPacket, packetSize))
      {
        Serial.println(F("Command input dropped"));
      }
  }
}

void DLEDsACI::setName(uint8_t* name, uint8_t name_len) {
  this->name = name;
  this->name_len = name_len;
}

void DLEDsACI::setSecurityPin(uint8_t* securityPin, uint8_t pin_len) {
//  this->securityPin = securityPin;
//  this->pin_len = pin_len;
}

void DLEDsACI::resetClientPin(void) {
  for(int i=0; i < MAX_PIN_LEN; i++) {
    clientPin[i] = 0;
  }
}
  
uint8_t* DLEDsACI::getClientPin(void) {
  return clientPin;
}

void DLEDsACI::setClientPin(uint8_t* clientPin) {
  for(int i=0; i < MAX_PIN_LEN; i++) {
    this->clientPin[i] = clientPin[i];
  }
}


