#include "buzzer.h"
#include "constants.h"
#include "estop.h"
#include "mrf.h"
#include "normal.h"
#include "promiscuous.h"
#include "radio_sleep.h"
#include <deferred.h>
#include <exception.h>
#include <format.h>
#include <rcc.h>
#include <registers.h>
#include <sleep.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unused.h>
#include <usb_configs.h>
#include <usb_ep0.h>
#include <usb_ep0_sources.h>
#include <usb_fifo.h>
#include <usb_ll.h>

static void stm32_main(void) __attribute__((noreturn));
static void nmi_vector(void);
static void service_call_vector(void);
static void system_tick_vector(void);
void adc_interrupt_vector(void);
void exti_dispatcher_0(void);
void exti_dispatcher_1(void);
void exti_dispatcher_2(void);
void exti_dispatcher_3(void);
void exti_dispatcher_4(void);
void exti_dispatcher_9_5(void);
void exti_dispatcher_15_10(void);
void timer5_interrupt_vector(void);
void timer6_interrupt_vector(void);
void timer7_interrupt_vector(void);

static char stack[65536] __attribute__((section(".stack")));

typedef void (*fptr)(void);
static const fptr exception_vectors[16] __attribute__((used, section(".exception_vectors"))) = {
	[0] = (fptr) (stack + sizeof(stack)),
	[1] = &stm32_main,
	[2] = &nmi_vector,
	[3] = &exception_hard_fault_vector,
	[4] = &exception_memory_manage_fault_vector,
	[5] = &exception_bus_fault_vector,
	[6] = &exception_usage_fault_vector,
	[11] = &service_call_vector,
	[14] = &deferred_fn_pendsv_handler,
	[15] = &system_tick_vector,
};

static const fptr interrupt_vectors[82] __attribute__((used, section(".interrupt_vectors"))) = {
	[6] = &exti_dispatcher_0,
	[7] = &exti_dispatcher_1,
	[8] = &exti_dispatcher_2,
	[9] = &exti_dispatcher_3,
	[10] = &exti_dispatcher_4,
	[18] = &adc_interrupt_vector,
	[23] = &exti_dispatcher_9_5,
	[40] = &exti_dispatcher_15_10,
	[50] = &timer5_interrupt_vector,
	[54] = &timer6_interrupt_vector,
	[55] = &timer7_interrupt_vector,
	[67] = &usb_ll_process,
};

static void nmi_vector(void) {
	abort();
}

static void service_call_vector(void) {
	for (;;);
}

static void system_tick_vector(void) {
	for (;;);
}

volatile uint64_t bootload_flag;

static const uint8_t DEVICE_DESCRIPTOR[18] = {
	18, // bLength
	USB_DTYPE_DEVICE, // bDescriptorType
	0, // bcdUSB LSB
	2, // bcdUSB MSB
	0xFF, // bDeviceClass
	0, // bDeviceSubClass
	0, // bDeviceProtocol
	8, // bMaxPacketSize0
	(uint8_t) MRF_DONGLE_VID, // idVendor LSB
	MRF_DONGLE_VID >> 8, // idVendor MSB
	(uint8_t) MRF_DONGLE_PID, // idProduct LSB
	MRF_DONGLE_PID >> 8, // idProduct MSB
	0, // bcdDevice LSB
	1, // bcdDevice MSB
	STRING_INDEX_MANUFACTURER, // iManufacturer
	STRING_INDEX_PRODUCT, // iProduct
	STRING_INDEX_SERIAL, // iSerialNumber
	3, // bNumConfigurations
};

static const uint8_t STRING_ZERO[4] = {
	sizeof(STRING_ZERO),
	USB_DTYPE_STRING,
	0x09, 0x10, /* English (Canadian) */
};

static const usb_configs_config_t * const CONFIGURATIONS[] = {
	&RADIO_SLEEP_CONFIGURATION,
	&NORMAL_CONFIGURATION,
	&PROMISCUOUS_CONFIGURATION,
	0
};

static usb_ep0_disposition_t on_zero_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_VENDOR | USB_REQ_TYPE_DEVICE) && pkt->request == CONTROL_REQUEST_BEEP) {
		// This request must have index set to zero.
		if (pkt->index) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Start the buzzer.
		buzzer_start(pkt->value);

		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_OUT | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_SET_ADDRESS) {
		// This request must have a valid address as its value, an index of zero, and occur while the device is unconfigured.
		if (pkt->value > 127 || pkt->index || usb_configs_get_current()) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// Lock in the address; the hardware knows to stay on address zero until the status stage is complete and, in fact, *FAILS* if the address is locked in later!
		OTG_FS_DCFG = (OTG_FS_DCFG & ~DCFG_DAD_MSK) | DCFG_DAD(pkt->value);

		// Initialize or deinitialize the configuration handler module depending on the assigned address.
		if (pkt->value) {
			usb_configs_init(CONFIGURATIONS);
		} else {
			usb_configs_deinit();
		}

		return USB_EP0_DISPOSITION_ACCEPT;
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static usb_ep0_disposition_t on_in_request(const usb_ep0_setup_packet_t *pkt, usb_ep0_source_t **source, usb_ep0_poststatus_cb_t *UNUSED(poststatus)) {
	static uint8_t stash_buffer[25];
	static union {
		usb_ep0_memory_source_t mem_src;
		usb_ep0_string_descriptor_source_t string_src;
	} src;

	if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_GET_STATUS) {
		// This request must have value and index set to zero.
		if (pkt->value || pkt->index) {
			return USB_EP0_DISPOSITION_REJECT;
		}

		// We do not support remote wakeup, so bit 1 is always set to zero.
		// We are always *effectively* bus-powered (we can be target-powered, but might as well be so only when the bus is disconnected), so bit 0 is always set to zero.
		stash_buffer[0] = 0;
		stash_buffer[1] = 0;
		*source = usb_ep0_memory_source_init(&src.mem_src, stash_buffer, 2);
		return USB_EP0_DISPOSITION_ACCEPT;
	} else if (pkt->request_type == (USB_REQ_TYPE_IN | USB_REQ_TYPE_STD | USB_REQ_TYPE_DEVICE) && pkt->request == USB_REQ_GET_DESCRIPTOR) {
		uint8_t type = pkt->value >> 8;
		uint8_t index = pkt->value;
		switch (type) {
			case USB_DTYPE_DEVICE:
			{
				// GET DESCRIPTOR(DEVICE)
				if (index || pkt->index) {
					return USB_EP0_DISPOSITION_REJECT;
				}
				*source = usb_ep0_memory_source_init(&src.mem_src, DEVICE_DESCRIPTOR, sizeof(DEVICE_DESCRIPTOR));
				return USB_EP0_DISPOSITION_ACCEPT;
			}

			case USB_DTYPE_CONFIGURATION:
			{
				// GET DESCRIPTOR(CONFIGURATION)
				if (pkt->index) {
					return USB_EP0_DISPOSITION_REJECT;
				}

				const uint8_t *descriptor = 0;
				switch (index) {
					case 0: descriptor = RADIO_SLEEP_CONFIGURATION_DESCRIPTOR; break;
					case 1: descriptor = NORMAL_CONFIGURATION_DESCRIPTOR; break;
					case 2: descriptor = PROMISCUOUS_CONFIGURATION_DESCRIPTOR; break;
				}
				if (descriptor) {
					size_t total_length = descriptor[2] | (descriptor[3] << 8);
					*source = usb_ep0_memory_source_init(&src.mem_src, descriptor, total_length);
					return USB_EP0_DISPOSITION_ACCEPT;
				} else {
					return USB_EP0_DISPOSITION_REJECT;
				}
			}

			case USB_DTYPE_STRING:
			{
				// GET DESCRIPTOR(STRING)
				if (!index && !pkt->index) {
					*source = usb_ep0_memory_source_init(&src.mem_src, STRING_ZERO, sizeof(STRING_ZERO));
					return USB_EP0_DISPOSITION_ACCEPT;
				} else if (pkt->index == 0x1009 /* English (Canadian) */) {
					const char *string = 0;
					switch (index) {
						case STRING_INDEX_MANUFACTURER: string = u8"UBC Thunderbots Small Size Team"; break;
						case STRING_INDEX_PRODUCT: string = u8"Radio Base Station"; break;
						case STRING_INDEX_CONFIG1: string = u8"Radio Sleep/Pre-DFU"; break;
						case STRING_INDEX_CONFIG2: string = u8"Normal Operation"; break;
						case STRING_INDEX_CONFIG3: string = u8"Promiscuous Mode"; break;
						case STRING_INDEX_SERIAL:
							formathex32((char *) stash_buffer + 0, U_ID_H);
							formathex32((char *) stash_buffer + 8, U_ID_M);
							formathex32((char *) stash_buffer + 16, U_ID_L);
							((char *) stash_buffer)[24] = '\0';
							string = (const char *) stash_buffer;
							break;
					}
					if (string) {
						*source = usb_ep0_string_descriptor_source_init(&src.string_src, string);
						return USB_EP0_DISPOSITION_ACCEPT;
					} else {
						return USB_EP0_DISPOSITION_REJECT;
					}
				} else {
					return USB_EP0_DISPOSITION_REJECT;
				}
			}

			case USB_DTYPE_INTERFACE:
			case USB_DTYPE_ENDPOINT:
			case USB_DTYPE_DEVICE_QUALIFIER:
			case USB_DTYPE_OTHER_SPEED_CONFIGURATION:
			case USB_DTYPE_INTERFACE_POWER:
			{
				// GET DESCRIPTOR(…)
				// These are either not present or are meant to be requested through GET DESCRIPTOR(CONFIGURATION).
				return USB_EP0_DISPOSITION_REJECT;
			}

			default:
			{
				// GET DESCRIPTOR(unknown)
				// Other descriptors may be handled by other layers.
				return USB_EP0_DISPOSITION_NONE;
			}
		}
	} else {
		return USB_EP0_DISPOSITION_NONE;
	}
}

static const usb_ep0_cbs_t GLOBAL_CBS = {
	.on_zero_request = &on_zero_request,
	.on_in_request = &on_in_request,
};

static void handle_usb_reset(void) {
	// Turn off the buzzer.
	buzzer_stop();

	// Shut down the control transfer layer, if the device was already active (implying the control transfer layer had been initialized).
	if (usb_ll_get_state() == USB_LL_STATE_ACTIVE) {
		usb_configs_deinit();
		usb_ep0_deinit();
	}
	
	// Configure receive FIFO and endpoint 0 transmit FIFO sizes.
	usb_fifo_init(512, 64);
}

static void handle_usb_enumeration_done(void) {
	usb_ep0_init(8);
	usb_ep0_cbs_push(&GLOBAL_CBS);
}

extern unsigned char linker_data_vma_start;
extern unsigned char linker_data_vma_end;
extern const unsigned char linker_data_lma_start;
extern unsigned char linker_bss_vma_start;
extern unsigned char linker_bss_vma_end;

static void stm32_main(void) {
	// Check if we’re supposed to go to the bootloader.
	uint32_t rcc_csr_shadow = RCC_CSR; // Keep a copy of RCC_CSR
	RCC_CSR |= RMVF; // Clear reset flags
	RCC_CSR &= ~RMVF; // Stop clearing reset flags
	if ((rcc_csr_shadow & SFTRSTF) && bootload_flag == UINT64_C(0xFE228106195AD2B0)) {
		bootload_flag = 0;
		asm volatile(
			"mov sp, %[stack]\n\t"
			"mov pc, %[vector]"
			:
			: [stack] "r" (*(const volatile uint32_t *) 0x1FFF0000), [vector] "r" (*(const volatile uint32_t *) 0x1FFF0004));
	}
	bootload_flag = 0;

	// Copy initialized globals and statics from ROM to RAM.
	memcpy(&linker_data_vma_start, &linker_data_lma_start, &linker_data_vma_end - &linker_data_vma_start);
	// Scrub the BSS section in RAM.
	memset(&linker_bss_vma_start, 0, &linker_bss_vma_end - &linker_bss_vma_start);

	// Always 8-byte-align the stack pointer on entry to an interrupt handler (as ARM recommends).
	SCS_CCR |= STKALIGN; // Guarantee 8-byte alignment

	// Set the interrupt system to set priorities as having the upper two bits for group priorities and the rest as subpriorities.
	SCS_AIRCR = (SCS_AIRCR & ~VECTKEY(0xFFFF)) | VECTKEY(0x05FA) | PRIGROUP(5);

	// Set up interrupt handling.
	exception_init();

	// Set up the memory protection unit to catch bad pointer dereferences.
	// We define the following regions:
	// Region 0: 0x08000000 length 1 MiB: Flash memory (normal, read-only, write-through cache, executable)
	// Region 1: 0x10000000 length 64 kiB: CCM (normal, read-write, write-back write-allocate cache, not executable)
	// Region 2: 0x1FFF7A00 length 64 bytes: U_ID and F_SIZE (device, read-only, not executable) (this should be 0x1FFF7A10 length 12 plus 0x1FFF7A22 length 4, but this is all we can do)
	// Region 3: 0x20000000 length 128 kiB: SRAM (normal, read-write, write-back write-allocate cache, not executable)
	// Region 4: 0x40000000 length 32 kiB: APB1 peripherals (device, read-write, not executable)
	// Region 5: 0x40010000 length 32 kiB: APB2 peripherals (device, read-write, not executable) using subregions:
	//   This should be 22,258 bytes.
	//   Regions must be powers of 2, so we set 32 kiB.
	//   The closest eight divsion we can get is ¾; ⅝ would be too small.
	//   So set the bottommost 6 subregions as present, and the top 2 subregions as absent.
	// Region 6: 0x40020000 length 512 kiB: AHB1 peripherals (device, read-write, not executable)
	// Region 7: 0x50000000 length 512 kiB: AHB2 peripherals (device, read-write, not executable)
	//   This should be 396,288 bytes.
	//   Regions must be powers of 2, so we set 512 kiB.
	//   The closest eight division we can get is ⅞; ¾ is too small.
	//   So set the bottommost 7 subregions as present, and the top 1 subregion as absent.
	// The private peripheral bus (0xE0000000 length 1 MiB) always uses the system memory map, so no region is needed for it.
	// We set up the regions first, then enable the MPU.
	MPU_RNR = 0;
	MPU_RBAR = 0x08000000;
	MPU_RASR = MPU_RASR_AP(0b111) | MPU_RASR_TEX(0b000) | MPU_RASR_C | MPU_RASR_SRD(0) | MPU_RASR_SIZE(19) | MPU_RASR_ENABLE;
	MPU_RNR = 1;
	MPU_RBAR = 0x10000000;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b011) | MPU_RASR_TEX(0b001) | MPU_RASR_C | MPU_RASR_B | MPU_RASR_SRD(0) | MPU_RASR_SIZE(15) | MPU_RASR_ENABLE;
	MPU_RNR = 2;
	MPU_RBAR = 0x1FFF7A00;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b111) | MPU_RASR_TEX(0b010) | MPU_RASR_SRD(0) | MPU_RASR_SIZE(5) | MPU_RASR_ENABLE;
	MPU_RNR = 3;
	MPU_RBAR = 0x20000000;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b011) | MPU_RASR_TEX(0b001) | MPU_RASR_C | MPU_RASR_B | MPU_RASR_SRD(0) | MPU_RASR_SIZE(16) | MPU_RASR_ENABLE;
	MPU_RNR = 4;
	MPU_RBAR = 0x40000000;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b011) | MPU_RASR_TEX(0b010) | MPU_RASR_SRD(0) | MPU_RASR_SIZE(14) | MPU_RASR_ENABLE;
	MPU_RNR = 5;
	MPU_RBAR = 0x40010000;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b011) | MPU_RASR_TEX(0b010) | MPU_RASR_SRD(0b11000000) | MPU_RASR_SIZE(14) | MPU_RASR_ENABLE;
	MPU_RNR = 6;
	MPU_RBAR = 0x40020000;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b011) | MPU_RASR_TEX(0b010) | MPU_RASR_SRD(0) | MPU_RASR_SIZE(18) | MPU_RASR_ENABLE;
	MPU_RNR = 7;
	MPU_RBAR = 0x50000000;
	MPU_RASR = MPU_RASR_XN | MPU_RASR_AP(0b011) | MPU_RASR_TEX(0b010) | MPU_RASR_SRD(0b10000000) | MPU_RASR_SIZE(18) | MPU_RASR_ENABLE;
	MPU_CTRL = MPU_CTRL_ENABLE;
	asm volatile("dsb");
	asm volatile("isb");

	// Enable the SYSCFG module.
	rcc_enable(APB2, 14);

	// Enable the HSE (8 MHz crystal) oscillator.
	RCC_CR = HSEON // Enable HSE oscillator
		| HSITRIM(16) // Trim HSI oscillator to midpoint
		| HSION; // Enable HSI oscillator for now as we’re still using it
	// Wait for the HSE oscillator to be ready.
	while (!(RCC_CR & HSERDY));
	// Configure the PLL.
	RCC_PLLCFGR = PLLQ(6) // Divide 288 MHz VCO output by 6 to get 48 MHz USB, SDIO, and RNG clock
		| PLLSRC // Use HSE for PLL input
		| PLLP(0) // Divide 288 MHz VCO output by 2 to get 144 MHz SYSCLK
		| PLLN(144) // Multiply 2 MHz VCO input by 144 to get 288 MHz VCO output
		| PLLM(4); // Divide 8 MHz HSE by 4 to get 2 MHz VCO input
	// Enable the PLL.
	RCC_CR |= PLLON; // Enable PLL
	// Wait for the PLL to lock.
	while (!(RCC_CR & PLLRDY));
	// Set up bus frequencies.
	RCC_CFGR = MCO2(2) // MCO2 pin outputs HSE
		| MCO2PRE(0) // Divide 8 MHz HSE by 1 to get 8 MHz MCO2 (must be ≤ 100 MHz)
		| MCO1PRE(0) // Divide 8 MHz HSE by 1 to get 8 MHz MCO1 (must be ≤ 100 MHz)
		| 0 // I2SSRC = 0; I2S module gets clock from PLLI2X
		| MCO1(2) // MCO1 pin outputs HSE
		| RTCPRE(8) // Divide 8 MHz HSE by 8 to get 1 MHz RTC clock (must be 1 MHz)
		| PPRE2(4) // Divide 144 MHz AHB clock by 2 to get 72 MHz APB2 clock (must be ≤ 84 MHz)
		| PPRE1(5) // Divide 144 MHz AHB clock by 4 to get 36 MHz APB1 clock (must be ≤ 42 MHz)
		| HPRE(0) // Divide 144 MHz SYSCLK by 1 to get 144 MHz AHB clock (must be ≤ 168 MHz)
		| SW(0); // Use HSI for SYSCLK for now, until everything else is ready
	// Wait 16 AHB cycles for the new prescalers to settle.
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	// Set Flash access latency to 4 wait states.
	FLASH_ACR = LATENCY(4); // Four wait states (acceptable for 120 ≤ HCLK ≤ 150)
	// Flash access latency change may not be immediately effective; wait until it’s locked in.
	while (LATENCY_X(FLASH_ACR) != 4);
	// Actually initiate the clock switch.
	RCC_CFGR = (RCC_CFGR & ~SW_MSK) | SW(2); // Use PLL for SYSCLK
	// Wait for the clock switch to complete.
	while (SWS_X(RCC_CFGR) != 2);
	// Turn off the HSI now that it’s no longer needed.
	RCC_CR &= ~HSION; // Disable HSI

	// Flush any data in the CPU caches (which are not presently enabled).
	FLASH_ACR |= DCRST // Reset data cache
		| ICRST; // Reset instruction cache
	FLASH_ACR &= ~DCRST // Stop resetting data cache
		& ~ICRST; // Stop resetting instruction cache

	// Turn on the caches.
	// There is an errata that says prefetching doesn’t work on some silicon, but it seems harmless to enable the flag even so.
	FLASH_ACR |= DCEN // Enable data cache
		| ICEN // Enable instruction cache
		| PRFTEN; // Enable prefetching

	// Enable the system configuration registers.
	rcc_enable(APB2, 14);

	// Set SYSTICK to divide by 144 so it overflows every microsecond.
	SCS_STRVR = 144 - 1;
	// Set SYSTICK to run with the core AHB clock.
	SCS_STCSR = CLKSOURCE // Use core clock
		| SCS_STCSR_ENABLE; // Counter is running
	// Reset the counter.
	SCS_STCVR = 0;

	// As we will be running at 144 MHz, switch to the lower-power voltage regulator mode (compatible only up to 144 MHz).
	rcc_enable(APB1, 28);
	PWR_CR &= ~VOS; // Set regulator scale 2
	rcc_disable(APB1, 28);

	// Initialize subsystems.
	buzzer_init();

	// Set up pins.
	rcc_enable_multi(AHB1, 0x0000000F); // Enable GPIOA, GPIOB, GPIOC, and GPIOD modules
	// PA15 = MRF /CS, start deasserted
	// PA14/PA13 = alternate function SWD
	// PA12/PA11 = alternate function OTG FS
	// PA10/PA9/PA8/PA7/PA6 = N/C
	// PA5/PA4 = shorted to VDD
	// PA3 = shorted to VSS
	// PA2 = alternate function TIM2 buzzer
	// PA1/PA0 = shorted to VDD
	GPIOA_ODR = 0b1000000000110011;
	GPIOA_OSPEEDR = 0b01000001010000000000000000000000;
	GPIOA_PUPDR = 0b00100100000000000000000000000000;
	GPIOA_AFRH = 0b00000000000010101010000000000000;
	GPIOA_AFRL = 0b00000000000000000000000100000000;
	GPIOA_MODER = 0b01101010100101010101010101100101;
	// PB15 = N/C
	// PB14 = LED 3
	// PB13 = LED 2
	// PB12 = LED 1
	// PB11/PB10 = N/C
	// PB9/PB8 = shorted to VSS
	// PB7 = MRF /reset, start asserted
	// PB6 = MRF wake, start deasserted
	// PB5 = alternate function MRF MOSI
	// PB4 = alternate function MRF MISO
	// PB3 = alternate function MRF SCK
	// PB2 = BOOT1, hardwired low
	// PB1 = run switch input, analogue
	// PB0 = run switch positive supply, start low
	GPIOB_ODR = 0b0111000000000000;
	GPIOB_OSPEEDR = 0b00000000000000000000010001000000;
	GPIOB_PUPDR = 0b00000000000000000000001000000000;
	GPIOB_AFRH = 0b00000000000000000000000000000000;
	GPIOB_AFRL = 0b00000000010101010101000000000000;
	GPIOB_MODER = 0b01010101010101010101101010011101;
	// PC15/PC14/PC13 = N/C
	// PC12 = MRF INT, input
	// PC11/PC10/PC9/PC8/PC7/PC6 = N/C
	// PC5 = run switch negative supply, always low
	// PC4/PC3/PC2/PC1/PC0 = N/C
	GPIOC_ODR = 0b0000000000000000;
	GPIOC_OSPEEDR = 0b00000000000000000000000000000000;
	GPIOC_PUPDR = 0b00000000000000000000000000000000;
	GPIOC_AFRH = 0b00000000000000000000000000000000;
	GPIOC_AFRL = 0b00000000000000000000000000000000;
	GPIOC_MODER = 0b01010100010101010101010101010101;
	// PD15/PD14/PD13/PD12/PD11/PD10/PD9/PD8/PD7/PD6/PD5/PD4/PD3 = unimplemented on package
	// PD2 = N/C
	// PD1/PD0 = unimplemented on package
	GPIOD_ODR = 0b0000000000000000;
	GPIOD_OSPEEDR = 0b00000000000000000000000000000000;
	GPIOD_PUPDR = 0b00000000000000000000000000000000;
	GPIOD_AFRH = 0b00000000000000000000000000000000;
	GPIOD_AFRL = 0b00000000000000000000000000000000;
	GPIOD_MODER = 0b01010101010101010101010101010101;
	// PE/PF/PG = unimplemented on this package
	// PH15/PH14/PH13/PH12/PH11/PH10/PH9/PH8/PH7/PH6/PH5/PH4/PH3/PH2 = unimplemented on this package
	// PH1 = OSC_OUT (not configured via GPIO registers)
	// PH0 = OSC_IN (not configured via GPIO registers)
	// PI15/PI14/PI13/PI12 = unimplemented
	// PI11/PI10/PI9/PI8/PI7/PI6/PI5/PI4/PI3/PI2/PI1/PI0 = unimplemented on this package

	// Initialize more subsystems.
	estop_init();

	// Wait a bit.
	sleep_ms(100);

	// Turn off LEDs.
	GPIOB_BSRR = GPIO_BR(12) | GPIO_BR(13) | GPIO_BR(14);

	// Initialize USB.
	usb_ll_attach(&handle_usb_reset, &handle_usb_enumeration_done, 0);
	NVIC_ISER[67 / 32] = 1 << (67 % 32); // SETENA67 = 1; enable USB FS interrupt

	// All activity from now on happens in interrupt handlers.
	// Therefore, enable the mode where the chip automatically goes to sleep on return from an interrupt handler.
	SCS_SCR |= SLEEPONEXIT;
	asm volatile("dsb");
	asm volatile("isb");

	// Now wait forever handling activity in interrupt handlers.
	for (;;) {
		asm volatile("wfi");
	}
}

