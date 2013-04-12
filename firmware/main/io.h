#ifndef IO_H
#define IO_H

/**
 * \file
 *
 * \brief Provides access to hardware I/O ports
 *
 * For each port, the purpose of each bit is defined.
 * Reserved bits should be maintained at zero.
 * The value in square brackets is the power-up default value.
 *
 * Legend:
 * R - bit is readable
 * W - bit is generally writeable
 * S - bit is writeable to 1 in software; writing 0 has no effect
 * C - bit is writeable to 0 in software; writing 1 has no effect
 */

#include <stdint.h>

#define IO_PORT(x) (*(volatile uint8_t *) ((x) + 32))

/**
 * \brief Controls the operation of the LEDs on the mainboard
 *
 * Bits:
 * 7 (R/W) [0]: Radio LED control; 1 = on, 0 = off
 * 6 (R/S) [0]: Radio LED blink; 1 = blink, 0 = no blink; cleared by hardware
 * 5 (R/W) [1]: Test LED signal source; 1 = direct software control, 0 = hardware source
 * 4–0 [00000]: (R/W): Test LED value:
 *   If bit 5 = 1:
 *     4–3: Reserved
 *     2–0: Test LED values, 1 = LED on, 0 = LED off
 *   If bit 5 = 0:
 *     00000 = LEDs display Hall sensors for wheel 0
 *     00001 = LEDs display Hall sensors for wheel 1
 *     00010 = LEDs display Hall sensors for wheel 2
 *     00011 = LEDs display Hall sensors for wheel 3
 *     00100 = LEDs display Hall sensors for dribbler
 *     00101 = LEDs display optical encoder for wheel 0
 *     00110 = LEDs display optical encoder for wheel 1
 *     00111 = LEDs display optical encoder for wheel 2
 *     01000 = LEDs display optical encoder for wheel 3
 *     Others: Reserved
 */
#define LED_CTL IO_PORT(0x00)

/**
 * \brief Controls power to various parts of the robot
 *
 * Bits:
 * 7–4: Reserved
 * 3 (R/W) [0]: Laser power; 1 = lit, 0 = unlit
 * 2: Reserved
 * 1 (R/W) [0]: Motor power; 1 = run, 0 = power down
 * 0 (R/W) [1]: Logic power; 1 = run, 0 = power down
 */
#define POWER_CTL IO_PORT(0x01)

/**
 * \brief A real-time timer counting 5 millisecond ticks
 *
 * Bits:
 * 7–0 (R) [00000000]: Count of ticks since power-up
 */
#define TICKS IO_PORT(0x02)

/**
 * \brief Selects which motor will be controlled.
 *
 * The value in this register controls which registers are viewed by the MOTOR_CTL, MOTOR_STATUS, and MOTOR_PWM locations.
 *
 * Bits:
 * 7–0 (R/W) [00000000]: Motor index
 *
 * Values:
 * 0: Motor registers control wheel 0
 * 1: Motor registers control wheel 1
 * 2: Motor registers control wheel 2
 * 3: Motor registers control wheel 3
 * 4: Motor registers control dribbler
 */
#define MOTOR_INDEX IO_PORT(0x03)

/**
 * \brief Controls the overall operation of the motor.
 *
 * Bits:
 * 7–6 (R/W) [00]: Phase 2 control
 * 5–4 (R/W) [00]: Phase 1 control
 * 3–2 (R/W) [00]: Phase 0 control
 * 1 (R/W) [0]: Automatic commutation
 * 0 (R/W) [0]: Direction
 *
 * When automatic commutation is enabled, the Direction bit controls the direction of motor power and the Phase bits are ignored.
 * When automatic commutation is disabled, the Direction bit is ignored and the Phase bits control the motor phases directly.
 *
 * Direction values:
 * 0: Forward
 * 1: Reverse
 *
 * Phase values:
 * 00: Float
 * 01: PWM
 * 10: Low
 * 11: High
 *
 * Unless interlocks are overridden, the upper six bits can only be 000000 or 101010.
 * Any write that does not satisfy this constraint will result in these bits being set to 000000.
 * The same will happen if interlocks are re-enabled while an illegal value is set.
 */
#define MOTOR_CTL IO_PORT(0x04)

/**
 * \brief Reports the status of the motor.
 *
 * Bits:
 * 7–2: Reserved
 * 1 (R/C) [0]: Hall sensors observed all high; 1 = failure observed, 0 = failure not observed
 * 0 (R/C) [0]: Hall sensors observed all low; 1 = failure observed, 0 = failure not observed
 */
#define MOTOR_STATUS IO_PORT(0x05)

/**
 * \brief The PWM duty cycle for the motor.
 *
 * Bits:
 * 7–0 (R/W) [00]: Duty cycle
 */
#define MOTOR_PWM IO_PORT(0x06)

/**
 * \brief Used in simulation only to move magic values from firmware to testbench
 *
 * A write sets the magic value.
 * A read returns the most recently written value.
 */
#define SIM_MAGIC IO_PORT(0x07)

/**
 * \brief Reports the accumulated count of optical encoder ticks
 *
 * A write to this register selects an optical encoder (0–3) and simultaneously snapshots and clears the accumulated count.
 * A read from this register returns the LSB of the snapshot value.
 */
#define ENCODER_LSB IO_PORT(0x0C)

/**
 * \brief Reports the accumulated count of optical encoder ticks
 *
 * A read from this register reports the MSB of the snapshot value taken by a write to \ref ENCODER_LSB.
 */
#define ENCODER_MSB IO_PORT(0x0D)

/**
 * \brief Reports encoder commutation failures
 *
 * Bits:
 * 7–4: Reserved
 * 3 (R/C) [0]: Encoder 3 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 * 2 (R/C) [0]: Encoder 2 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 * 1 (R/C) [0]: Encoder 1 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 * 0 (R/C) [0]: Encoder 0 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 */
#define ENCODER_FAIL IO_PORT(0x0E)

/**
 * \brief Reports mainboard analogue-to-digital converter readings
 *
 * A write to this register selects a channel (0–7) and snapshots the most recent conversion result for the channel.
 * A read from this register returns the LSB of the snapshot value.
 */
#define ADC_LSB IO_PORT(0x0F)

/**
 * \brief Reports mainboard analogue-to-digital converter readings
 *
 * A read from this register reports the MSB of the snapshot value taken by a write to \ref ADC_LSB.
 */
#define ADC_MSB IO_PORT(0x10)

/**
 * \brief Controls and reports on the chicker subsystem
 *
 * Bits:
 * 7–6: Reserved
 * 5 (R/W) [0]: Activates the safe discharge circuit to discharge the capacitors; 1 = enabled, 0 = disabled
 * 4 (R) [0]: Indicates whether the capacitors are fully charged; 1 = charged, 0 = not charged
 * 3 (R) [0]: Indicates whether charging timed out; 1 = timeout detected, 0 = timeout not detected
 * 2 (R/S) [0]: Fires the chipper; 1 = fire, 0 = do not fire, cleared in hardware at end of pulse
 * 1 (R/S) [0]: Fires the kicker; 1 = fire, 0 = do not fire, cleared in hardware at end of pulse
 * 0 (R/W) [0]: Enables the charger; 1 = charge, 0 = do not charge
 */
#define CHICKER_CTL IO_PORT(0x13)

/**
 * \brief Sets the chicker firing pulse width
 *
 * The actual width of the generated pulse is (N + 1) ÷ 4 microseconds, where N is the value loaded into the registers.
 *
 * Bits:
 * 7–0 (R/W) [00000000]: LSB of pulse width setting
 */
#define CHICKER_PULSE_LSB IO_PORT(0x14)

/**
 * \brief Sets the chicker firing pulse width
 *
 * Bits:
 * 7–0 (R/W) [00000000]: MSB of pulse width setting
 */
#define CHICKER_PULSE_MSB IO_PORT(0x15)

/**
 * \brief Controls and reports status of the SPI Flash
 *
 * The chip select pin must not be asserted unless the debug port is disabled.
 *
 * Bits:
 * 7–2: Reserved
 * 1 (R/W) [1]: Sets the level on the /CS pin
 * 0 (R) [0]: Indicates whether an SPI transaction is in progress; 1 = busy, 0 = idle
 */
#define FLASH_CTL IO_PORT(0x16)

/**
 * \brief Reads and writes data on the SPI Flash bus
 *
 * A transaction must not be started unless the debug port is disabled.
 *
 * On write, starts an SPI transaction outputting the written byte
 * On read (when FLASH_CTL<0> = 0), returns the most recent byte read from the bus
 */
#define FLASH_DATA IO_PORT(0x17)

/**
 * \brief Controls and reports status of the MRF24J40
 *
 * Bits:
 * 7–5: Reserved
 * 4 (R): Reports the level on the INT pin
 * 3 (R/W) [0]: Sets the level on the WAKE pin
 * 2 (R/W) [1]: Sets the level on the /RESET pin
 * 1 (R/W) [1]: Sets the level on the /CS pin
 * 0 (R) [0]: Indicates whether an SPI transaction is in progress; 1 = busy, 0 = idle
 */
#define MRF_CTL IO_PORT(0x18)

/**
 * \brief Reads and writes data on the MRF24J40 SPI bus
 *
 * On write, starts an SPI transaction outputting the written byte
 * On read (when MRF_CTL<0> = 0), returns the most recent byte read from the bus
 */
#define MRF_DATA IO_PORT(0x19)

/**
 * \brief Controls the lateral position sensor
 *
 * Bits:
 * 7–4: Reserved
 * 3–0 (R/W) [0]: Drive LPS LEDs; 1 = LED lit, 0 = LED dark
 */
#define LPS_CTL IO_PORT(0x1B)

/**
 * \brief Holds the least significant byte of the device DNA.
 *
 * Bits:
 * 7–0 (R): The byte
 */
#define DEVICE_ID0 IO_PORT(0x1C)
#define DEVICE_ID1 IO_PORT(0x1D)
#define DEVICE_ID2 IO_PORT(0x1E)
#define DEVICE_ID3 IO_PORT(0x1F)
#define DEVICE_ID4 IO_PORT(0x20)
#define DEVICE_ID5 IO_PORT(0x21)
#define DEVICE_ID6 IO_PORT(0x22)

/**
 * \brief Status of the device dna registers
 *
 * 7-1: Reserved
 * 0 (R) [0]: Valid ID; 1 = Device DNA is valid, 0 = Device DNA not ready
 */
#define DEVICE_ID_STATUS IO_PORT(0x23)

/**
 * \brief Linear feedback shift register control register
 *
 * 7-1: Reserved
 * 0 (R/W) [1]: A write produces an increment of the LFSR, a read provides the LSb of its current value
 */
#define LFSR IO_PORT(0x24)

/**
 * \brief Controls and reports the status of the debug port
 *
 * The debug port must not be enabled unless the flash memory SPI port is idle and chip select is deasserted.
 *
 * 7–2: Reserved
 * 1 (R) [0]: Indicates whether a debug port transaction is in progress; 1 = busy, 0 = idle
 * 0 (R/W) [0]: Controls whether the debug port is enabled; 1 = enabled, 0 = idle
 */
#define DEBUG_CTL IO_PORT(0x25)

/**
 * \brief Starts a transaction on the debug port
 *
 * A write to this register sends the written value over the debug port.
 *
 * The debug port must be enabled before starting a transaction.
 * Writes to this register while the port is either disabled or busy will be discarded.
 */
#define DEBUG_DATA IO_PORT(0x26)

/**
 * \brief Controls and reports the status of the internal configuration access port
 *
 * 7–1: Reserved
 * 0 (R) [0]: Indicates whether the ICAP is busy; 1 = busy, 0 = idle
 */
#define ICAP_CTL IO_PORT(0x27)

/**
 * \brief Starts a transaction on the FPGA’s internal configuration access port
 *
 * A write to this register initiates a transaction using the written value along with the value most recently written to \ref ICAP_MSB.
 * A read returns the most recently written value.
 */
#define ICAP_LSB IO_PORT(0x28)

/**
 * \brief Buffers the MSB of the next ICAP transaction
 *
 * A write sets the buffer value.
 * A read returns the most recently written value.
 */
#define ICAP_MSB IO_PORT(0x29)

/**
 * \brief The LSB of the stack pointer
 */
#define SP_LSB IO_PORT(0x3D)

/**
 * \brief The MSB of the stack pointer
 */
#define SP_MSB IO_PORT(0x3E)

/**
 * \brief The status register
 */
#define SREG IO_PORT(0x3F)

#endif

