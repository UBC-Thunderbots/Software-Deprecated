/**
 * \defgroup ICB Inter-Chip Bus Functions
 *
 * \brief These functions manage the inter-chip bus between microcontroller and FPGA.
 *
 * The bus is used in two different ways at different times.
 * During system boot, the ICB is used to deliver a configuration bitstream to the FPGA.
 * Once the FPGA is configured, the ICB is used to communicate with the logic in the FPGA.
 *
 * In addition to the basic SPI bus which runs between the two chips, there is also an interrupt wire, used during normal ICB operation.
 * An interrupt controller in the FPGA records and latches a set of edge-sensitive interrupts generated by different subsystems.
 * The current set of pending interrupts can be atomically read and cleared using an ordinary command over the bus.
 * Additionally, whenever the pending interrupt set is nonempty, the separate interrupt wire is driven high.
 * An internal task in this module is notified when this happens, issues the read-and-clear command, and distributes the resulting interrupt sources to their individual handlers.
 *
 * @{
 */

#include "icb.h"
#include "dma.h"
#include "error.h"
#include "pins.h"
#include "priority.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <crc32.h>
#include <exception.h>
#include <gpio.h>
#include <minmax.h>
#include <nvic.h>
#include <portmacro.h>
#include <rcc.h>
#include <semphr.h>
#include <string.h>
#include <task.h>
#include <unused.h>
#include <registers/dma.h>
#include <registers/exti.h>
#include <registers/spi.h>
#include <registers/syscfg.h>

/**
 * \internal
 *
 * \brief The DMA stream number for the SPI receive path.
 */
#define DMA_STREAM_RX 0U

/**
 * \internal
 *
 * \brief The DMA stream number for the SPI transmit path.
 */
#define DMA_STREAM_TX 3U

/**
 * \internal
 *
 * \brief The DMA channel number.
 */
#define DMA_CHANNEL 3U

/**
 * \internal
 *
 * \brief The interrupt number for the receive DMA interrupt.
 */
#define IRQ_RX_DMA NVIC_IRQ_DMA2_STREAM0

/**
 * \internal
 *
 * \brief The interrupt number for the transmit DMA interrupt.
 */
#define IRQ_TX_DMA NVIC_IRQ_DMA2_STREAM3

/**
 * \brief The possible states the bus can be in.
 */
typedef enum {
	/**
	 * \brief No ICB activity is occurring.
	 */
	ICB_STATE_IDLE,

	/**
	 * \brief The command byte or parameter block of an OUT ICB transaction is being sent.
	 */
	ICB_STATE_OUT_HEADER_DATA,

	/**
	 * \brief The CRC32 of an OUT ICB transaction is being sent.
	 */
	ICB_STATE_OUT_CRC,

	/**
	 * \brief The command byte, CRC32, and padding of an IN ICB transaction is being sent.
	 */
	ICB_STATE_IN_HEADER,

	/**
	 * \brief The parameter block of an IN ICB transaction is being received.
	 */
	ICB_STATE_IN_DATA,

	/**
	 * \brief The CRC32 of the parameter block of an IN ICB transaction is being received.
	 */
	ICB_STATE_IN_CRC,
} icb_state_t;

/**
 * \brief A byte that is always zero.
 */
static const uint8_t ZERO = 0U;

/**
 * \brief A mutex protecting the bus from multiple simultaneous accesses.
 */
static SemaphoreHandle_t bus_mutex;

/**
 * \brief The current state of the bus.
 */
static icb_state_t state = ICB_STATE_IDLE;

/**
 * \brief The CRC32 of the current phase of the current transaction.
 *
 * For OUT transactions, this is the CRC32 of the command byte and parameter block.
 * For IN transactions in ICB_STATE_IN_HEADER, this is the CRC32 of the command byte.
 * For IN transactions after ICB_STATE_IN_CRC, this is where the CRC is received.
 */
static uint32_t crc;

/**
 * \brief A small buffer in which data can be collected.
 */
static uint8_t temp_buffer[6U];

/**
 * \brief A semaphore used by the ISRs to notify when a transaction is complete.
 */
static SemaphoreHandle_t transaction_complete_sem;

/**
 * \brief A semaphore handle used by the EXTI ISR to notify the interrupt dispatcher task.
 */
static SemaphoreHandle_t irq_sem;

/**
 * \brief The semaphores to notify when ICB IRQs are asserted.
 */
static void (*irq_handlers[ICB_IRQ_COUNT])(void);

/**
 * \brief Takes the simultaneous-access mutex.
 */
static void lock_bus(void) {
	xSemaphoreTake(bus_mutex, portMAX_DELAY);
}

/**
 * \brief Releases the simultaneous-access mutex.
 */
static void unlock_bus(void) {
	xSemaphoreGive(bus_mutex);
}

/**
 * \brief Delays for roughly a bit time.
 */
#define sleep_bit(void) \
	do { \
		asm volatile("nop":::); \
		asm volatile("nop":::); \
		asm volatile("nop":::); \
		asm volatile("nop":::); \
	} while (0)

/**
 * \brief Enables the SPI module.
 *
 * This includes locking the bus and asserting chip select.
 */
void enable_spi(void) {
	// Prevent simultaneous bus access.
	lock_bus();

	// Sanity check.
	assert(state == ICB_STATE_IDLE);

	// Enable the SPI module.
	SPI_CR1_t cr1 = {
		.CPHA = 0, // Capture on first clock transition, drive new data on second.
		.CPOL = 0, // Clock idles low.
		.MSTR = 1, // Master mode.
		.BR = 0, // Transmission speed is 84 MHz (APB2) ÷ 2 = 42 MHz.
		.SPE = 1, // SPI module now enabled.
		.LSBFIRST = 0, // Most significant bit is sent first.
		.SSI = 1, // Module should assume slave select is high → deasserted → no other master is using the bus.
		.SSM = 1, // Module internal slave select logic is controlled by software (SSI bit).
		.RXONLY = 0, // Transmit and receive.
		.DFF = 0, // Frames are 8 bits wide.
		.CRCNEXT = 0, // Do not transmit a CRC now.
		.CRCEN = 0, // CRC calculation not used.
		.BIDIMODE = 0, // 2-line bidirectional communication used.
	};
	SPI1.CR1 = cr1;

	// Allow the clock pin to settle.
	sleep_bit();

	// Assert chip select.
	gpio_reset(PIN_ICB_CS);

	// Allow chip select to settle.
	sleep_bit();
}

/**
 * \brief Disables the SPI module.
 *
 * This includes deasserting chip select and unlocking the bus.
 */
void disable_spi(void) {
	// Wait for bus idle.
	while (!SPI1.SR.TXE);
	while (SPI1.SR.BSY);

	// Wait for final required interval.
	sleep_bit();

	// Deassert chip select.
	gpio_set(PIN_ICB_CS);

	// Allow chip select to settle.
	sleep_bit();

	// Disable transmit DMA.
	SPI_CR2_t cr2 = { 0 };
	SPI1.CR2 = cr2;

	// Disable the SPI module.
	SPI_CR1_t cr1 = { 0 };
	SPI1.CR1 = cr1;

	// Sanity check.
	assert(state == ICB_STATE_IDLE);

	// Allow subsequent bus accesses to other tasks.
	unlock_bus();
}

/**
 * \brief Configures the transmit DMA channel.
 *
 * \param[in] data the data to send
 * \param[in] length the number of bytes
 * \param[in] unmask_tcie whether or not to unmask transfer complete interrupts
 * \param[in] minc whether or not to enable memory address incrementing
 */
void config_tx_dma(const void *data, size_t length, bool unmask_tcie, bool minc) {
	// Clear old interrupts.
	_Static_assert(DMA_STREAM_TX == 3U, "LIFCR needs rewriting to the proper stream number!");
	DMA_LIFCR_t lifcr = {
		.CFEIF3 = 1U, // Clear FIFO error interrupt flag.
		.CDMEIF3 = 1U, // Clear direct mode error interrupt flag.
		.CTEIF3 = 1U, // Clear transfer error interrupt flag.
		.CHTIF3 = 1U, // Clear half transfer interrupt flag.
		.CTCIF3 = 1U, // Clear transfer complete interrupt flag.
	};
	DMA2.LIFCR = lifcr;

	// Ensure all memory writes have reached memory before the DMA is enabled.
	__atomic_thread_fence(__ATOMIC_RELEASE);

	// Enable the FIFO.
	DMA_SxFCR_t fcr = {
		.FTH = DMA_FIFO_THRESHOLD_HALF, // Threshold.
		.DMDIS = 1, // Use the FIFO.
	};
	DMA2.streams[DMA_STREAM_TX].FCR = fcr;

	// Set up the memory addresses and length.
	DMA2.streams[DMA_STREAM_TX].PAR = &SPI1.DR;
	DMA2.streams[DMA_STREAM_TX].M0AR = (void*) data; // Casting away constness is safe because this DMA stream will operate in memory-to-peripheral mode.
	DMA2.streams[DMA_STREAM_TX].NDTR = length;

	// Enable the channel.
	DMA_SxCR_t scr = {
		.EN = 1, // Enable DMA engine.
		.DMEIE = 1, // Enable direct mode error interrupt.
		.TEIE = 1, // Enable transfer error interrupt.
		.TCIE = unmask_tcie, // Enable or dissable transfer complete interrupt.
		.PFCTRL = 0, // DMA engine controls data length.
		.DIR = DMA_DIR_M2P,
		.CIRC = 0, // No circular buffer mode.
		.PINC = 0, // Do not increment peripheral address.
		.MINC = minc, // Increment or not memory address.
		.PSIZE = DMA_DSIZE_BYTE,
		.MSIZE = DMA_DSIZE_BYTE,
		.PINCOS = 0, // No special peripheral address increment mode.
		.PL = 1, // Priority 1 (medium).
		.DBM = 0, // No double-buffer mode.
		.CT = 0, // Use memory pointer zero.
		.PBURST = DMA_BURST_SINGLE,
		.MBURST = DMA_BURST_SINGLE,
		.CHSEL = DMA_CHANNEL,
	};
	DMA2.streams[DMA_STREAM_TX].CR = scr;
}

/**
 * \brief Configures the receive DMA channel.
 *
 * \param[in] buffer the data buffer to receive into
 * \param[in] length the number of bytes
 */
void config_rx_dma(void *buffer, size_t length) {
	// Clear old interrupts.
	_Static_assert(DMA_STREAM_RX == 0U, "LIFCR needs rewriting to the proper stream number!");
	DMA_LIFCR_t lifcr = {
		.CFEIF0 = 1U, // Clear FIFO error interrupt flag.
		.CDMEIF0 = 1U, // Clear direct mode error interrupt flag.
		.CTEIF0 = 1U, // Clear transfer error interrupt flag.
		.CHTIF0 = 1U, // Clear half transfer interrupt flag.
		.CTCIF0 = 1U, // Clear transfer complete interrupt flag.
	};
	DMA2.LIFCR = lifcr;

	// Enable the FIFO.
	DMA_SxFCR_t fcr = {
		.FTH = DMA_FIFO_THRESHOLD_HALF, // Threshold.
		.DMDIS = 1, // Use the FIFO.
	};
	DMA2.streams[DMA_STREAM_RX].FCR = fcr;

	// Set up the memory addresses and length.
	DMA2.streams[DMA_STREAM_RX].PAR = &SPI1.DR;
	DMA2.streams[DMA_STREAM_RX].M0AR = buffer;
	DMA2.streams[DMA_STREAM_RX].NDTR = length;

	// Enable the channel.
	DMA_SxCR_t scr = {
		.EN = 1, // Enable DMA engine.
		.DMEIE = 1, // Enable direct mode error interrupt.
		.TEIE = 1, // Enable transfer error interrupt.
		.TCIE = 1, // Enable transfer complete interrupt.
		.PFCTRL = 0, // DMA engine controls data length.
		.DIR = DMA_DIR_P2M,
		.CIRC = 0, // No circular buffer mode.
		.PINC = 0, // Do not increment peripheral address.
		.MINC = 1, // Increment memory address.
		.PSIZE = DMA_DSIZE_BYTE,
		.MSIZE = DMA_DSIZE_BYTE,
		.PINCOS = 0, // No special peripheral address increment mode.
		.PL = 1, // Priority 1 (medium).
		.DBM = 0, // No double-buffer mode.
		.CT = 0, // Use memory pointer zero.
		.PBURST = DMA_BURST_SINGLE,
		.MBURST = DMA_BURST_SINGLE,
		.CHSEL = DMA_CHANNEL,
	};
	DMA2.streams[DMA_STREAM_RX].CR = scr;
}

/**
 * \brief Starts a DMA transfer.
 *
 * \param[in] tx whether to enable transmit DMA
 * \param[in] rx whether to enable receive DMA
 */
void start_dma(bool tx, bool rx) {
	SPI_CR2_t cr2 = {
		.TXDMAEN = tx,
		.RXDMAEN = rx,
	};
	SPI1.CR2 = cr2;
}

/**
 * \brief Disables DMA operation in the SPI engine.
 */
void stop_dma(void) {
	SPI_CR2_t cr2 = {
		.TXDMAEN = 0,
		.RXDMAEN = 0,
	};
	SPI1.CR2 = cr2;
}

/**
 * \name Module initialization
 *
 * @{
 */

/**
 * \brief Initializes the ICB.
 */
void icb_init(void) {
	// Create the FreeRTOS objects.
	bus_mutex = xSemaphoreCreateMutex();
	transaction_complete_sem = xSemaphoreCreateBinary();
	irq_sem = xSemaphoreCreateBinary();
	assert(bus_mutex && transaction_complete_sem && irq_sem);

	// Enable clock and reset module.
	rcc_enable_reset(APB2, SPI1);

	// Enable the EXTI0 interrupt.
	// Other interrupts will be enabled as needed.
	portENABLE_HW_INTERRUPT(NVIC_IRQ_EXTI0);
}

/**
 * @}
 */

/**
 * \name Normal communication
 *
 * These functions are used to communicate with logic in a fully configured FPGA.
 *
 * @{
 */

/**
 * \internal
 *
 * \brief Executes a microcontroller-to-FPGA ICB transaction with a parameter block.
 *
 * \param[in] command the command to request
 * \param[in] data the data block to send after the command byte as a parameter
 * \param[in] length the number of bytes in the data block to send
 */
static void icb_send_param(icb_command_t command, const void *data, size_t length) {
	// Sanity check.
	assert(!(command & 0x80U));
	assert(dma_check(data, length));

	// Enable the bus.
	enable_spi();

	// Send the command byte.
	SPI1.DR = command;

	// Enable transmit DMA, which will take over with the parameter bytes once the command byte is done.
	config_tx_dma(data, length, true, true);
	start_dma(true, false);

	// Update state.
	state = ICB_STATE_OUT_HEADER_DATA;

	// Compute the CRC32 of the command byte and parameter block.
	uint8_t command_byte = command;
	crc = __builtin_bswap32(crc32_be(data, length, crc32_be(&command_byte, 1U, CRC32_EMPTY)));

	// State and CRC have been locked in. We are now ready for the DMA transfer
	// complete ISR to be run when the parameter block is done. Allow that to
	// happen, but ensure the write to CRC finishes before enabling the
	// interrupt.
	__atomic_thread_fence(__ATOMIC_RELEASE);
	portENABLE_HW_INTERRUPT(IRQ_TX_DMA);

	// Wait for the transaction to finish.
	// This includes the CRC.
	xSemaphoreTake(transaction_complete_sem, portMAX_DELAY);

	// Sanity check.
	assert(state == ICB_STATE_IDLE);

	// Disable DMA interrupts.
	portDISABLE_HW_INTERRUPT(IRQ_TX_DMA);

	// Disable the bus.
	disable_spi();
}

/**
 * \internal
 *
 * \brief Executes a microcontroller-to-FPGA ICB transaction with no parameter block.
 *
 * \param[in] command the command to request
 */
static void icb_send_nullary(icb_command_t command) {
	// Sanity check.
	assert(!(command & 0x80U));

	// Enable the bus.
	enable_spi();

	// Build the five-byte data block to transmit.
	// Byte 0: command
	// Bytes 1 through 4: CRC32
	temp_buffer[0U] = command;
	uint32_t local_crc = crc32_be(temp_buffer, 1U, CRC32_EMPTY);
	local_crc = __builtin_bswap32(local_crc);
	memcpy(&temp_buffer[1U], &local_crc, sizeof(local_crc));

	// Enable transmit DMA.
	config_tx_dma(temp_buffer, 5U, true, true);
	start_dma(true, false);

	// Update state.
	state = ICB_STATE_OUT_CRC;

	// We are now ready for the DMA transfer complete ISR to be run when the command+CRC block is done.
	// Allow that to happen.
	__atomic_signal_fence(__ATOMIC_RELEASE);
	portENABLE_HW_INTERRUPT(IRQ_TX_DMA);

	// Wait for the transaction to finish.
	xSemaphoreTake(transaction_complete_sem, portMAX_DELAY);

	// Sanity check.
	assert(state == ICB_STATE_IDLE);

	// Disable DMA interrupts.
	portDISABLE_HW_INTERRUPT(IRQ_TX_DMA);

	// Disable the bus.
	disable_spi();
}

/**
 * \brief Executes a microcontroller-to-FPGA ICB transaction.
 *
 * \param[in] command the command to request
 * \param[in] data the data block to send after the command byte as a parameter
 * \param[in] length the number of bytes in the data block to send
 */
void icb_send(icb_command_t command, const void *data, size_t length) {
	if (length) {
		icb_send_param(command, data, length);
	} else {
		icb_send_nullary(command);
	}
}

/**
 * \brief Executes an FPGA-to-microcontroller ICB transaction.
 *
 * \param[in] command the command to request
 * \param[out] buffer the buffer into which to receive data
 * \param[in] length the number of bytes to receive
 *
 * \retval true the transaction completed successfully
 * \retval false the transaction failed due to a CRC32 error
 */
bool icb_receive(icb_command_t command, void *buffer, size_t length) {
	// Sanity check.
	assert(command & 0x80U);
	assert(length);
	assert(dma_check(buffer, length));

	// Enable the bus.
	enable_spi();

	// Build the six-byte data block to transmit.
	// Byte 0: command
	// Bytes 1 through 4: CRC32
	// Byte 5: padding
	temp_buffer[0U] = command;
	uint32_t local_crc = crc32_be(temp_buffer, 1U, CRC32_EMPTY);
	local_crc = __builtin_bswap32(local_crc);
	memcpy(&temp_buffer[1U], &local_crc, sizeof(local_crc));
	temp_buffer[5U] = 0U;

	// Enable transmit DMA.
	config_tx_dma(temp_buffer, 6U, true, true);
	start_dma(true, false);

	// Update state.
	state = ICB_STATE_IN_HEADER;

	// We are now ready for the DMA transfer complete ISR to be run when the command+CRC block is done.
	// Allow that to happen.
	__atomic_signal_fence(__ATOMIC_RELEASE);
	portENABLE_HW_INTERRUPT(IRQ_TX_DMA);

	// Wait for the transaction to finish.
	xSemaphoreTake(transaction_complete_sem, portMAX_DELAY);

	// Sanity check.
	assert(state == ICB_STATE_IN_DATA);

	// Disable DMA interrupts.
	portDISABLE_HW_INTERRUPT(IRQ_TX_DMA);

	// Wait for bus idle.
	while (!SPI1.SR.TXE);
	while (SPI1.SR.BSY);

	// Flush received data.
	while (SPI1.SR.RXNE) {
		(void) SPI1.DR;
	}

	// Set up the DMA channels.
	// Transmit will send a string of zeroes while receive will receive the parameter data.
	config_tx_dma(&ZERO, length, false, false);
	config_rx_dma(buffer, length);
	start_dma(true, true);

	// Enable both interrupts.
	// Only receive has transfer complete unmasked.
	// So, any error on either channel will be detected.
	// However, only receive complete will finish the transfer.
	portENABLE_HW_INTERRUPT(IRQ_TX_DMA);
	portENABLE_HW_INTERRUPT(IRQ_RX_DMA);

	// Wait for transaction complete.
	xSemaphoreTake(transaction_complete_sem, portMAX_DELAY);

	// Disable the interrupts.
	portDISABLE_HW_INTERRUPT(IRQ_TX_DMA);
	portDISABLE_HW_INTERRUPT(IRQ_RX_DMA);

	// Sanity check.
	assert(state == ICB_STATE_IDLE);

	// As soon as we get here, the bus should be idle.
	// Otherwise the receive DMA should not have been able to complete.
	assert(SPI1.SR.TXE);
	assert(!SPI1.SR.BSY);

	// Ensure all DMA memory writes have completed before any CPU memory reads start.
	__atomic_thread_fence(__ATOMIC_ACQUIRE);

	// Grab a copy of the received CRC into a local variable to protect it from modification after unlocking the bus mutex.
	local_crc = crc;

	// Compute the CRC of the received data, before disabling the bus so we use the bus mutex to protect against multiple simultaneous accesses to the CRC module.
	uint32_t computed_crc = crc32_be(buffer, length, CRC32_EMPTY);

	// Disable bus.
	disable_spi();

	// Check CRC of received parameter block.
	if (computed_crc == __builtin_bswap32(local_crc)) {
		return true;
	} else {
		error_et_fire(ERROR_ET_ICB_CRC);
		return false;
	}
}

/**
 * @}
 */

/**
 * \name ICB Interrupts
 *
 * These functions handle checking and dispatching interrupts reported from the FPGA.
 *
 * @{
 */

/**
 * \brief The ICB interrupt dispatching task.
 */
static void irq_task(void *UNUSED(param)) {
	for (;;) {
		// If the IRQ pin is low, we have nothing to do.
		while (!gpio_get_input(PIN_ICB_IRQ)) {
			xSemaphoreTake(irq_sem, portMAX_DELAY);
		}

		// Read in the IRQ line state.
		static uint8_t status[(ICB_IRQ_COUNT + 7U) / 8U];
		icb_receive(ICB_COMMAND_GET_CLEAR_IRQS, status, sizeof(status));

		// Dispatch the IRQs.
		for (unsigned int i = 0U; i < ICB_IRQ_COUNT; ++i) {
			if (status[i / 8U] & (1U << (i % 8U))) {
				__atomic_signal_fence(__ATOMIC_ACQUIRE);
				void (*isr)(void) = __atomic_load_n(&irq_handlers[i], __ATOMIC_RELAXED);
				if (isr) {
					isr();
				}
			}
		}
	}
}

/**
 * \brief Handles ICB CRC error IRQs.
 */
static void icb_crc_error_isr(void) {
	error_et_fire(ERROR_ET_ICB_CRC);
}

/**
 * \brief Initializes the interrupt-handling subsystem.
 *
 * \pre The FPGA must already be configured.
 */
void icb_irq_init(void) {
	// Map EXTI0 to PB0.
	rcc_enable(APB2, SYSCFG);
	SYSCFG.EXTICR[0U] = (SYSCFG.EXTICR[0U] & ~(0xFU << 0U)) | (0b0001 << 0U);
	rcc_disable(APB2, SYSCFG);

	// Enable rising edge interrupts on EXTI0.
	EXTI.IMR |= 1U;
	EXTI.RTSR |= 1U;

	// Set up a handler for the ICB CRC error IRQ.
	icb_irq_set_vector(ICB_IRQ_ICB_CRC, &icb_crc_error_isr);

	// Start the IRQ dispatching task.
	BaseType_t ok = xTaskCreate(&irq_task, "icb-irq", 512U, 0, PRIO_TASK_ICB_IRQ, 0);
	assert(ok == pdPASS);
}

/**
 * \brief Shuts down the interrupt-handling subsystem.
 */
void icb_irq_shutdown(void) {
	// Disable rising edge interrupts on EXTI0.
	EXTI.IMR &= ~1U;
}

/**
 * \brief Sets the function that will be invoked when a particular ICB IRQ occurs.
 *
 * \param[in] irq the interrupt bit to configure
 * \param[in] isr the function to invoke when the interrupt source occurs
 */
void icb_irq_set_vector(icb_irq_t irq, void (*isr)(void)) {
	assert(irq < ICB_IRQ_COUNT);
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
	__atomic_store_n(&irq_handlers[irq], isr, __ATOMIC_RELAXED);
	__atomic_signal_fence(__ATOMIC_RELEASE);
}

/**
 * @}
 */

/**
 * \name Configuration
 *
 * These functions are used to configure the FPGA.
 *
 * @{
 */

/**
 * \brief Starts the configuration process.
 *
 * \pre The ICB must have been initialized with \ref icb_init.
 *
 * \retval ICB_CONF_CONTINUE configuration can proceed by means of \ref icb_conf_block
 * \retval ICB_CONF_INIT_B_STUCK_HIGH INIT_B failed to fall when PROGRAM_B was pulled low
 * \retval ICB_CONF_INIT_B_STUCK_LOW INIT_B failed to rise after clearing configuration memory
 * \retval ICB_CONF_DONE_STUCK_HIGH DONE failed to be low for configuration
 */
icb_conf_result_t icb_conf_start(void) {
	// Lock the bus for the entire duration of the configuration operation.
	lock_bus();

	// Float chip select, thus allowing the FPGA to drive it during configuration.
	gpio_set(PIN_ICB_CS);
	gpio_set_pupd(PIN_ICB_CS, GPIO_PUPD_PU);
	gpio_set_od(PIN_ICB_CS);

	// Force the FPGA into configuration mode by pulling PROGRAM_B low.
	gpio_reset(PIN_FPGA_PROGRAM_B);

	// Enable the SPI module.
	SPI_CR1_t cr1 = {
		.CPHA = 0, // Capture on first clock transition, drive new data on second.
		.CPOL = 0, // Clock idles low.
		.MSTR = 1, // Master mode.
		.BR = 4, // Transmission speed is 84 MHz (APB2) ÷ 32 = 2.625 MHz.
		.SPE = 1, // SPI module now enabled.
		.LSBFIRST = 0, // Most significant bit is sent first.
		.SSI = 1, // Module should assume slave select is high → deasserted → no other master is using the bus.
		.SSM = 1, // Module internal slave select logic is controlled by software (SSI bit).
		.RXONLY = 0, // Transmit and receive.
		.DFF = 0, // Frames are 8 bits wide.
		.CRCNEXT = 0, // Do not transmit a CRC now.
		.CRCEN = 0, // CRC calculation disabled for this transaction.
		.BIDIMODE = 0, // 2-line bidirectional communication used.
	};
	SPI1.CR1 = cr1;

	// Wait for INIT_B to go low.
	{
		unsigned int tries = 100U / portTICK_PERIOD_MS;
		while (gpio_get_input(PIN_FPGA_INIT_B) && tries) {
			--tries;
			vTaskDelay(1U);
		}
		if (gpio_get_input(PIN_FPGA_INIT_B)) {
			unlock_bus();
			return ICB_CONF_INIT_B_STUCK_HIGH;
		}
	}

	// Release PROGRAM_B.
	gpio_set(PIN_FPGA_PROGRAM_B);

	// Wait for the FPGA to clear configuration memory, sample mode pins, and set INIT_B high.
	{
		unsigned int tries = 100U / portTICK_PERIOD_MS;
		while (!gpio_get_input(PIN_FPGA_INIT_B) && tries) {
			--tries;
			vTaskDelay(1U);
		}
		if (!gpio_get_input(PIN_FPGA_INIT_B)) {
			unlock_bus();
			return ICB_CONF_INIT_B_STUCK_LOW;
		}
	}

	// The FPGA should have brought DONE low as it started configuration.
	if (gpio_get_input(PIN_FPGA_DONE)) {
		unlock_bus();
		return ICB_CONF_DONE_STUCK_HIGH;
	}

	// The FPGA is now ready to receive blocks of bitstream.
	return ICB_CONF_OK;
}

/**
 * \brief Delivers a block of configuration bitstream to the FPGA.
 *
 * \pre The configuration process must have been started with \ref icb_conf_start.
 *
 * \param[in] data the block of bitstream to send
 * \param[in] length the length of the bitstream block
 */
void icb_conf_block(const void *data, size_t length) {
	// Send the data.
	const uint8_t *data8 = data;
	while (length--) {
		SPI1.DR = *data8++;
		while (!SPI1.SR.TXE);
	}
}

/**
 * \brief Finishes configuring the FPGA.
 *
 * \pre All bitstream blocks must have been delivered with \ref icb_conf_block.
 *
 * \retval ICB_CONF_DONE configuration is complete
 * \retval ICB_CONF_DONE_STUCK_LOW the DONE pin failed to go high, indicating a corrupted bitstream
 * \retval ICB_CONF_CRC_ERROR the INIT_B pin went low, indicating a bitstream CRC error
 */
icb_conf_result_t icb_conf_end(void) {
	// Wait for idle bus.
	while (SPI1.SR.BSY);

	// Disable the SPI module.
	SPI_CR1_t cr1 = { 0 };
	SPI1.CR1 = cr1;

	// Unlock the bus.
	unlock_bus();

	// Wait until either DONE goes high (indicating completion) or INIT_B goes
	// low (indicating CRC error).
	TickType_t last_wake_time = xTaskGetTickCount();
	unsigned int tries = 1000U / portTICK_PERIOD_MS;
	while (tries--) {
		if (!gpio_get_input(PIN_FPGA_INIT_B)) {
			return ICB_CONF_CRC_ERROR;
		}
		if (gpio_get_input(PIN_FPGA_DONE)) {
			// Drive chip select hard as FPGA should now have released it.
			// It was already set as output high before.
			gpio_set_pp(PIN_ICB_CS);
			gpio_set_pupd(PIN_ICB_CS, GPIO_PUPD_NONE);
			return ICB_CONF_OK;
		}
		vTaskDelayUntil(&last_wake_time, 1U);
	}

	// DONE never went high!
	return ICB_CONF_DONE_STUCK_LOW;
}

/**
 * @}
 */

/**
 * \name ISRs
 *
 * These interrupt service routines are registered in the interrupt vector table.
 *
 * @{
 */

/**
 * \brief Handles DMA controller 2 stream 0 interrupts.
 *
 * This function should be registered in the interrupt vector table at position 56.
 */
void dma2_stream0_isr(void) {
	_Static_assert(DMA_STREAM_RX == 0U, "Function needs rewriting to the proper stream number!");
	DMA_LISR_t lisr = DMA2.LISR;
	DMA_LIFCR_t lifcr = {
		.CFEIF0 = lisr.FEIF0,
		.CDMEIF0 = lisr.DMEIF0,
		.CTEIF0 = lisr.TEIF0,
		.CHTIF0 = lisr.HTIF0,
		.CTCIF0 = lisr.TCIF0,
	};
	assert(!lisr.DMEIF0 && !lisr.TEIF0);
	DMA2.LIFCR = lifcr;
	if (lisr.TCIF0) {
		BaseType_t yield = pdFALSE;
		switch (state) {
			case ICB_STATE_IN_DATA:
				// Data is done. Receive CRC.
				stop_dma();
				config_tx_dma(&ZERO, sizeof(crc), false, false);
				config_rx_dma(&crc, sizeof(crc));
				start_dma(true, true);
				state = ICB_STATE_IN_CRC;
				break;

			case ICB_STATE_IN_CRC:
				// CRC is done. Hand back control.
				stop_dma();
				xSemaphoreGiveFromISR(transaction_complete_sem, &yield);
				state = ICB_STATE_IDLE;
				break;

			default:
				// These cases should not get here.
				abort();
		}
		if (yield) {
			portYIELD_FROM_ISR();
		}
	}

	EXCEPTION_RETURN_BARRIER();
}

/**
 * \brief Handles DMA controller 2 stream 3 interrupts.
 *
 * This function should be registered in the interrupt vector table at position 59.
 */
void dma2_stream3_isr(void) {
	_Static_assert(DMA_STREAM_TX == 3U, "Function needs rewriting to the proper stream number!");
	DMA_LISR_t lisr = DMA2.LISR;
	DMA_LIFCR_t lifcr = {
		.CFEIF3 = lisr.FEIF3,
		.CDMEIF3 = lisr.DMEIF3,
		.CTEIF3 = lisr.TEIF3,
		.CHTIF3 = lisr.HTIF3,
		.CTCIF3 = lisr.TCIF3,
	};
	assert(!lisr.DMEIF3 && !lisr.TEIF3);
	DMA2.LIFCR = lifcr;
	if (lisr.TCIF3) {
		BaseType_t yield = pdFALSE;
		switch (state) {
			case ICB_STATE_OUT_HEADER_DATA:
				// Data is done. Send CRC.
				stop_dma();
				config_tx_dma(&crc, sizeof(crc), true, true);
				start_dma(true, false);
				state = ICB_STATE_OUT_CRC;
				break;

			case ICB_STATE_OUT_CRC:
				// CRC is done. Hand back control.
				stop_dma();
				xSemaphoreGiveFromISR(transaction_complete_sem, &yield);
				state = ICB_STATE_IDLE;
				break;

			case ICB_STATE_IN_HEADER:
				// Header is done. Hand back control.
				stop_dma();
				xSemaphoreGiveFromISR(transaction_complete_sem, &yield);
				state = ICB_STATE_IN_DATA;
				break;

			default:
				// These cases should not get here.
				abort();
		}
		if (yield) {
			portYIELD_FROM_ISR();
		}
	}

	EXCEPTION_RETURN_BARRIER();
}

/**
 * \brief Handles external interrupt line 0 interrupts.
 *
 * This function should be registered in the interrupt vector table at position 6.
 */
void exti0_isr(void) {
	// Clear pending interrupt.
	EXTI.PR = 1U;

	// Give semaphore.
	BaseType_t yield = pdFALSE;
	xSemaphoreGiveFromISR(irq_sem, &yield);
	if (yield) {
		portYIELD_FROM_ISR();
	}

	EXCEPTION_RETURN_BARRIER();
}

/**
 * @}
 */

/**
 * @}
 */
