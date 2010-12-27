#ifndef DONGLE_STATUS_H
#define DONGLE_STATUS_H

#include <stdint.h>

/**
 * \brief The possible states the emergency stop switch can be in.
 */
typedef enum {
	/**
	 * \brief The switch has not been sampled yet.
	 */
	ESTOP_STATE_UNINITIALIZED = 0,

	/**
	 * \brief The switch is not plugged in or is not working properly.
	 */
	ESTOP_STATE_DISCONNECTED = 1,

	/**
	 * \brief The switch is in the stop position.
	 */
	ESTOP_STATE_STOP = 2,

	/**
	 * \brief The switch is in the run position.
	 */
	ESTOP_STATE_RUN = 3,
} estop_state_t;

/**
 * \brief The possible states the XBees can be in.
 */
typedef enum {
	/**
	 * \brief Stage 1 initialization has not yet started.
	 */
	XBEES_STATE_PREINIT,

	/**
	 * \brief XBee 0 is in stage 1 initialization.
	 */
	XBEES_STATE_INIT1_0,

	/**
	 * \brief XBee 1 is in stage 1 initialization.
	 */
	XBEES_STATE_INIT1_1,

	/**
	 * \brief The XBees have completed stage 1 initialization and are awaiting channel assignments.
	 */
	XBEES_STATE_INIT1_DONE,

	/**
	 * \brief XBee 0 is in stage 2 initialization.
	 */
	XBEES_STATE_INIT2_0,

	/**
	 * \brief XBee 1 is in stage 2 initialization.
	 */
	XBEES_STATE_INIT2_1,

	/**
	 * \brief The XBees have completed all initialization and are communicating normally.
	 */
	XBEES_STATE_RUNNING,

	/**
	 * \brief XBee 0 failed.
	 */
	XBEES_STATE_FAIL_0,

	/**
	 * \brief XBee 1 failed.
	 */
	XBEES_STATE_FAIL_1,
} xbees_state_t;

/**
 * \brief The layout of a dongle status packet.
 */
typedef struct {
	/**
	 * \brief The current state of the emergency stop switch.
	 */
	estop_state_t estop;

	/**
	 * \brief The states of the XBees.
	 */
	xbees_state_t xbees;

	/**
	 * \brief The mask of robot responsiveness.
	 */
	uint16_t robots;
} dongle_status_t;

/**
 * \brief The dongle status block that application components should write to.
 */
extern volatile dongle_status_t dongle_status;

/**
 * \brief Starts reporting dongle status updates over USB.
 */
void dongle_status_start(void);

/**
 * \brief Stops reporting dongle status updates over USB.
 */
void dongle_status_stop(void);

/**
 * \brief Halts the dongle status endpoint.
 */
void dongle_status_halt(void);

/**
 * \brief Unhalts the dongle status endpoint.
 */
void dongle_status_unhalt(void);

/**
 * \brief This function must be called after the application writes to dongle_status.
 */
void dongle_status_dirty(void);

#endif

