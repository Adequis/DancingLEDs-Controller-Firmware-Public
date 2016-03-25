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

#ifndef DLEDS_FLASH_H
#define DLEDS_FLASH_H

#include <Arduino.h>
#include <SPI.h>
#include "norflash.h"

//Hardware Definitions
#define MEMCS_N 22

#define FLASH_MEMORY_SIZE 4194304 // 4Mbyte total Flash size
#define FLASH_RSVD_SIZE 524288 // 512Kbyte reserved
#define FLASH_USEABLE_SIZE (FLASH_MEMORY_SIZE - FLASH_RSVD_SIZE)
#define MAX_FLASH_FILES 256
#define FLASH_PAGE_SIZE 256
#define FILE_TABLE_SIZE 8192
#define MAX_FILENAME_SIZE 32

typedef enum FileType_t { ftNone, ftImage, ftCompImage, ftVideo = 4, ftCorrupt = 255 };

class DLEDsFlash {
private:
  int count;
  int flashFAT[MAX_FLASH_FILES];
  int flashFiles; // Number of files in flash
  int currentFlashFile;
  boolean aciPageReceived;
  
  byte checkFlashStatus(void);
  void eraseFlashChip(void);
  void eraseFlashSector(int flashAddr);
  void eraseFlashBlock(int flashAddr);

public:
  DLEDsFlash();
  SPISettings norFlash_SPISettings;
  
  byte pageBuf[256];
  int writeAddress; // where next flash write will start
  int writeNameAddress;
  
  void begin(void);
  boolean isAciPageReceived(void);
  void setAciPageReceived(boolean aciPageReceived);
  int getFlashFiles(void);
  void updateCurrentFlashFile(boolean flashForward);
  int getCurrentFlashFile(void);
  void setCurrentFlashFile(int currentFlashFile);
  int getCurrentFlashFAT(void);
  boolean busy(void);
  void scanFlash(void);
  uint8_t getFlashFileName(int fileNum, byte* fileName);
  void writeFlashFileName(uint8_t* command_buffer);
  void serialFlashPageWrite(void);
  void aciFlashPageWrite(uint8_t* command_buffer);
  int jumpToNextFile(int jAddr);
  boolean checkMoreFiles (int checkAddr);
  FileType_t checkFileType(int fileStartAddr);
  void eraseFlashFiles(void);
  void printFlashSector(void);

  void readFlash(int flashAddr, int flashReadBytes, byte* myReadBuf);
  void writeFlash(int flashAddr, int flashWriteBytes, byte* writeBuf);
  
  //void serialEraseFlash(void);
  //void serialFlashFileClose(void);
  
};

#endif
