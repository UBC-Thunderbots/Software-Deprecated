#ifndef ADCPINS_H
#define ADCPINS_H

#include <avr/interrupt.h>

/*
 * Initializes the ADC subsystem.
 */
void adc_init(void);

/*
 * DO NOT USE THIS VARIABLE!
 */
extern volatile uint16_t adc_results[8];

/*
 * Reads a value from the ADC.
 */
static uint16_t adc_read(uint8_t pin) __attribute__((__always_inline__, __unused__));
static uint16_t adc_read(uint8_t pin) {
	uint8_t oldsreg;
	uint16_t ret;

	oldsreg = SREG;
	cli();
	ret = adc_results[pin];
	SREG = oldsreg;
	return ret;
}

#endif

