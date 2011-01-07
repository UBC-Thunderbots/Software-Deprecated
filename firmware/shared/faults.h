#ifndef SHARED_FAULT_H
#define SHARED_FAULT_H

/**
 * \file
 *
 * \brief Defines the possible fault codes that can be reported by the dongle or a robot.
 */

/**
 * \brief The fault codes that can be reported by either the dongle or a robot.
 */
typedef enum {
	FAULT_XBEE0_FERR,
	FAULT_XBEE1_FERR,
	FAULT_XBEE0_OERR_HW,
	FAULT_XBEE1_OERR_HW,
	FAULT_XBEE0_OERR_SW,
	FAULT_XBEE1_OERR_SW,
	FAULT_XBEE0_CHECKSUM_FAILED,
	FAULT_XBEE1_CHECKSUM_FAILED,
	FAULT_XBEE0_NONZERO_LENGTH_MSB,
	FAULT_XBEE1_NONZERO_LENGTH_MSB,
	FAULT_XBEE0_INVALID_LENGTH_LSB,
	FAULT_XBEE1_INVALID_LENGTH_LSB,
	FAULT_XBEE0_TIMEOUT,
	FAULT_XBEE1_TIMEOUT,
	FAULT_XBEE0_AT_RESPONSE_WRONG_COMMAND,
	FAULT_XBEE1_AT_RESPONSE_WRONG_COMMAND,
	FAULT_XBEE0_AT_RESPONSE_FAILED_UNKNOWN_REASON,
	FAULT_XBEE1_AT_RESPONSE_FAILED_UNKNOWN_REASON,
	FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_COMMAND,
	FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_COMMAND,
	FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_PARAMETER,
	FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_PARAMETER,
	FAULT_XBEE0_RESET_FAILED,
	FAULT_XBEE1_RESET_FAILED,
	FAULT_XBEE0_ENABLE_RTS_FAILED,
	FAULT_XBEE1_ENABLE_RTS_FAILED,
	FAULT_XBEE0_GET_FW_VERSION_FAILED,
	FAULT_XBEE1_GET_FW_VERSION_FAILED,
	FAULT_XBEE0_SET_NODE_ID_FAILED,
	FAULT_XBEE1_SET_NODE_ID_FAILED,
	FAULT_XBEE0_SET_CHANNEL_FAILED,
	FAULT_XBEE1_SET_CHANNEL_FAILED,
	FAULT_XBEE0_SET_PAN_ID_FAILED,
	FAULT_XBEE1_SET_PAN_ID_FAILED,
	FAULT_XBEE0_SET_ADDRESS_FAILED,
	FAULT_XBEE1_SET_ADDRESS_FAILED,
	FAULT_OUT_MICROPACKET_OVERFLOW,
	FAULT_OUT_MICROPACKET_NOPIPE,
	FAULT_OUT_MICROPACKET_BAD_LENGTH,
	FAULT_DEBUG_OVERFLOW,
	FAULT_COMMON_COUNT,
} fault_common_t;

/**
 * \brief The fault codes that can be reported only by the dongle.
 */
typedef enum {
	FAULT_ERROR_QUEUE_OVERFLOW = FAULT_COMMON_COUNT,
	FAULT_SEND_FAILED_ROBOT1,
	FAULT_SEND_FAILED_ROBOT2,
	FAULT_SEND_FAILED_ROBOT3,
	FAULT_SEND_FAILED_ROBOT4,
	FAULT_SEND_FAILED_ROBOT5,
	FAULT_SEND_FAILED_ROBOT6,
	FAULT_SEND_FAILED_ROBOT7,
	FAULT_SEND_FAILED_ROBOT8,
	FAULT_SEND_FAILED_ROBOT9,
	FAULT_SEND_FAILED_ROBOT10,
	FAULT_SEND_FAILED_ROBOT11,
	FAULT_SEND_FAILED_ROBOT12,
	FAULT_SEND_FAILED_ROBOT13,
	FAULT_SEND_FAILED_ROBOT14,
	FAULT_SEND_FAILED_ROBOT15,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT1,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT2,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT3,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT4,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT5,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT6,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT7,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT8,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT9,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT10,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT11,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT12,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT13,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT14,
	FAULT_IN_MICROPACKET_OVERFLOW_ROBOT15,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT1,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT2,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT3,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT4,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT5,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT6,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT7,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT8,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT9,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT10,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT11,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT12,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT13,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT14,
	FAULT_IN_MICROPACKET_NOPIPE_ROBOT15,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT1,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT2,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT3,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT4,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT5,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT6,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT7,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT8,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT9,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT10,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT11,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT12,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT13,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT14,
	FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT15,
} fault_dongle_t;

/**
 * \brief The fault codes that can be reported only by a robot.
 */
typedef enum {
	FAULT_CAPACITOR_CHARGE_TIMEOUT = FAULT_COMMON_COUNT,
	FAULT_CHICKER_COMM_ERROR,
	FAULT_CHICKER_NOT_PRESENT,
	FAULT_FPGA_NO_BITSTREAM,
	FAULT_FPGA_INVALID_BITSTREAM,
	FAULT_FPGA_DCM_LOCK_FAILED,
	FAULT_FPGA_ONLINE_CRC_FAILED,
	FAULT_FPAG_COMM_ERROR,
	FAULT_OSCILLATOR_FAILED,
	FAULT_MOTOR1_HALL_000,
	FAULT_MOTOR2_HALL_000,
	FAULT_MOTOR3_HALL_000,
	FAULT_MOTOR4_HALL_000,
	FAULT_MOTORD_HALL_000,
	FAULT_MOTOR1_HALL_111,
	FAULT_MOTOR2_HALL_111,
	FAULT_MOTOR3_HALL_111,
	FAULT_MOTOR4_HALL_111,
	FAULT_MOTORD_HALL_111,
	FAULT_MOTOR1_HALL_STUCK,
	FAULT_MOTOR2_HALL_STUCK,
	FAULT_MOTOR3_HALL_STUCK,
	FAULT_MOTOR4_HALL_STUCK,
	FAULT_MOTORD_HALL_STUCK,
	FAULT_MOTOR1_ENCODER_STUCK,
	FAULT_MOTOR2_ENCODER_STUCK,
	FAULT_MOTOR3_ENCODER_STUCK,
	FAULT_MOTOR4_ENCODER_STUCK,
	FAULT_MOTORD_ENCODER_STUCK,
	FAULT_DRIBBLER_OVERHEAT,
	FAULT_DRIBBLER_THERMISTOR_FAILED,
	FAULT_FLASH_COMM_ERROR,
	FAULT_NO_PARAMS,
	FAULT_IN_PACKET_OVERFLOW,
	FAULT_FIRMWARE_BAD_REQUEST,
	FAULT_ROBOT_COUNT,
} fault_robot_t;

#endif

