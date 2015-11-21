/**
 * \defgroup MAIN Main System Functions
 *
 * \brief These functions handle overall management and supervision of the robot.
 *
 * @{
 */

#include "adc.h"
#include "breakbeam.h"
#include "charger.h"
#include "chicker.h"
#include "constants.h"
#include "dma.h"
#include "dr.h"
#include "encoder.h"
#include "feedback.h"
#include "icb.h"
#include "leds.h"
#include "log.h"
#include "lps.h"
#include "main.h"
#include "motor.h"
#include "mrf.h"
#include "pins.h"
#include "priority.h"
#include "receive.h"
#include "sdcard.h"
#include "tick.h"
#include "usb_config.h"
#include "wheels.h"
#include "primitives/primitive.h"
#include "upgrade/dfu.h"
#include "upgrade/fpga.h"
#include "upgrade/fw.h"
#include <FreeRTOS.h>
#include <build_id.h>
#include <cdcacm.h>
#include <core_progmem.h>
#include <crc32.h>
#include <exception.h>
#include <format.h>
#include <gpio.h>
#include <init.h>
#include <inttypes.h>
#include <nvic.h>
#include <rcc.h>
#include <semphr.h>
#include <sleep.h>
#include <stack.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unused.h>
#include <usb.h>
#include <registers/id.h>
#include <registers/iwdg.h>
#include <registers/mpu.h>
#include <registers/otg_fs.h>
#include <registers/systick.h>

static void stm32_main(void) __attribute__((noreturn));

STACK_ALLOCATE(mstack, 4096);
STACK_ALLOCATE(main_task_stack, 4096);

typedef void (*fptr)(void);
static const fptr exception_vectors[16U] __attribute__((used, section(".exception_vectors"))) = {
	[0U] = (fptr) (mstack + sizeof(mstack) / sizeof(*mstack)),
	[1U] = &stm32_main,
	[3U] = &exception_hard_fault_isr,
	[4U] = &exception_memory_manage_fault_isr,
	[5U] = &exception_bus_fault_isr,
	[6U] = &exception_usage_fault_isr,
	[11U] = &vPortSVCHandler,
	[12U] = &exception_debug_fault_isr,
	[14U] = &vPortPendSVHandler,
	[15U] = &vPortSysTickHandler,
};

static const fptr interrupt_vectors[82U] __attribute__((used, section(".interrupt_vectors"))) = {
	[NVIC_IRQ_EXTI0] = &exti0_isr,
	[NVIC_IRQ_EXTI9_5] = &sd_d0_exti_isr,
	[NVIC_IRQ_SDIO] = &sd_isr,
	[NVIC_IRQ_TIM6_DAC] = &timer6_isr,
	[NVIC_IRQ_DMA2_STREAM0] = &dma2_stream0_isr,
	[NVIC_IRQ_DMA2_STREAM3] = &dma2_stream3_isr,
	[NVIC_IRQ_OTG_FS] = &udev_isr,
};

static void app_exception_early(void) {
	// Kick the hardware watchdog.
	IWDG.KR = 0xAAAAU;

	// Power down the USB engine to disconnect from the host.
	OTG_FS.GCCFG.PWRDWN = 0;

	// Turn the LEDs on, except for Charged.
	gpio_set(PIN_LED_STATUS);
	gpio_set(PIN_LED_LINK);

	// Scram the charger so we don’t overcharge.
	charger_shutdown();

	// Cut motor power so we don’t go ramming into walls.
	// Normally we gracefully stop the motors first to avoid stress on the power control MOSFET, but under the circumstances, we can’t afford an ICB transaction and this is better than nothing.
	gpio_reset(PIN_POWER_HV);
}

static void app_exception_late(bool core_written) {
	// Disable SYSTICK while changing frequency.
	{
		SYST_CSR_t tmp = { 0 };
		SYSTICK.CSR = tmp;
	}
	// Set SYSTICK to divide by 168 so it overflows every microsecond.
	SYSTICK.RVR = 168U - 1U;
	// Reset the counter.
	SYSTICK.CVR = 0U;
	// Set SYSTICK to run with the core AHB clock.
	{
		SYST_CSR_t tmp = {
			.CLKSOURCE = 1, // Use core clock
			.ENABLE = 1, // Counter is running
		};
		SYSTICK.CSR = tmp;
	}

	// Show flashing lights.
	for (;;) {
		IWDG.KR = 0xAAAAU;
		gpio_reset(PIN_LED_STATUS);
		gpio_reset(PIN_LED_LINK);
		sleep_ms(500U);
		IWDG.KR = 0xAAAAU;
		gpio_set(PIN_LED_STATUS);
		if (core_written) {
			gpio_set(PIN_LED_LINK);
		}
		sleep_ms(500U);
	}
}

static const init_specs_t INIT_SPECS = {
	.flags = {
		.hse_crystal = false,
		.freertos = true,
		.io_compensation_cell = true,
	},
	.hse_frequency = 8,
	.pll_frequency = 336,
	.sys_frequency = 168,
	.cpu_frequency = 168,
	.apb1_frequency = 42,
	.apb2_frequency = 84,
	.exception_core_writer = &core_progmem_writer,
	.exception_app_cbs = {
		.early = &app_exception_early,
		.late = &app_exception_late,
	},
	.exception_prios = {
		[NVIC_IRQ_EXTI0] = EXCEPTION_MKPRIO(3, 1),
		[NVIC_IRQ_EXTI9_5] = EXCEPTION_MKPRIO(5, 0),
		[NVIC_IRQ_SDIO] = EXCEPTION_MKPRIO(5, 0),
		[NVIC_IRQ_TIM6_DAC] = EXCEPTION_MKPRIO(2, 0),
		[NVIC_IRQ_DMA2_STREAM0] = EXCEPTION_MKPRIO(3, 0),
		[NVIC_IRQ_DMA2_STREAM3] = EXCEPTION_MKPRIO(3, 0),
		[NVIC_IRQ_OTG_FS] = EXCEPTION_MKPRIO(6, 0),
	}
};

static const usb_string_descriptor_t STRING_EN_CA_MANUFACTURER = USB_STRING_DESCRIPTOR_INITIALIZER(u"UBC Thunderbots Football Club");
static const usb_string_descriptor_t STRING_EN_CA_PRODUCT = USB_STRING_DESCRIPTOR_INITIALIZER(u"Robot");
static const usb_string_descriptor_t STRING_EN_CA_PRODUCT_DFU = USB_STRING_DESCRIPTOR_INITIALIZER(u"Robot (DFU mode)");
static usb_string_descriptor_t STRING_SERIAL = USB_STRING_DESCRIPTOR_INITIALIZER(u"                        ");
static const usb_string_descriptor_t STRING_EN_CA_DFU_FW = USB_STRING_DESCRIPTOR_INITIALIZER(u"Firmware");
static const usb_string_descriptor_t STRING_EN_CA_DFU_FPGA = USB_STRING_DESCRIPTOR_INITIALIZER(u"FPGA Bitstream");
static const usb_string_descriptor_t * const STRINGS_EN_CA[] = {
	[STRING_INDEX_MANUFACTURER - 1U] = &STRING_EN_CA_MANUFACTURER,
	[STRING_INDEX_PRODUCT - 1U] = &STRING_EN_CA_PRODUCT,
	[STRING_INDEX_PRODUCT_DFU - 1U] = &STRING_EN_CA_PRODUCT_DFU,
	[STRING_INDEX_SERIAL - 1U] = &STRING_SERIAL,
	[STRING_INDEX_DFU_FW - 1U] = &STRING_EN_CA_DFU_FW,
	[STRING_INDEX_DFU_FPGA - 1U] = &STRING_EN_CA_DFU_FPGA,
};
const udev_language_info_t MAIN_LANGUAGE_TABLE[] = {
	{ .id = 0x1009U /* en_CA */, .strings = STRINGS_EN_CA },
	{ .id = 0, .strings = 0 },
};
const usb_string_zero_descriptor_t MAIN_STRING_ZERO = {
	.bLength = sizeof(usb_string_zero_descriptor_t) + sizeof(uint16_t),
	.bDescriptorType = USB_DTYPE_STRING,
	.wLANGID = { 0x1009U /* en_CA */, },
};

static bool usb_control_handler(const usb_setup_packet_t *pkt) {
	if (pkt->bmRequestType.recipient == USB_RECIPIENT_DEVICE && pkt->bmRequestType.type == USB_CTYPE_VENDOR && pkt->bRequest == CONTROL_REQUEST_READ_CORE && pkt->wValue < 256U && !pkt->wIndex && pkt->wLength == 1024U) {
		uep0_data_write(&core_progmem_dump[pkt->wValue * 1024U / 4U], 1024U);
		return true;
	} else if (pkt->bmRequestType.recipient == USB_RECIPIENT_DEVICE && pkt->bmRequestType.type == USB_CTYPE_VENDOR && pkt->bRequest == CONTROL_REQUEST_READ_BUILD_ID && !pkt->wValue && !pkt->wIndex && pkt->wLength == 4U) {
		uint32_t id = build_id_get();
		uep0_data_write(&id, sizeof(id));
		return true;
	}
	return false;
}

STACK_ALLOCATE(usb_task_stack, 4096);

static const udev_info_t USB_INFO = {
	.flags = {
		.vbus_sensing = 1,
		.minimize_interrupts = 1,
		.self_powered = 1,
	},
	.internal_task_priority = PRIO_TASK_USB,
	.internal_task_stack = usb_task_stack,
	.internal_task_stack_size = sizeof(usb_task_stack),
	.receive_fifo_words = 10U /* SETUP packets */ + 1U /* Global OUT NAK status */ + ((64U / 4U) + 1U) * 2U /* Packets */ + 4U /* Transfer complete status */,
	.device_descriptor = {
		.bLength = sizeof(usb_device_descriptor_t),
		.bDescriptorType = USB_DTYPE_DEVICE,
		.bcdUSB = 0x0200U,
		.bDeviceClass = 0x00U,
		.bDeviceSubClass = 0x00U,
		.bDeviceProtocol = 0x00U,
		.bMaxPacketSize0 = 64U,
		.idVendor = VENDOR_ID,
		.idProduct = PRODUCT_ID,
		.bcdDevice = 0x0101U,
		.iManufacturer = STRING_INDEX_MANUFACTURER,
		.iProduct = STRING_INDEX_PRODUCT,
		.iSerialNumber = STRING_INDEX_SERIAL,
		.bNumConfigurations = 1U,
	},
	.string_count = STRING_INDEX_COUNT - 1U /* exclude zero */,
	.string_zero_descriptor = &MAIN_STRING_ZERO,
	.language_table = MAIN_LANGUAGE_TABLE,
	.control_handler = &usb_control_handler,
	.configurations = {
		&USB_CONFIGURATION,
	},
};

static uint32_t idle_cycles = 0;
static bool shutting_down = false;
static main_shut_mode_t shutdown_mode;
static SemaphoreHandle_t supervisor_sem;
static unsigned int wdt_sources = 0U;

/**
 * \brief A semaphore that modules can use to sequence shutdown operations
 * (i.e. for \c _shutdown functions to to wait until the worker tasks have
 * quiesced).
 */
SemaphoreHandle_t main_shutdown_sem;

void vApplicationIdleHook(void) {
	// We will do a WFI to put the chip to sleep until some activity happens.
	// WFI will finish when an enabled interrupt (or some other
	// implementation-specific event) occurs. However, for the definition of
	// “enabled” for the sake of WFI, an interrupt disabled due to the PRIMASK
	// bit is not considered disabled—PRIMASK will prevent the ISR from being
	// entered, but not WFI from waking up. Instead, on wakeup, execution will
	// continue after the WFI, with the interrupt remaining pending. We can use
	// this to get some control before handling the interrupt in an ISR.
	asm volatile("cpsid i");
	// ARMv7-M Architecture Reference Manual B5.2.1 (CPS instruction) states
	// that “If execution of a CPS instruction increases the execution
	// priority, the CPS execution serializes that change to the instruction
	// stream.”. So no ISB is needed here.

	// Capture the system tick count before sleeping.
	uint32_t systick_before = SYSTICK.CVR;

	// Go to sleep until an interrupt.
	asm volatile("wfi");

	// Capture the system tick count after sleeping.
	uint32_t systick_after = SYSTICK.CVR;

	// System tick counts downwards, so if after ≥ before, that means it must
	// have wrapped. It can’t have wrapped more than once, because as soon as
	// it wraps the first time, the system tick interrupt will become pending
	// and wake us up. We can assume (especially since we have PRIMASK set)
	// that we will get past this code before the system tick counter wraps a
	// second time, so it can only have wrapped once. Accumulate the count of
	// cycles we spent asleep onto the idle cycle counter.
	if (systick_after >= systick_before) {
		systick_before += SYSTICK.RVR + 1;
	}
	__atomic_fetch_add(&idle_cycles, systick_before - systick_after, __ATOMIC_RELAXED);

	// Re-enable interrupts, which allows whichever pending interrupt woke us
	// from WFI to now execute its ISR and execute to proceed as usual.
	asm volatile("cpsie i");
	// ARMv7-M Architecture Reference Manual B5.2.1 (CPS instruction) states
	// that “If execution of a CPS instruction decreases the execution
	// priority, the architecture guarantees only that the new priority is
	// visible to instructions executed after either executing an ISB, or
	// performing an exception entry or exception return.”. Normally this
	// wouldn’t matter because we’re going off and doing something else anyway;
	// however, theoretically, it’s possible that we might get back into
	// vApplicationIdleHook before the CPU had a chance to take the pending
	// interrupt, disable interrupts again, and thus not actually handle the
	// interrupt. So, use a barrier.
	asm volatile("isb");
}

static void main_task(void *param) __attribute__((noreturn));

static void stm32_main(void) {
	// Initialize the basic chip hardware.
	init_chip(&INIT_SPECS);

	// Initialize the GPIO pins.
	gpio_init(PINS_INIT, sizeof(PINS_INIT) / sizeof(*PINS_INIT));

	// Get into FreeRTOS.
	main_shutdown_sem = xSemaphoreCreateCounting((UBaseType_t) -1, 0);
	BaseType_t ok = xTaskGenericCreate(&main_task, "main", sizeof(main_task_stack) / sizeof(*main_task_stack), 0, PRIO_TASK_SUPERVISOR, 0, main_task_stack, 0);
	assert(ok == pdPASS);
	vTaskStartScheduler();
	__builtin_unreachable();
}

static void run_normal(void) {
	// Read the configuration switches.
	static uint8_t switches[2U];
	icb_receive(ICB_COMMAND_READ_SWITCHES, switches, sizeof(switches));
	iprintf("Switches: Robot index %" PRIu8 ", channel %u\r\n", switches[0U], (switches[1] & 3U));

	// Enable independent watchdog.
	while (IWDG.SR.PVU);
	IWDG.KR = 0x5555U;
	IWDG.PR = 3U; // Divide by 32.
	while (IWDG.SR.RVU);
	IWDG.KR = 0x5555U;
	IWDG.RLR = 0xFFFU; // Reload value maximum, roughly 4096 millisecond period.
	IWDG.KR = 0xCCCCU;

	// Bring up lots of stuff.
	icb_irq_init();
	chicker_init();
	charger_init();
	static const struct {
		uint8_t channel;
		bool symbol_rate;
		uint16_t pan;
	} mrf_profiles[4] = {
		{ 16, false, 0x1846U },
		{ 17, false, 0x1847U },
		{ 17, false, 0x1848U },
		{ 17, false, 0x1849U },
	};
	
	// Assign a mrf profile.
	unsigned int profile;
	profile = (switches[1] & 3U);

	fputs("MRF init: ", stdout);
	fflush(stdout);
	mrf_init(mrf_profiles[profile].channel, mrf_profiles[profile].symbol_rate, mrf_profiles[profile].pan, switches[0U], UINT64_C(0xec89d61e8ffd409b));
	fputs("OK\r\n", stdout);
	feedback_init();
	motor_init();
	wheels_init();
	encoder_init();
	dr_init();
	primitive_init();
	lps_init();

	// Bring up the data logger.
	fputs("Supervisor: log init: ", stdout);
	if (log_init()) {
		fputs("OK\r\n", stdout);
	} else {
		switch (log_state()) {
			case LOG_STATE_OK: fputs("Confused\r\n", stdout); break;
			case LOG_STATE_UNINITIALIZED: fputs("Uninitialized\r\n", stdout); break;
			case LOG_STATE_SD_ERROR: fputs("SD card error\r\n", stdout); break;
			case LOG_STATE_CARD_FULL: fputs("SD card full\r\n", stdout); break;
		}
	}

	// Receive must be the second-last module initialized, because received
	// packets can cause calls to other modules.
	receive_init(switches[0U]);
	
	// Ticks must be the last module initialized, because ticks propagate into
	// other modules.
	tick_init();

	// Done!
	fputs("System online.\r\n", stdout);
	leds_status_set(true);

	// Supervise.
	while (!__atomic_load_n(&shutting_down, __ATOMIC_RELAXED)) {
		BaseType_t ret = xSemaphoreTake(supervisor_sem, 100U / portTICK_PERIOD_MS);
		if (ret == pdFALSE) {
			// Timeout occurred, so check for task liveness.
			assert(__atomic_load_n(&wdt_sources, __ATOMIC_RELAXED) == (1U << MAIN_WDT_SOURCE_COUNT) - 1U);
			__atomic_store_n(&wdt_sources, 0U, __ATOMIC_RELAXED);
			// Kick the hardware watchdog.
			IWDG.KR = 0xAAAAU;
		}
	}

	// Kick the hardware watchdog to avoid timeouts during shutdown.
	IWDG.KR = 0xAAAAU;

	// Shut down the system. This comprises four basic phases:
	//
	// Phase 1: prevent outside influence on the system. This means
	// stopping the tick generator, the radio receive task, and the
	// feedback task. Once this is done, further shutdown activity does not
	// need to worry about its work being undone.
	//
	// Phase 2: stop things from getting less safe. This means setting the
	// motors to coast and turning off the charger. Once this is done,
	// nothing should be putting significant load on the battery, and
	// nothing should be doing anything scary.
	//
	// Phase 3: make dangerous things safe. This means discharging the
	// capacitors. Once this is done, the circuit should be safe to touch.
	//
	// Phase 4: incrementally shut down remaining circuit components. This
	// means cutting supply to the motor drivers, resetting the radio,
	// wiping the FPGA, detaching from the USB, and then either cutting
	// logic supply or rebooting the CPU. The last few steps are done in
	// main_task instead of here, as those particular items are initialized in
	// both safe mode and normal mode.
	fputs("System shutdown.\r\n", stdout);

	tick_shutdown();
	receive_shutdown();
	feedback_shutdown();

	charger_shutdown();
	motor_shutdown();

	chicker_shutdown();

	// Kick the hardware watchdog to avoid timeouts. Chicker shutdown sometimes
	// takes up to three seconds, particularly if the board is not plugged in,
	// so that eats most of the timeout period.
	IWDG.KR = 0xAAAAU;

	gpio_reset(PIN_POWER_HV);
	mrf_shutdown();
	log_shutdown();
	vTaskDelay(100U / portTICK_PERIOD_MS);
	icb_irq_shutdown();
}

static void run_safe_mode(void) {
	// Show status.
	fputs("System running in safe mode.\r\n", stdout);
	fflush(stdout);

	// Wait until requested to shut down for some reason (e.g. DFU entry).
	// Blink the status light while we’re at it.
	unsigned int counter = 0U;
	TickType_t last_wake_time = xTaskGetTickCount();
	while (!__atomic_load_n(&shutting_down, __ATOMIC_RELAXED)) {
		++counter;
		if (counter == 1000U / portTICK_PERIOD_MS) {
			counter = 0;
			gpio_set(PIN_LED_STATUS);
		} else if (counter == 150U / portTICK_PERIOD_MS) {
			gpio_reset(PIN_LED_STATUS);
		}
		vTaskDelayUntil(&last_wake_time, 1U);
	}

	// Shut down.
	fputs("System shutdown.\r\n", stdout);
}

static void main_task(void *UNUSED(param)) {
	// Create a semaphore which other tasks will use to ping the supervisor.
	supervisor_sem = xSemaphoreCreateBinary();
	assert(supervisor_sem);

	// Initialize DMA engines.
	// These are needed for a lot of other things so must come first.
	dma_init();

	// Initialize ADCs.
	// SAFETY CRITICAL: The ADCs drive the charged LED, so we must initialize them early while the bootup LEDs are still on!
	adc_init();

	// Wait a bit.
	vTaskDelay(100U / portTICK_PERIOD_MS);

	// Fill in the device serial number.
	{
		char temp[24U];
		formathex32(&temp[0U], U_ID.H);
		formathex32(&temp[8U], U_ID.M);
		formathex32(&temp[16U], U_ID.L);
		for (size_t i = 0U; i < 24U; ++i) {
			STRING_SERIAL.bString[i] = temp[i];
		}
	}

	// Initialize CRC32 calculator.
	crc32_init();

	// Calculate the build ID.
	build_id_init();

	// Initialize LEDs.
	leds_init();

	// Initialize CDC ACM.
	cdcacm_init(2U, PRIO_TASK_CDC_ACM);

	// Initialize USB.
	udev_init(&USB_INFO);
	udev_attach();

	// Provide a message that will be printed as soon as the user connects.
	iprintf("Supervisor: System init\r\nMCU Build ID: 0x%08" PRIX32 "\r\n", build_id_get());

	// Initialize ICB-related objects. This will be needed later, even if no SD
	// card is found, as we unconditionally try to wipe the FPGA bitstream as
	// part of shutdown, and that process requires that the relevant
	// semaphores, etc. have been created.
	icb_init();

	// Bring up the SD card.
	fputs("Supervisor: SD init: ", stdout);
	bool ok = sd_init() == SD_STATUS_OK;
	if (ok) {
		// Check for a new firmware image to install, or an existing one to
		// delete due to the ephemeral flag.
		upgrade_fw_check_install();

		// Check if we have a proper FPGA bitstream.
		ok = upgrade_fpga_check();
		if (ok) {
			iprintf("FPGA Build ID: 0x%08" PRIX32 "\r\n", upgrade_fpga_build_id());
		} else {
			fputs("Supervisor: No FPGA bitstream\r\n", stdout);
		}
	}
	if (ok) {
		// Bring up the ICB and configure the FPGA using a bitstream stored the
		// SD card.
		fputs("Supervisor: FPGA init: ", stdout);
		fflush(stdout);
		icb_conf_result_t rc = icb_conf_start();
		bool reading_ok = true;
		if (rc == ICB_CONF_OK) {
			reading_ok = upgrade_fpga_send();
			rc = icb_conf_end();
		}
		if (reading_ok) {
			switch (rc) {
				case ICB_CONF_OK:
					fputs("OK\r\n", stdout);
					break;
				case ICB_CONF_INIT_B_STUCK_HIGH:
					fputs("INIT_B stuck high\r\n", stdout);
					break;
				case ICB_CONF_INIT_B_STUCK_LOW:
					fputs("INIT_B stuck low\r\n", stdout);
					break;
				case ICB_CONF_DONE_STUCK_HIGH:
					fputs("DONE stuck high\r\n", stdout);
					break;
				case ICB_CONF_DONE_STUCK_LOW:
					fputs("DONE stuck low\r\n", stdout);
					break;
				case ICB_CONF_CRC_ERROR:
					fputs("Bitstream CRC error\r\n", stdout);
					break;
			}
		} else {
			fputs("Error reading bitstream from SD\r\n", stdout);
		}
		ok = reading_ok && (rc == ICB_CONF_OK);
	}
	if (ok) {
		// Give the FPGA circuit some time to bring up clocks, come out of
		// reset, etc.
		vTaskDelay(100U / portTICK_PERIOD_MS);

		// Read the FPGA device DNA.
		fputs("Device DNA: ", stdout);
		fflush(stdout);
		TickType_t last_wake_time = xTaskGetTickCount();
		unsigned int tries = 1000U / portTICK_PERIOD_MS;
		ok = false;
		while (!ok && tries--) {
			static uint64_t device_dna = 0U;
			vTaskDelayUntil(&last_wake_time, 1U);
			icb_receive(ICB_COMMAND_READ_DNA, &device_dna, 7U);
			if ((device_dna & (UINT64_C(1) << 55U))) {
				iprintf("0x%014" PRIX64 "\r\n", device_dna & ~(UINT64_C(1) << 55U));
				ok = true;
			}
		}
		if (!ok) {
			fputs("Timed out waiting for device DNA\r\n", stdout);
		}
	}
	if (ok) {
		run_normal();
	} else {
		run_safe_mode();
	}

	// Wipe the FPGA configuration, if any.
	icb_conf_start();

	// Disconnect from the USB and wait for the disconnect to take effect.
	udev_detach();
	vTaskDelay(100U / portTICK_PERIOD_MS);

	// Do whatever we are supposed to do now.
	switch (shutdown_mode) {
		case MAIN_SHUT_MODE_DFU:
			upgrade_dfu_run();
			// Fall through to reboot after DFU is done.
		case MAIN_SHUT_MODE_REBOOT:
			asm volatile("cpsid i");
			asm volatile("dsb");
			{
				AIRCR_t tmp = SCB.AIRCR;
				tmp.VECTKEY = 0x05FA;
				tmp.SYSRESETREQ = 1;
				SCB.AIRCR = tmp;
			}
			__sync_synchronize();
			for (;;);

		case MAIN_SHUT_MODE_POWER:
		default:
			asm volatile("cpsid i");
			asm volatile("dsb");
			gpio_reset(PIN_POWER_LOGIC);
			asm volatile("dsb");
			for (;;);
	}
}

/**
 * \brief Records liveness of a supervised task.
 *
 * \param[in] source the source of the report
 */
void main_kick_wdt(main_wdt_source_t source) {
	__atomic_or_fetch(&wdt_sources, 1U << source, __ATOMIC_RELAXED);
}

/**
 * \brief Begins shutting down the robot.
 *
 * This function is asynchronous; it returns immediately, but proceeds with the
 * shutdown sequence in another task.
 *
 * \param[in] mode what to do after shutting down
 */
void main_shutdown(main_shut_mode_t mode) {
	shutdown_mode = mode;
	__atomic_signal_fence(__ATOMIC_RELEASE);
	__atomic_store_n(&shutting_down, true, __ATOMIC_RELAXED);
	xSemaphoreGive(supervisor_sem);
}

/**
 * \brief Reads and clears the count of idle cycles.
 *
 * \return the number of CPU cycles during which the system was idle since this
 * function was last called
 */
uint32_t main_read_clear_idle_cycles(void) {
	return __atomic_exchange_n(&idle_cycles, 0, __ATOMIC_RELAXED);
}

/**
 * @}
 */
