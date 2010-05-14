#ifndef AI_TACTIC_PASS_H
#define AI_TACTIC_PASS_H

#include "ai/tactic.h"
#include "ai/tactic/kick.h"
#include "ai/tactic/move.h"
#include "ai/tactic/pass_mode.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class pass : public tactic {
	public:

		//
		// A pointer to a pass tactic.
		//
		typedef Glib::RefPtr<pass> ptr;

		//
		// Constructs a new pass tactic.
		//
		pass(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player, player::ptr receiver);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

	protected:
		
		kick::ptr kick_tactic;

		move::ptr move_tactic;

		// Pointer to the receiver of the pass.
		player::ptr the_receiver;

		// The distance assumed that opponent robots may travel to intercept the pass.
		static const double INTERCEPT_RADIUS = 0.1;

		// The threshold speed for the receiver to be considered not moving.
		static const double SPEED_THRESHOLD = 0.5;
};

#endif

