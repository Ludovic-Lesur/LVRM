/*
 * adc.c
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#include "adc.h"

#include "adc_reg.h"
#include "gpio.h"
#include "lptim.h"
#include "mapping.h"
#include "math.h"
#include "rcc_reg.h"

/*** ADC local macros ***/

#define ADC_CHANNEL_VIN						6
#define ADC_CHANNEL_VOUT					4
#define ADC_CHANNEL_IOUT					0
#define ADC_CHANNEL_VREFINT					17

#define ADC_MEDIAN_FILTER_LENGTH			9
#define ADC_CENTER_AVERAGE_LENGTH			3

#define ADC_FULL_SCALE_12BITS				4095

#define ADC_VREFINT_VOLTAGE_MV				((VREFINT_CAL * VREFINT_VCC_CALIB_MV) / (ADC_FULL_SCALE_12BITS))
#define ADC_VMCU_DEFAULT_MV					3000

#define ADC_VOLTAGE_DIVIDER_RATIO_VIN		10
#define ADC_VOLTAGE_DIVIDER_RATIO_VOUT		10

#define ADC_LT6106_VOLTAGE_GAIN				59
#define ADC_LT6106_SHUNT_RESISTOR_MOHMS		10
#define ADC_LT6106_OFFSET_CURRENT_UA		25000 // 250µV maximum / 10mR = 25mA.

#define ADC_TIMEOUT_COUNT					1000000

/*** ADC local structures ***/

typedef struct {
	unsigned int vrefint_12bits;
	unsigned int data[ADC_DATA_IDX_MAX];
} ADC_context_t;

/*** ADC local global variables ***/

static ADC_context_t adc_ctx;

/*** ADC local functions ***/

/* PERFORM A SINGLE ADC CONVERSION.
 * @param adc_channel:			Channel to convert.
 * @param adc_result_12bits:	Pointer to int that will contain ADC raw result on 12 bits.
 * @return:						None.
 */
static void ADC1_single_conversion(unsigned char adc_channel, unsigned int* adc_result_12bits) {
	// Select input channel.
	ADC1 -> CHSELR &= 0xFFF80000; // Reset all bits.
	ADC1 -> CHSELR |= (0b1 << adc_channel);
	// Clear all flags.
	ADC1 -> ISR |= 0x0000089F;
	// Read raw supply voltage.
	ADC1 -> CR |= (0b1 << 2); // ADSTART='1'.
	unsigned int loop_count = 0;
	while (((ADC1 -> ISR) & (0b1 << 2)) == 0) {
		// Wait end of conversion ('EOC='1') or timeout.
		loop_count++;
		if (loop_count > ADC_TIMEOUT_COUNT) return;
	}
	(*adc_result_12bits) = (ADC1 -> DR);
}

/* PERFORM SEVERAL CONVERSIONS FOLLOWED BY A MEDIAN FIILTER.
 * @param adc_channel:			Channel to convert.
 * @param adc_result_12bits:	Pointer to int that will contain ADC filtered result on 12 bits.
 * @return:						None.
 */
static void ADC1_filtered_conversion(unsigned char adc_channel, unsigned int* adc_result_12bits) {
	// Perform all conversions.
	unsigned int adc_sample_buf[ADC_MEDIAN_FILTER_LENGTH] = {0x00};
	unsigned char idx = 0;
	for (idx=0 ; idx<ADC_MEDIAN_FILTER_LENGTH ; idx++) {
		ADC1_single_conversion(adc_channel, &(adc_sample_buf[idx]));
	}
	// Apply median filter.
	(*adc_result_12bits) = MATH_median_filter(adc_sample_buf, ADC_MEDIAN_FILTER_LENGTH, ADC_CENTER_AVERAGE_LENGTH);
}

/* COMPUTE INPUT VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_compute_vin(void) {
	// Get raw result.
	unsigned int vin_12bits = 0;
	ADC1_filtered_conversion(ADC_CHANNEL_VIN, &vin_12bits);
	// Convert to mV using bandgap result.
	adc_ctx.data[ADC_DATA_IDX_VIN_MV] = (ADC_VREFINT_VOLTAGE_MV * vin_12bits * ADC_VOLTAGE_DIVIDER_RATIO_VIN) / (adc_ctx.vrefint_12bits);
}

/* COMPUTE OUTPUT VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_compute_vout(void) {
	// Get raw result.
	unsigned int vout_12bits = 0;
	ADC1_filtered_conversion(ADC_CHANNEL_VOUT, &vout_12bits);
	// Convert to mV using bandgap result.
	adc_ctx.data[ADC_DATA_IDX_VOUT_MV] = (ADC_VREFINT_VOLTAGE_MV * vout_12bits * ADC_VOLTAGE_DIVIDER_RATIO_VOUT) / (adc_ctx.vrefint_12bits);
}

/* COMPUTE OUTPUT CURRENT.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_compute_iout(void) {
	// Get raw result.
	unsigned int iout_12bits = 0;
	ADC1_filtered_conversion(ADC_CHANNEL_IOUT, &iout_12bits);
	// Convert to uA using bandgap result.
	unsigned long long num = iout_12bits;
	num *= ADC_VREFINT_VOLTAGE_MV;
	num *= 1000000;
	unsigned long long den = adc_ctx.vrefint_12bits;
	den *= ADC_LT6106_VOLTAGE_GAIN;
	den *= ADC_LT6106_SHUNT_RESISTOR_MOHMS;
	adc_ctx.data[ADC_DATA_IDX_IOUT_UA] = (num) / (den);
	// Remove offset current.
	if (adc_ctx.data[ADC_DATA_IDX_IOUT_UA] < ADC_LT6106_OFFSET_CURRENT_UA) {
		adc_ctx.data[ADC_DATA_IDX_IOUT_UA] = 0;
	}
	else {
		adc_ctx.data[ADC_DATA_IDX_IOUT_UA] -= ADC_LT6106_OFFSET_CURRENT_UA;
	}
}

/* COMPUTE MCU SUPPLY VOLTAGE.
 * @param:	None.
 * @return:	None.
 */
static void ADC1_compute_vmcu(void) {
	// Retrieve supply voltage from bandgap result.
	adc_ctx.data[ADC_DATA_IDX_VMCU_MV] = (VREFINT_CAL * VREFINT_VCC_CALIB_MV) / (adc_ctx.vrefint_12bits);
}

/*** ADC functions ***/

/* INIT ADC1 PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void ADC1_init(void) {
	// Init GPIOs.
	GPIO_configure(&GPIO_ADC1_IN0, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_ADC1_IN4, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	GPIO_configure(&GPIO_ADC1_IN6, GPIO_MODE_ANALOG, GPIO_TYPE_OPEN_DRAIN, GPIO_SPEED_LOW, GPIO_PULL_NONE);
	// Init context.
	adc_ctx.vrefint_12bits = 0;
	unsigned char data_idx = 0;
	for (data_idx=0 ; data_idx<ADC_DATA_IDX_MAX ; data_idx++) adc_ctx.data[data_idx] = 0;
	adc_ctx.data[ADC_DATA_IDX_VMCU_MV] = ADC_VMCU_DEFAULT_MV;
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 9); // ADCEN='1'.
	// Ensure ADC is disabled.
	if (((ADC1 -> CR) & (0b1 << 0)) != 0) {
		ADC1 -> CR |= (0b1 << 1); // ADDIS='1'.
	}
	// Enable ADC voltage regulator.
	ADC1 -> CR |= (0b1 << 28);
	LPTIM1_delay_milliseconds(5);
	// ADC configuration.
	ADC1 -> CCR |= (0b1 << 25); // Enable low frequency clock (LFMEN='1').
	ADC1 -> CFGR2 |= (0b11 << 30); // Use PCLK2 as ADCCLK (MSI).
	ADC1 -> SMPR |= (0b111 << 0); // Maximum sampling time.
	// ADC calibration.
	ADC1 -> CR |= (0b1 << 31); // ADCAL='1'.
	unsigned int loop_count = 0;
	while ((((ADC1 -> CR) & (0b1 << 31)) != 0) && (((ADC1 -> ISR) & (0b1 << 11)) == 0)) {
		// Wait until calibration is done or timeout.
		loop_count++;
		if (loop_count > ADC_TIMEOUT_COUNT) break;
	}
	// Disable peripheral by default.
	RCC -> APB2ENR &= ~(0b1 << 9); // ADCEN='0'.
}

/* ENABLE INTERNAL ADC PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void ADC1_enable(void) {
	// Enable peripheral clock.
	RCC -> APB2ENR |= (0b1 << 9); // ADCEN='1'.
}

/* DISABLE INTERNAL ADC PERIPHERAL.
 * @param:	None.
 * @return:	None.
 */
void ADC1_disable(void) {
	// Disable peripheral clock.
	RCC -> APB2ENR &= ~(0b1 << 9); // ADCEN='0'.
}

/* PERFORM INTERNAL ADC MEASUREMENTS.
 * @param:	None.
 * @return:	None.
 */
void ADC1_perform_measurements(void) {
	// Enable ADC peripheral.
	ADC1 -> CR |= (0b1 << 0); // ADEN='1'.
	unsigned int loop_count = 0;
	while (((ADC1 -> ISR) & (0b1 << 0)) == 0) {
		// Wait for ADC to be ready (ADRDY='1') or timeout.
		loop_count++;
		if (loop_count > ADC_TIMEOUT_COUNT) return;
	}
	// Wake-up VREFINT.
	ADC1 -> CCR |= (0b1 << 22); //  VREFEF='1'.
	LPTIM1_delay_milliseconds(10); // Wait internal reference stabilization (max 3ms).
	// Perform measurements.
	ADC1_filtered_conversion(ADC_CHANNEL_VREFINT, &adc_ctx.vrefint_12bits);
	ADC1_compute_vin();
	ADC1_compute_vout();
	ADC1_compute_iout();
	ADC1_compute_vmcu();
	// Turn VREFINT off.
	ADC1 -> CCR &= ~(0b1 << 22); // VREFEF='0'.
	// Disable ADC peripheral.
	if (((ADC1 -> CR) & (0b1 << 0)) != 0) {
		ADC1 -> CR |= (0b1 << 1); // ADDIS='1'.
	}
}

/* GET ADC DATA.
 * @param data_idx:		Index of the data to retrieve.
 * @param data:				Pointer that will contain ADC data.
 * @return:					None.
 */
void ADC1_get_data(ADC_data_index_t data_idx, unsigned int* data) {
	(*data) = adc_ctx.data[data_idx];
}
