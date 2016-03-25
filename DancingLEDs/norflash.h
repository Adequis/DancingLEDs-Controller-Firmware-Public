#ifndef NORFLASH_H
#define NORFLASH_H

//Read Data
#define FLASHOP_READ       0x03
#define FLASHOP_READ_REV   0xC0
//Read Status Register 1
#define FLASHOP_RDSR1      0x05
#define FLASHOP_RDSR1_REV  0xA0
//Read Status Register 2
#define FLASHOP_RDSR2      0x35
#define FLASHOP_RDSR2_REV  0xAC
//Read Status Register 3
#define FLASHOP_RDSR3      0x33
#define FLASHOP_RDSR3_REV  0xCC
//Set Write Enabled
#define FLASHOP_WREN       0x06
#define FLASHOP_WREN_REV   0x60
//Set write enabled of volatile status register
#define FLASHOP_VWREN      0x50
#define FLASHOP_VWREN_REV  0x0A
//Write Disable
#define FLASHOP_WRDI       0x04
#define FLASHOP_WRDI_REV   0x20
//Write Status Registers
#define FLASHOP_WRSR       0x01
#define FLASHOP_WRSR_REV   0x80
//Write Page program
#define FLASHOP_WRPAGE     0x02
#define FLASHOP_WRPAGE_REV 0x40
//Sector Erase
#define FLASHOP_SECERS     0x20
#define FLASHOP_SECERS_REV 0x04
//Block Erase
#define FLASHOP_BLKERS     0xD8
#define FLASHOP_BLKERS_REV 0x1B
//Chip Erase
#define FLASHOP_CHPERS     0xC7
#define FLASHOP_CHPERS_REV 0xE3

#endif
