#include "sleep.h"
#include "io.h"

void sleep_short(void) {
	uint8_t old = inb(TICKS);
	while (inb(TICKS) == old);
	old = inb(TICKS);
	while (inb(TICKS) == old);
}

void sleep_1s(void) {
	uint8_t x = 200;
	while (--x) {
		uint8_t old = inb(TICKS);
		while (inb(TICKS) == old);
	}
}

