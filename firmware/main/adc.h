#ifndef ADC_H
#define ADC_H

#include "io.h"
#include <stdint.h>

typedef enum {
	CHICKER = 0,
	BATT_VOLT = 1,
	TEMPERATURE = 5,
	LPS = 6,
	BREAKBEAM = 7,
} adc_t;

static inline uint16_t read_main_adc(adc_t adc_index) {
	uint16_t ret_val;
	ADC_LSB = adc_index;
	ret_val = ADC_MSB;
	ret_val = (ret_val << 8) | ADC_LSB;
	return ret_val;
}

static inline int16_t read_breakbeam_diff(void) {
	int16_t diff;
	BREAKBEAM_DIFF_L = 0;
	diff = BREAKBEAM_DIFF_H;
	diff = (diff << 8) | BREAKBEAM_DIFF_L;
	return diff;
}

#endif

