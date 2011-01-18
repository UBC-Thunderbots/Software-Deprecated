#ifndef SHARED_FEEDBACK_H
#define SHARED_FEEDBACK_H

/**
 * \file
 *
 * \brief Provides the layout of the feedback state block.
 */

#include "faults.h"
#include <stdint.h>

/**
 * \brief The layout of the flags byte of the feedback state block.
 */
typedef struct {
	/**
	 * \brief Always one to indicate the feedback block is valid.
	 */
	unsigned valid : 1;

	/**
	 * \brief Zero if the ball is not in the break beam, or one if it is.
	 */
	unsigned ball_in_beam : 1;

	/**
	 * \brief Zero if the ball is not loading the dribbler motor, or one if it is.
	 */
	unsigned ball_on_dribbler : 1;

	/**
	 * \brief Zero if the capacitor is not fully charged, or one if it is.
	 */
	unsigned capacitor_charged : 1;
} feedback_flags_t;

/**
 * \brief The layout of the feedback state block.
 */
typedef struct {
	/**
	 * \brief Miscellaneous flags.
	 */
	feedback_flags_t flags;

	/**
	 * \brief The raw ADC reading of the battery voltage monitor.
	 */
	uint16_t battery_voltage_raw;

	/**
	 * \brief The raw ADC reading of the capacitor voltage monitor.
	 */
	uint16_t capacitor_voltage_raw;

	/**
	 * \brief The raw ADC reading of the dribbler thermistor.
	 */
	uint16_t dribbler_temperature_raw;

	/**
	 * \brief The bitmask of latched faults.
	 */
	uint8_t faults[(FAULT_ROBOT_COUNT + 7) / 8];
} feedback_block_t;

#endif

