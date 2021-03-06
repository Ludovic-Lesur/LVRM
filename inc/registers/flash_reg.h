/*
 * nvm_reg.h
 *
 *  Created on: 19 june 2018
 *      Author: Ludo
 */

#ifndef FLASH_REG_H
#define FLASH_REG_H

/*** FLASH registers ***/

typedef struct {
	volatile unsigned int ACR;		// NVM interface access control register.
	volatile unsigned int PECR;		// NVM interface program and erase control register.
	volatile unsigned int PDKEYR;	// NVM interface power down key register.
	volatile unsigned int PEKEYR;	// NVM interface PECR unlock key register.
	volatile unsigned int PRGKEYR;	// NVM interface program and erase key register.
	volatile unsigned int OPTKEYR;	// NVM interface option bytes unlock key register.
	volatile unsigned int SR;		// NVM interface status register.
	volatile unsigned int OPTR;		// NVM interface option bytes register.
	volatile unsigned int WRPROT1;	// NVM interface write protection register 1.
	unsigned int RESERVED[23];		// Reserved 0x24.
	volatile unsigned int WRPROT2;	// NVM interface write protection register 2.
} FLASH_base_address_t;

/*** FLASH registers base address ***/

#define FLASH	((FLASH_base_address_t*) ((unsigned int) 0x40022000))

/*** EEPROM address range ***/

#define EEPROM_START_ADDRESS	(unsigned int) 0x08080000
#define EEPROM_SIZE				1024 // 1kB for STM32L041xxxx (category 2 device).

#endif /* FLASH_REG_H */
