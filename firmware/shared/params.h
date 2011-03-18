#ifndef SHARED_PARAMS_H
#define SHARED_PARAMS_H

/**
 * \file
 *
 * \brief Provides the operational parameters block.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * \brief The possible contents of the SPI flash.
 */
typedef enum {
	/**
	 * \brief The flash contains an FPGA bitstream.
	 */
	FLASH_CONTENTS_FPGA,

	/**
	 * \brief The flash contains a PIC firmware upgrade.
	 */
	FLASH_CONTENTS_PIC,

	/**
	 * \brief The flash contains no useful data.
	 */
	FLASH_CONTENTS_NONE,
} flash_contents_t;

/**
 * \brief The type of the operational parameters block.
 */
typedef struct {
	/**
	 * \brief The contents of the SPI flash chip.
	 */
	flash_contents_t flash_contents;

	/**
	 * \brief The XBee radio channels.
	 */
	uint8_t xbee_channels[2];

	/**
	 * \brief The robot number.
	 */
	uint8_t robot_number;

	/**
	 * \brief The power level to send to the dribbler when it's active.
	 */
	uint8_t dribble_power;

	/**
	 * \brief A padding byte because parameters must be an even length.
	 */
	uint8_t reserved;
} params_t;

#endif

