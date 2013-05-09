#ifndef FW_FB_CONSTANTS_H
#define FW_FB_CONSTANTS_H

/**
 * \brief The vendor ID used by the dongle.
 */
#define FLASH_BURNER_VID 0x0483

/**
 * \brief The product ID used by the dongle.
 */
#define FLASH_BURNER_PID 0x497D

enum {
	STRING_INDEX_ZERO = 0,
	STRING_INDEX_MANUFACTURER,
	STRING_INDEX_PRODUCT,
	STRING_INDEX_CONFIG1,
	STRING_INDEX_CONFIG2,
	STRING_INDEX_CONFIG3,
	STRING_INDEX_CONFIG4,
	STRING_INDEX_SERIAL,
};

enum {
	CONTROL_REQUEST_READ_IO,
	CONTROL_REQUEST_WRITE_IO,
	CONTROL_REQUEST_JEDEC_ID,
	CONTROL_REQUEST_READ_STATUS,
	CONTROL_REQUEST_READ,
	CONTROL_REQUEST_WRITE,
	CONTROL_REQUEST_ERASE,
	CONTROL_REQUEST_GET_ERRORS,
};

enum {
	SELECT_TARGET_STATUS_OK = 0,
	SELECT_TARGET_STATUS_UNRECOGNIZED_JEDEC_ID = 1,
};

#endif
