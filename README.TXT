The ble_OctoWS2811.ino file requires the nrf8001_arm_support_new, TimerThree, Time, TimeAlarms, OctoWS2811 and SPI libraries to be installed.

nrf8001_arm_support_new has been updated to support Teensy SPI.

The TimerThree and SPI libraries may have been installed as part of the Arduino or Teensyduino installation, so make sure to replace them.  In Windows, libraries located under Program Files/Arduino/Libraries take precedence over libraries under My Documents/Arduino/Libraries.

Development occurs in Arduino 1.0.6 and Teensyduino 1.20.

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
