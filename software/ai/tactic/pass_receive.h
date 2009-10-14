#ifndef AI_TACTIC_PASS_RECEIVE_H
#define AI_TACTIC_PASS_RECEIVE_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"

//
// A tactic controls the operation of a single player doing some activity.
//
class pass_receive : public tactic {
	public:

		//
		// Constructs a new pass receive tactic. 
		//
		pass_receive(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		void set_passer(player::ptr p) {
			passer = p;
		}		
		
		//
		// Runs the AI for one time tick.
		//
		void update();	

	protected:
		
		// Pointer to the passer of the pass.
		player::ptr passer;
};

#endif

