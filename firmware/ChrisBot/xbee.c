#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "constants.h"
#include "xbee.h"
#include "rtc.h"
#include "debug.h"

#define XBEE_DEBUG 1
#define XBEE_MAX_PACKET 32

static void memcpy_volatile_to_volatile(volatile void *dest, const volatile void *src, uint8_t len) __attribute__((__always_inline__));
static void memcpy_volatile_to_volatile(volatile void *dest, const volatile void *src, uint8_t len) {
	volatile char *dptr = dest;
	const volatile char *sptr = src;
	while (len--)
		*dptr++ = *sptr++;
}

static void memcpy_to_volatile(volatile void *dest, const void *src, uint8_t len) __attribute__((__always_inline__));
static void memcpy_to_volatile(volatile void *dest, const void *src, uint8_t len) {
	volatile char *dptr = dest;
	const char *sptr = src;
	while (len--)
		*dptr++ = *sptr++;
}

static void memcpy_from_volatile(void *dest, const volatile void *src, uint8_t len) __attribute__((__always_inline__));
static void memcpy_from_volatile(void *dest, const volatile void *src, uint8_t len) {
	char *dptr = dest;
	const volatile char *sptr = src;
	while (len--)
		*dptr++ = *sptr++;
}

unsigned long xbee_rxtimestamp;
struct xbee_rxdata xbee_rxdata = {
	.emergency = 0xFF,
};
struct xbee_txdata xbee_txdata;

static volatile uint8_t host_address[8];

static uint8_t rxbuf[XBEE_MAX_PACKET];
static uint8_t rxptr;
static volatile struct xbee_rxdata rxdata_dbuf;
static volatile unsigned long rxtimestamp_dbuf;
ISR(USART1_RX_vect, ISR_BLOCK) {
	uint8_t ch, i;

	// Check for framing or overrun error.
	if (UCSR1A & (_BV(FE1) | _BV(DOR1))) {
		ch = UDR1;
		rxptr = 0;
		return;
	}
	
	// Read received data.
	ch = UDR1;

	// Packet must start with delimiter (try to resync if corrupted).
	if (rxptr == 0 && ch != 0x7E) {
		return;
	}

	// MSB of length must be zero (we don't handle big packets).
	if (rxptr == 1 && ch != 0) {
		rxptr = 0;
		return;
	}

	// LSB of length must be small enough.
	if (rxptr == 2 && ch > XBEE_MAX_PACKET - 4) {
		rxptr = 0;
		return;
	}

	// Stash the byte.
	rxbuf[rxptr++] = ch;

	// Check if we've received a full packet.
	if (rxptr != rxbuf[2] + 4) {
		return;
	}

	// Check if the packet has a valid checksum.
	ch = 0;
	i = rxbuf[2] + 1;
	while (i--) {
		ch += rxbuf[3 + i];
	}
	if (ch != 0xFF) {
		rxptr = 0;
		return;
	}

	// Deal with the packet.
	if (rxbuf[3] == 0x80) {
		// Receive data packet.
		if (rxbuf[2] == 1 + 8 + 1 + 1 + sizeof(struct xbee_rxdata)) {
			// Correctly-sized data packet. Stuff into double buffer.
			memcpy_to_volatile(&host_address[0], &rxbuf[4], sizeof(host_address));
			memcpy_to_volatile(&rxdata_dbuf, &rxbuf[4 + 8 + 1 + 1], sizeof(rxdata_dbuf));
			rxtimestamp_dbuf = rtc_millis();
		}
	}

	// Clear the buffer.
	rxptr = 0;
}

static volatile uint8_t txbuf[XBEE_MAX_PACKET];
static volatile uint8_t txptr, txlen;
ISR(USART1_UDRE_vect, ISR_BLOCK) {
	if (txptr < txlen) {
		UDR1 = txbuf[txptr++];
	} else {
		UCSR1B &= ~_BV(UDRIE1);
	}
}

static uint8_t init_read_ok(void) {
	unsigned long start = rtc_millis();
	uint8_t seen = 0;
	char ch;

	while (rtc_millis() - start < 1000) {
		if (UCSR1A & _BV(RXC1)) {
			ch = UDR1;
			if (seen == 0 && ch == 'O') seen = 1;
			else if (seen == 1 && ch == 'K') seen = 2;
			else if (seen == 2 && ch == '\r') return 1;
			else seen = 0;
		}
	}
	return 0;
}

static void init_write(const char *data) {
	while (*data) {
		while (!(UCSR1A & _BV(UDRE1)));
		UDR1 = *data++;
	}
}

static void init_command(const char *cmd) {
#if XBEE_DEBUG
	debug_puts("XBee: ");
	debug_puts(cmd);
#endif
	do {
		init_write(cmd);
		init_write("\r");
	} while (!init_read_ok());
	debug_puts(" OK.\n");
}

void xbee_init(void) {
#define XBEE_BAUD_DIVISOR (((F_CPU + 8UL * XBEE_BAUD) / (16UL * XBEE_BAUD)) - 1UL)
	UBRR1H = XBEE_BAUD_DIVISOR / 256UL;
	UBRR1L = XBEE_BAUD_DIVISOR % 256UL;
	UCSR1A = 0;
	UCSR1B = _BV(RXEN1) | _BV(TXEN1);
	UCSR1C = _BV(USBS1) | _BV(UCSZ11) | _BV(UCSZ10);

	// Send +++, receive OK.
#if XBEE_DEBUG
	debug_puts("XBee: +++\n");
#endif
	do {
		_delay_ms(1100);
		init_write("+++");
		_delay_ms(1100);
	} while (!init_read_ok());

	// Factory reset.
	init_command("ATRE");

	// Non-escaped API mode.
	init_command("ATAP1");

	// Power level.
	init_command("ATPL" XBEE_POWER_LEVEL);

	// Channel.
	init_command("ATCH" XBEE_CHANNEL);

	// PAN.
	init_command("ATID" XBEE_PAN);

	// Local address (none).
	init_command("ATMYFFFF");

	// Exit command mode.
	init_command("ATCN");
}

void xbee_receive(void) {
	cli();
	memcpy_from_volatile(&xbee_rxdata, &rxdata_dbuf, sizeof(xbee_rxdata));
	xbee_rxtimestamp = rxtimestamp_dbuf;
	sei();
}

void xbee_send(void) {
	uint8_t checksum, i;

	if (txptr == txlen && xbee_rxtimestamp) {
		txbuf[0] = 0x7E;
		txbuf[1] = 0;
		txbuf[2] = 1 + 1 + 8 + 1 + sizeof(xbee_txdata);
		txbuf[3] = 0x00;
		txbuf[4] = 0x00;
		memcpy_volatile_to_volatile(&txbuf[5], &host_address[0], sizeof(host_address));
		txbuf[13] = 0x01;
		memcpy_to_volatile(&txbuf[14], &xbee_txdata, sizeof(xbee_txdata));
		checksum = 0xFF;
		for (i = 3; i < 3 + 1 + 1 + 8 + 1 + sizeof(xbee_txdata); i++)
			checksum -= txbuf[i];
		txbuf[14 + sizeof(xbee_txdata)] = checksum;
		cli();
		txptr = 0;
		txlen = 14 + sizeof(xbee_txdata) + 1;
		sei();
	}
}

