#include <rcc.h>
#include <registers.h>
#include <sleep.h>
#include "spi.h"

void spi_init(void) {
	// Reset and enable the SPI module.
	rcc_enable(APB2, 12);

	// Configure the SPI module for the target.
	SPI1_CR2 = 0 // Motorola frame format
		| 0 // SPOE = 0; hardware chip select output is disabled
		| 0 // TXDMAEN = 0; no transmit DMA
		| 0; // RXDMAEN = 0; no receive DMA
	SPI1_CR1 = 0 // BIDIMODE = 0; 2-line unidirectional mode
		| 0 // BIDIOE = 0; ignored in unidirectional mode
		| 0 // CRCEN = 0; hardware CRC calculation disabled
		| 0 // CRCNEXT = 0; next transfer is not a CRC
		| 0 // DFF = 0; data frames are 8 bits wide
		| 0 // RXONLY = 0; module both transmits and receives
		| SSM // Software slave select management selected
		| SSI // Assume slave select is deasserted (no other master is transmitting)
		| 0 // LSBFIRST = 0; data is transferred MSb first
		| SPE // Module is enabled
		| BR(0) // Transmission speed is 84 MHz (APB2) ÷ 2 = 42 MHz
		| MSTR // Master mode
		| 0 // CPOL = 0; clock idles low
		| 0; // CPHA = 0; capture is on rising (first) edge while advance is on falling (second) edge

	// Reset and enable the SPI module.
	rcc_enable(APB1, 15);

	// Configure the SPI module for the onboard memory.
	SPI3_CR2 = 0 // Motorola frame format
		| 0 // SPOE = 0; hardware chip select output is disabled
		| 0 // TXDMAEN = 0; no transmit DMA
		| 0; // RXDMAEN = 0; no receive DMA
	SPI3_CR1 = 0 // BIDIMODE = 0; 2-line unidirectional mode
		| 0 // BIDIOE = 0; ignored in unidirectional mode
		| 0 // CRCEN = 0; hardware CRC calculation disabled
		| 0 // CRCNEXT = 0; next transfer is not a CRC
		| 0 // DFF = 0; data frames are 8 bits wide
		| 0 // RXONLY = 0; module both transmits and receives
		| SSM // Software slave select management selected
		| SSI // Assume slave select is deasserted (no other master is transmitting)
		| 0 // LSBFIRST = 0; data is transferred MSb first
		| SPE // Module is enabled
		| BR(0) // Transmission speed is 42 MHz (APB1) ÷ 2 = 21 MHz
		| MSTR // Master mode
		| 0 // CPOL = 0; clock idles low
		| 0; // CPHA = 0; capture is on rising (first) edge while advance is on falling (second) edge
}

static inline void sleep_50ns(void) {
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
}

static void int_drive_bus(void) {
}

static void int_assert_cs(void) {
	sleep_50ns();
	GPIOA_BSRR = GPIO_BR(4);
	sleep_50ns();
}

static void int_deassert_cs(void) {
	while (SPI3_SR & SPI_BSY); // Wait until no activity
	sleep_50ns();
	GPIOA_BSRR = GPIO_BS(4);
}

static uint8_t int_transceive_byte(uint8_t byte) {
	while (!(SPI3_SR & SPI_TXE)); // Wait for send buffer space
	SPI3_DR = byte;
	while (!(SPI3_SR & SPI_RXNE)); // Wait for inbound byte to be received
	return SPI3_DR;
}

static void int_read_bytes(void *buffer, size_t length) {
	uint8_t *pch = buffer;
	size_t sent = 0, received = 0;
	while (sent < length || received < length) {
		if (received < length && (SPI3_SR & SPI_RXNE)) {
			*pch++ = SPI3_DR;
			++received;
		}
		if (sent < length && (SPI3_SR & SPI_TXE)) {
			SPI3_DR = 0;
			++sent;
		}
	}
}

static void int_write_bytes(const void *buffer, size_t length) {
	size_t sent = 0, received = 0;
	const uint8_t *pch = buffer;
	while (sent < length || received < length) {
		if (received < length && (SPI3_SR & SPI_RXNE)) {
			(void) SPI3_DR;
			++received;
		}
		if (sent < length && (SPI3_SR & SPI_TXE)) {
			SPI3_DR = *pch++;
			++sent;
		}
	}
}

const struct spi_ops spi_internal_ops = {
	.drive_bus = &int_drive_bus,
	.assert_cs = &int_assert_cs,
	.deassert_cs = &int_deassert_cs,
	.transceive_byte = &int_transceive_byte,
	.read_bytes = &int_read_bytes,
	.write_bytes = &int_write_bytes,
};

static void ext_drive_bus(void) {
	// Switch the MOSI (PB5), MISO (PB4), and Clock (PB3) into alternate function mode.
	GPIOB_MODER = (GPIOB_MODER & ~(0b111111 << (3 * 2))) | (0b101010 << (3 * 2));

	// Switch /CS (PA15) to output high.
	GPIOA_BSRR = GPIO_BS(15);
	GPIOA_MODER = (GPIOA_MODER & ~(0b11 << (15 * 2))) | (0b01 << (15 * 2));
}

static void ext_assert_cs(void) {
	sleep_50ns();
	GPIOA_BSRR = GPIO_BR(15);
	sleep_50ns();
}

static void ext_deassert_cs(void) {
	while (SPI1_SR & SPI_BSY); // Wait until no activity
	sleep_50ns();
	GPIOA_BSRR = GPIO_BS(15);
}

static uint8_t ext_transceive_byte(uint8_t byte) {
	while (!(SPI1_SR & SPI_TXE)); // Wait for send buffer space
	SPI1_DR = byte;
	while (!(SPI1_SR & SPI_RXNE)); // Wait for inbound byte to be received
	return SPI1_DR;
}

static void ext_read_bytes(void *buffer, size_t length) {
	uint8_t *pch = buffer;
	size_t sent = 0, received = 0;
	while (sent < length || received < length) {
		if (received < length && (SPI1_SR & SPI_RXNE)) {
			*pch++ = SPI1_DR;
			++received;
		}
		if (sent < length && (SPI1_SR & SPI_TXE)) {
			SPI1_DR = 0;
			++sent;
		}
	}
}

static void ext_write_bytes(const void *buffer, size_t length) {
	size_t sent = 0, received = 0;
	const uint8_t *pch = buffer;
	while (sent < length || received < length) {
		if (received < length && (SPI1_SR & SPI_RXNE)) {
			(void) SPI1_DR;
			++received;
		}
		if (sent < length && (SPI1_SR & SPI_TXE)) {
			SPI1_DR = *pch++;
			++sent;
		}
	}
}

const struct spi_ops spi_external_ops = {
	.drive_bus = &ext_drive_bus,
	.assert_cs = &ext_assert_cs,
	.deassert_cs = &ext_deassert_cs,
	.transceive_byte = &ext_transceive_byte,
	.read_bytes = &ext_read_bytes,
	.write_bytes = &ext_write_bytes,
};

