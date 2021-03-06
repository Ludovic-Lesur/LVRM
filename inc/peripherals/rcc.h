/*
 * rcc.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef RCC_H
#define RCC_H

/*** RCC macros ***/

#define RCC_LSI_FREQUENCY_HZ	38000
#define RCC_LSE_FREQUENCY_HZ	32768
#define RCC_MSI_FREQUENCY_KHZ	2100

/*** RCC functions ***/

void RCC_init(void);
void RCC_enable_lsi(void);
void RCC_enable_lse(void);

#endif /* RCC_H */
