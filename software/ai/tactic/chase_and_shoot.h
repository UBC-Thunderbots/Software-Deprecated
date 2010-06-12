#ifndef AI_TACTIC_CHASE_AND_SHOOT_H
#define AI_TACTIC_CHASE_AND_SHOOT_H

#include "ai/world/world.h"
#include "ai/tactic/tactic.h"
#include "geom/point.h"

/**
 * Chases after the ball, and aims by doing a pivoting turn.
 */
class chase_and_shoot : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<chase_and_shoot> ptr;

		/**
		 * Set a target that robot would like to aim the ball
		 * after gaining possesion.
		 */
		void set_target(const point& t) {
			target = t;
		}

		//
		// Constructs a new chase tactic. 
		//
		chase_and_shoot(player::ptr player, world::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:

		const world::ptr the_world;
		point target;
};

#endif

