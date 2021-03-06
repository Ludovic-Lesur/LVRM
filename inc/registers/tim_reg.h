/*
 * tim_reg.h
 *
 *  Created on: 3 may 2018
 *      Author: Ludo
 */

#ifndef TIM_REG_H
#define TIM_REG_H

/*** TIMx registers ***/

typedef struct {
	volatile unsigned int CR1;    	// Control register 1.
	volatile unsigned int CR2;    	// Control register 2.
	volatile unsigned int SMCR;    	// Slave mode controler register (!)
	volatile unsigned int DIER;    	// DMA interrupt enable register.
	volatile unsigned int SR;    	// Status register.
	volatile unsigned int EGR;    	// Event generation register.
	volatile unsigned int CCMR1;    // Capture/compare mode register 1 (!).
	volatile unsigned int CCMR2;    // Capture/compare mode register 2 (!).
	volatile unsigned int CCER;    	// Capture/compare enable register (!).
	volatile unsigned int CNT;    	// Counter register.
	volatile unsigned int PSC;    	// Prescaler register.
	volatile unsigned int ARR;    	// Auto-reload register.
	unsigned int RESERVED0;    		// Reserved 0x30.
	volatile unsigned int CCRx[4];  // Capture/compare registers 1-4 (!).
	unsigned int RESERVED1;    		// Reserved 0x44
	volatile unsigned int DCR;    	// DMA control register (!).
	volatile unsigned int DMAR;    	// DMA address for full transfer register (!).
	volatile unsigned int OR;    	// Option register (!).
} TIM_base_address_t;

/*** TIMx base addresses ***/

#define TIM2	((TIM_base_address_t*) ((unsigned int) 0x40000000))
//#define TIM3	((TIM_base_address_t*) ((unsigned int) 0x40000400)) // Not present on STM32L011F3xx.
#define TIM21	((TIM_base_address_t*) ((unsigned int) 0x40010800))
//#define TIM22	((TIM_base_address_t*) ((unsigned int) 0x40011400)) // Not present on STM32L011F3xx.
//#define TIM6	((TIM_base_address_t*) ((unsigned int) 0x40001000)) // Not present on STM32L011F3xx.
//#define TIM7	((TIM_base_address_t*) ((unsigned int) 0x40001400)) // Not present on STM32L011F3xx.

#endif /* TIM_REG_H */
