#ifndef CPUUSAGE_H
#define CPUUSAGE_H

#include "led.h"
#include "iopins.h"

#define CPU_INUSE()                     \
	do {                                \
		led_on();                       \
		iopin_write(IOPIN_CPU_BUSY, 1); \
	} while (0)

#define CPU_IDLE()                      \
	do {                                \
		led_off();                      \
		iopin_write(IOPIN_CPU_BUSY, 0); \
	} while (0)

#endif

