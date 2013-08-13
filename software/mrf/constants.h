#ifndef MRF_CONSTANTS_H
#define MRF_CONSTANTS_H

/**
 * \brief The vendor ID used by the dongle.
 */
#define MRF_DONGLE_VID 0x0483

/**
 * \brief The product ID used by the dongle.
 */
#define MRF_DONGLE_PID 0x497C

/**
 * \brief The interface subclass number used by the dongle in normal mode.
 */
#define MRF_DONGLE_NORMAL_SUBCLASS 0

/**
 * \brief The interface subclass number used by the dongle in promiscuous mode.
 */
#define MRF_DONGLE_PROMISCUOUS_SUBCLASS 0

/**
 * \brief The interface protocol number used by the burner in normal mode.
 *
 * This number acts as a version number and will change if incompatible protocol changes are made, thus ensuring software and firmware match capabilities.
 */
#define MRF_DONGLE_NORMAL_PROTOCOL 0

/**
 * \brief The interface protocol number used by the burner in promiscuous mode.
 *
 * This number acts as a version number and will change if incompatible protocol changes are made, thus ensuring software and firmware match capabilities.
 */
#define MRF_DONGLE_PROMISCUOUS_PROTOCOL 0

/**
 * \brief The string indices understood by a GET DESCRIPTOR(String) request.
 */
enum {
	STRING_INDEX_ZERO = 0,
	STRING_INDEX_MANUFACTURER,
	STRING_INDEX_PRODUCT,
	STRING_INDEX_CONFIG1,
	STRING_INDEX_CONFIG2,
	STRING_INDEX_CONFIG3,
	STRING_INDEX_CONFIG4,
	STRING_INDEX_CONFIG5,
	STRING_INDEX_SERIAL,
};

/**
 * \brief The vendor-specific control requests understood by the dongle.
 */
enum {
	CONTROL_REQUEST_GET_CHANNEL = 0,
	CONTROL_REQUEST_SET_CHANNEL,
	CONTROL_REQUEST_GET_SYMBOL_RATE,
	CONTROL_REQUEST_SET_SYMBOL_RATE,
	CONTROL_REQUEST_GET_PAN_ID,
	CONTROL_REQUEST_SET_PAN_ID,
	CONTROL_REQUEST_GET_MAC_ADDRESS,
	CONTROL_REQUEST_SET_MAC_ADDRESS,
	CONTROL_REQUEST_GET_ADDRESS_MAP_ENTRY,
	CONTROL_REQUEST_SET_ADDRESS_MAP_ENTRY,
	CONTROL_REQUEST_GET_PROMISCUOUS_FLAGS,
	CONTROL_REQUEST_SET_PROMISCUOUS_FLAGS,
	CONTROL_REQUEST_BEEP,
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

#endif

