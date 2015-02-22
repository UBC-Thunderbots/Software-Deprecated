#ifndef MRF_CONSTANTS_H
#define MRF_CONSTANTS_H

#include <stdint.h>

namespace MRF {
	/**
	 * \brief The vendor ID used by the dongle.
	 */
	constexpr uint16_t VENDOR_ID = 0x0483;

	/**
	 * \brief The product ID used by the dongle.
	 */
	constexpr uint16_t PRODUCT_ID = 0x497C;

	/**
	 * \brief The interface subclass number used by the radio interface.
	 */
	constexpr uint8_t SUBCLASS = 0x01;

	/**
	 * \brief The interface protocol number used by the dongle in radio off mode.
	 */
	constexpr uint8_t PROTOCOL_OFF = 0x01;

	/**
	 * \brief The interface protocol number used by the dongle in normal mode.
	 */
	constexpr uint8_t PROTOCOL_NORMAL = 0x41;

	/**
	 * \brief The interface protocol number used by the dongle in promiscuous mode.
	 */
	constexpr uint8_t PROTOCOL_PROMISCUOUS = 0x81;

	/**
	 * \brief The vendor-specific control requests understood by the radio interface.
	 */
	enum {
		CONTROL_REQUEST_GET_CHANNEL = 0x00,
		CONTROL_REQUEST_SET_CHANNEL = 0x01,
		CONTROL_REQUEST_GET_SYMBOL_RATE = 0x02,
		CONTROL_REQUEST_SET_SYMBOL_RATE = 0x03,
		CONTROL_REQUEST_GET_PAN_ID = 0x04,
		CONTROL_REQUEST_SET_PAN_ID = 0x05,
		CONTROL_REQUEST_GET_MAC_ADDRESS = 0x06,
		CONTROL_REQUEST_SET_MAC_ADDRESS = 0x07,
		CONTROL_REQUEST_GET_PROMISCUOUS_FLAGS = 0x0A,
		CONTROL_REQUEST_SET_PROMISCUOUS_FLAGS = 0x0B,
		CONTROL_REQUEST_BEEP = 0x0C,
		CONTROL_REQUEST_READ_CORE = 0x0D,
		CONTROL_REQUEST_READ_BUILD_ID = 0x0E,
	};

	/**
	 * The delivery status codes reported in message delivery reports.
	 */
	enum {
		MDR_STATUS_OK,
		MDR_STATUS_NOT_ASSOCIATED,
		MDR_STATUS_NOT_ACKNOWLEDGED,
		MDR_STATUS_NO_CLEAR_CHANNEL,
	};
}

#endif

