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

#include "dleds_flash.h"

DLEDsFlash::DLEDsFlash() : norFlash_SPISettings(32000000, MSBFIRST, SPI_MODE0) {
  currentFlashFile = 0;
  aciPageReceived = false;
}

void DLEDsFlash::begin(void) {
    pinMode(MEMCS_N, OUTPUT);
    SPI.begin();
    digitalWrite(MEMCS_N, HIGH);   // JRE 10/19/14 hold the memory CS_N high to avoid bus contention
}

boolean DLEDsFlash::isAciPageReceived(void) {
  return aciPageReceived;
}

void DLEDsFlash::setAciPageReceived(boolean aciPageReceived) {
  this->aciPageReceived = aciPageReceived;
}

int DLEDsFlash::getFlashFiles(void) {
  return flashFiles;
}

void DLEDsFlash::updateCurrentFlashFile(boolean flashForward) {
  if(flashForward) {
    currentFlashFile++;
    if (currentFlashFile == flashFiles) {
      currentFlashFile = 0;  
    }
  }
  else {
    if (currentFlashFile != 0) {
      currentFlashFile--;
    }
    else {
      currentFlashFile = flashFiles - 1;  
    }
  }
}

int DLEDsFlash::getCurrentFlashFile(void) {
  return currentFlashFile;
}

void DLEDsFlash::setCurrentFlashFile(int currentFlashFile) {
  if (currentFlashFile > flashFiles) {
    this->currentFlashFile = 0;
  }
  else {
    this->currentFlashFile = currentFlashFile;
  }
}

int DLEDsFlash::getCurrentFlashFAT() {
  return flashFAT[currentFlashFile];
}

boolean DLEDsFlash::busy(void){
  if (checkFlashStatus() & 0x01) {
    return true;
  }
  else {
    return false;
  }
}

//Scan Flash must be done during setup to set writeAddress to the correct value
void DLEDsFlash::scanFlash(void){
  
  while (checkFlashStatus() & 0x01) {
    delay(1);
  } // Wait for Flash to be ready
  
  //Start scanning at end of file table (8k)
  int tempAddr = FILE_TABLE_SIZE;
  flashFiles=0;

  Serial.println("Scanning Flash memory");
  while(checkMoreFiles(tempAddr)) {
    if ((tempAddr < FLASH_USEABLE_SIZE) && (flashFiles < MAX_FLASH_FILES)) {
      flashFAT[flashFiles] = tempAddr;
      getFlashFileName(flashFiles, pageBuf);
      tempAddr = jumpToNextFile(tempAddr);
      flashFiles++;  
    }
  }
  if (tempAddr < FLASH_USEABLE_SIZE) {
    writeAddress = tempAddr;
    writeNameAddress = flashFiles*MAX_FILENAME_SIZE;
  }
  else {
    //First available write address is at end of file table
    writeAddress = FILE_TABLE_SIZE;
    writeNameAddress = 0;
  }
  
  Serial.print("Scan complete. ");
  Serial.print(flashFiles, DEC);
  Serial.println(" valid files found");   
  
}

uint8_t DLEDsFlash::getFlashFileName(int fileNum, byte* fileName){
  int idx = 0;
  
  readFlash(fileNum*MAX_FILENAME_SIZE, MAX_FILENAME_SIZE, fileName);
  
  while(fileName[idx] != 0) {
    if(idx < MAX_FILENAME_SIZE) {
      Serial.print((char) fileName[idx++]);
    }
    else {
      return MAX_FILENAME_SIZE;
    }
  }
  Serial.print('\n');
  
  //Return the length of the file name
  return idx;
}

void DLEDsFlash::writeFlashFileName(uint8_t* command_buffer) {
  
  int idx = 3; //Skip first 3 characters of the command
  int addr = 0;
  int writePage = writeNameAddress / FLASH_PAGE_SIZE;
  int pageAddr = writeNameAddress % FLASH_PAGE_SIZE;
  
  //First read the page on which the name will live
  readFlash(writePage, FLASH_PAGE_SIZE, pageBuf);
  
  //Change the bytes of the name address
  while((char) command_buffer[idx] != ')'){
    //Limit the number of characters to 31 (1 used for null termination)
    if(addr < MAX_FILENAME_SIZE - 1) {
      pageBuf[pageAddr + addr] = command_buffer[idx++];
      addr++;
    }
    else {
      return;
    }
  }
  Serial.print('\n');
  //Add NULL to end of name string
  pageBuf[pageAddr + addr] = 0;
  
  while (checkFlashStatus() & 0x01) {
    delay(1);
  } // Wait for Flash to be ready for a write
  
  //Re-write the page to flash
  writeFlash(writePage, FLASH_PAGE_SIZE, pageBuf);
  
  aciPageReceived = true; // Acknowledge page data received
}

void DLEDsFlash::serialFlashPageWrite(void) {
   int bytesOnPage = Serial.read() + 1; // 0-255 maps into 1-256 bytes
   count = Serial.readBytes((char *)pageBuf, bytesOnPage);
   if (count == bytesOnPage) {
      while (bytesOnPage < FLASH_PAGE_SIZE) {
         pageBuf[bytesOnPage] = '.';  // Used to pad last page in file
         bytesOnPage++;   
      }
 
      while (checkFlashStatus() & 0x01) {
         delay(1);
      } // Wait for Flash to be ready for a write        
      writeFlash( writeAddress, FLASH_PAGE_SIZE, pageBuf);
      //flashFAT[flashFiles] = writeAddress;
      //flashFiles++;
      writeAddress += FLASH_PAGE_SIZE; // always write full page
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

void DLEDsFlash::aciFlashPageWrite(uint8_t* command_buffer) {
  count = 0;
  int idx = 2; //Skip '(' and '$' characters
  int bytesOnPage = command_buffer[idx++] + 1; // 0-255 maps into 1-256 bytes
  
  for(int i=0; i < bytesOnPage; i++) {
    pageBuf[i] = command_buffer[idx++];
    count++;
  }
  if (count == bytesOnPage) {
    while (bytesOnPage < FLASH_PAGE_SIZE) {
      pageBuf[bytesOnPage] = '.';  // Used to pad last page in file
      bytesOnPage++;   
    }
 
    while (checkFlashStatus() & 0x01) {
      delay(1);
    } // Wait for Flash to be ready for a write
    writeFlash( writeAddress, FLASH_PAGE_SIZE, pageBuf);
    //flashFAT[flashFiles] = writeAddress;
    //flashFiles++;
    writeAddress += FLASH_PAGE_SIZE; // always write full page
    aciPageReceived = true; // Acknowledge page received
    }
  else {
    if (count > bytesOnPage) {
      aciPageReceived = false;
      Serial.println('>'); // Too many bytes
    }
    else {
      aciPageReceived = false;
      Serial.println('<'); // Too few bytes
    }
  }  
}

int DLEDsFlash::jumpToNextFile(int jAddr) {
   int nextFileAddr=0;
   int tAddr=jAddr;
   byte headers[4];

   while ((nextFileAddr == 0) && (tAddr < FLASH_USEABLE_SIZE)) {
      readFlash(tAddr, 1, headers);
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

boolean DLEDsFlash::checkMoreFiles (int checkAddr) {
   byte headers[4];
   boolean moreFiles = false;  
   while (checkFlashStatus() & 0x01) {
     delay(1);
   } // Wait for Flash to be ready
   readFlash(checkAddr, 4, headers);
   if (headers[0] == '(') {
      moreFiles = true;
   }
   return(moreFiles);   
}

void DLEDsFlash::eraseFlashFiles(void) {
  int delayCycles=0;
  
  Serial.println("Erasing flash files");
  eraseFlashChip();
  delayCycles=0;
  while (busy()) {
    delay(10);
    delayCycles++;  
  }
  
  //Reset Flash Count variables
  flashFiles = 0;
  //First available write address is at end of file table
  writeAddress = FILE_TABLE_SIZE;
  writeNameAddress = 0;
  currentFlashFile = 0;
  for(int i=0; i < MAX_FLASH_FILES; i++) {
    flashFAT[i] = 0;
  }
  
  Serial.print("Flash erase completed in ");
  Serial.print(delayCycles, DEC);
  Serial.println(" delay(10) cycles");
}

void DLEDsFlash::printFlashSector(void) {
  for (int tpage=0; tpage < 16; tpage++) {
    readFlash(256*tpage, 256, pageBuf);
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
        byte tempDat = pageBuf[16*i + j];
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
}

FileType_t DLEDsFlash::checkFileType(int fileStartAddr) {
   int tAddr = fileStartAddr;
   FileType_t fileType = ftNone;
   byte headers[4];
 
   //Serial.print("checkFileType: ");
   //Serial.println(fileStartAddr, DEC);  
   while ((fileType == ftNone) && (tAddr < FLASH_USEABLE_SIZE)) {
  
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
            fileType = ftImage;
         } break;
         case '@': {
            fileType = ftCompImage;  
         } break;
         case '*':
         case '&':
         case '^': {
            fileType = ftVideo;
         } break;
         default: {
            fileType = ftCorrupt;
            Serial.println("Corrupt file type detected");  
         }
      }
   }  
   return fileType;
}

byte DLEDsFlash::checkFlashStatus(void){
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, LOW);
  delayMicroseconds(1); // TODO: see if necessary
  byte tempByte;
  tempByte = SPI.transfer(FLASHOP_RDSR1);
  delayMicroseconds(1); // TODO: see if necessary   
  tempByte = SPI.transfer(0);
  digitalWrite(MEMCS_N, HIGH);
  SPI.endTransaction();
  return(tempByte); 
}

void DLEDsFlash::readFlash (int flashAddr, int flashReadBytes, byte* myReadBuf) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, LOW);
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
  
  digitalWrite(MEMCS_N, HIGH);
  SPI.endTransaction();
}

void DLEDsFlash::writeFlash(int flashAddr, int flashWriteBytes, byte* writeBuf) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, LOW);
  byte tempByte;
  byte addrByte;

  // Bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, HIGH);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, LOW);
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

  digitalWrite(MEMCS_N, HIGH);
  SPI.endTransaction();  
}

void DLEDsFlash::eraseFlashChip(void) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, LOW);
  delayMicroseconds(1); // TODO: see if necessary
  byte tempByte;

  // bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, HIGH);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, LOW);
  delayMicroseconds(1); // TODO: see if necessary
  tempByte = SPI.transfer(FLASHOP_CHPERS);
  
   digitalWrite(MEMCS_N, HIGH);
   SPI.endTransaction();
  
}

void DLEDsFlash::eraseFlashSector(int flashAddr) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, LOW);
  byte tempByte;
  byte addrByte;

  // bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, HIGH);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, LOW);
  tempByte = SPI.transfer(FLASHOP_SECERS);
  
  addrByte = (flashAddr >> 16) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = (flashAddr >> 8) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = flashAddr & 0xFF;
  tempByte = SPI.transfer(addrByte);

  digitalWrite(MEMCS_N, HIGH);
  SPI.endTransaction();
  
}

void DLEDsFlash::eraseFlashBlock(int flashAddr) {
  SPI.beginTransaction(norFlash_SPISettings);
  digitalWrite(MEMCS_N, LOW);
  byte tempByte;
  byte addrByte;

  // bit order now handled in SPI.beginTransaction

  tempByte = SPI.transfer(FLASHOP_WREN);
  digitalWrite(MEMCS_N, HIGH);
  
  delayMicroseconds(1);
  digitalWrite(MEMCS_N, LOW);
  tempByte = SPI.transfer(FLASHOP_BLKERS);
  
  addrByte = (flashAddr >> 16) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = (flashAddr >> 8) & 0xFF;
  tempByte = SPI.transfer(addrByte);
  
  addrByte = flashAddr & 0xFF;
  tempByte = SPI.transfer(addrByte);

  digitalWrite(MEMCS_N, HIGH);
  SPI.endTransaction();
}

//void DLEDsFlash::serialEraseFlash() {
//     byte flashStatus;
//     flashStatus = checkFlashStatus();
//     if (flashStatus & 0x01) { 
//        eraseFlashChip();
//        int delayCount=0;
//        boolean eraseTimeout = false;
//        Serial.write('.');
//        while ((checkFlashStatus() & 0x01) && !eraseTimeout) {
//           Serial.write('.');
//           delay(1000);
//           delayCount++;
//           if (delayCount > 60) {
//              eraseTimeout = true;
//           } 
//        }
//        if (eraseTimeout) {
//           Serial.println(F("Error: Flash Erase Timeout"));
//        }
//        else {
//           Serial.print(F("Flash erase complete in ~"));
//           Serial.print(delayCount);
//           Serial.println(F(" seconds"));
//           //Here
//        }
//     } // end if checkFlashStatus...
//     else {
//        Serial.print("Error: Flash status = ");
//        Serial.print(flashStatus);
//        Serial.println(" prior to erase");     
//     }
////     Serial.flush(); // Make sure all serial data transmits before leaving function  
//}


//void DLEDsFlash::serialFlashFileClose () {
// TODO  
//}
