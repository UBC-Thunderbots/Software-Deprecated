#ifndef AI_TACTIC_RECEIVE_H
#define AI_TACTIC_RECEIVE_H

#include "ai/tactic/tactic.h"
#include "ai/navigator/robot_navigator.h"

/**
 * The robot tries to ensure there is clear line of sight to the ball.
 * Otherwise it tries to move until it finds one.
 * Perhaps watch out for other robots trying to block its line of sight.
 * NOTE: Unlike pass_receive, this does not require knowledge of the passer.
 */
class receive : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<receive> ptr;

		//
		// Constructs a new receive tactic. 
		//
		receive(player::ptr player, world::ptr world);
		
		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		const world::ptr the_world;
		robot_navigator navi;
};

#endif

