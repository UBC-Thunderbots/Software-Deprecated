#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * \brief The vendor ID.
 */
#define VENDOR_ID 0x0483

/**
 * \brief The product ID.
 */
#define PRODUCT_ID 0x497C

/**
 * \brief The interface numbers.
 */
enum {
	INTERFACE_RADIO,
	INTERFACE_DFU,
	INTERFACE_COUNT,
};

/**
 * \brief The interface subclass numbers used by the interfaces.
 */
enum {
	SUBCLASS_RADIO = 0x01,
};

/**
 * \brief The alternate setting numbers used by the radio interface.
 */
enum {
	RADIO_ALTSETTING_OFF = 0,
	RADIO_ALTSETTING_NORMAL = 1,
	RADIO_ALTSETTING_PROMISCUOUS = 2,
};

/**
 * \brief The protocol numbers used by the radio interface.
 *
 * These numbers act both to differentiate the alternate settings and also as version numbers; any change to the operation of the alternate setting will increment the protocol number.
 */
enum {
	RADIO_PROTOCOL_OFF = 0x01,
	RADIO_PROTOCOL_NORMAL = 0x41,
	RADIO_PROTOCOL_PROMISCUOUS = 0x81,
};

/**
 * \brief The string indices understood by a GET DESCRIPTOR(String) request.
 */
enum {
	STRING_INDEX_ZERO = 0,
	STRING_INDEX_MANUFACTURER,
	STRING_INDEX_PRODUCT,
	STRING_INDEX_RADIO_OFF,
	STRING_INDEX_NORMAL,
	STRING_INDEX_PROMISCUOUS,
	STRING_INDEX_SERIAL,
};

/**
 * \brief The vendor-specific control requests understood by the dongle.
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
};

/**
 * The delivery status codes reported in message delivery reports.
 */
enum {
	MDR_STATUS_OK = 0,
	MDR_STATUS_NOT_ASSOCIATED = 1,
	MDR_STATUS_NOT_ACKNOWLEDGED = 2,
	MDR_STATUS_NO_CLEAR_CHANNEL = 3,
};

#endif
