#include "primitive.h"
#include "catch.h"
#include "direct_velocity.h"
#include "direct_wheels.h"
#include "dribble.h"
#include "move.h"
#include "pivot.h"
#include "shoot.h"
#include "spin.h"
#include "stop.h"
#include "../chicker.h"
#include "../dr.h"
#include "../dribbler.h"
#include "../receive.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <semphr.h>
#include <stdint.h>

/**
 * \brief The available movement primitives.
 *
 * This array is indexed by movement primitive number. If movement primitives
 * are added or removed, they must be added or removed in this array and it
 * must be kept in the same order as the enumeration in @c
 * software/mrf/constants.h.
 *
 * Make sure stop is always the first primitive.
 */
static const primitive_t * const PRIMITIVES[] = {
	&STOP_PRIMITIVE,
	&MOVE_PRIMITIVE,
	&DRIBBLE_PRIMITIVE,
	&SHOOT_PRIMITIVE,
	&CATCH_PRIMITIVE,
	&PIVOT_PRIMITIVE,
	&SPIN_PRIMITIVE,
	&DIRECT_WHEELS_PRIMITIVE,
	&DIRECT_VELOCITY_PRIMITIVE,
};

/**
 * \brief The number of primitives.
 */
#define PRIMITIVE_COUNT (sizeof(PRIMITIVES) / sizeof(*PRIMITIVES))

/**
 * \brief The mutex that prevents multiple entries into the same primitive at
 * the same time.
 */
static SemaphoreHandle_t primitive_mutex;

/**
 * \brief The primitive that is currently operating.
 */
static const primitive_t *primitive_current;

/**
 * \brief The index number of the current primitive.
 */
static unsigned int primitive_current_index;

/**
 * \brief Initializes the movement primitive manager and all the primitives.
 */
void primitive_init(void) {
	static StaticSemaphore_t primitive_mutex_storage;
	primitive_mutex = xSemaphoreCreateMutexStatic(&primitive_mutex_storage);
	for (size_t i = 0; i != PRIMITIVE_COUNT; ++i) {
		PRIMITIVES[i]->init();
	}
}

/**
 * \brief Starts a new movement.
 *
 * \param[in] primitive the index of the primitive to run
 * \param[in] params the parameters to the primitive
 */
void primitive_start(unsigned int primitive, const primitive_params_t *params) {
	assert(primitive < PRIMITIVE_COUNT);
	xSemaphoreTake(primitive_mutex, portMAX_DELAY);
	if (primitive_current) {
		primitive_current->end();
	}
	chicker_auto_disarm();
	dribbler_set_speed(0);
	dr_reset();
	primitive_current = PRIMITIVES[primitive];
	primitive_current_index = primitive;
	primitive_current->start(params);
	xSemaphoreGive(primitive_mutex);
}

/**
 * \brief Ticks the current primitive.
 *
 * \param[out] log the log record to fill with information about the tick, or
 * \c NULL if no record is to be filled
 */
void primitive_tick(log_record_t *log) {
	xSemaphoreTake(primitive_mutex, portMAX_DELAY);
	dr_tick(log);
	if (log) {
		log->tick.drive_serial = receive_last_serial();
		log->tick.primitive = (uint8_t)primitive_current_index;
	}
	if (primitive_current) {
		primitive_current->tick(log);
	}
	xSemaphoreGive(primitive_mutex);
}

/**
 * \brief Checks whether a particular primitive is direct.
 *
 * \param[in] primitive the primitive to check
 * \retval true the primitive is direct
 * \retval false the primitive is a movement primitive
 */
bool primitive_is_direct(unsigned int primitive) {
	return PRIMITIVES[primitive]->direct;
}

unsigned int get_primitive_index(){
	return primitive_current_index;
}
