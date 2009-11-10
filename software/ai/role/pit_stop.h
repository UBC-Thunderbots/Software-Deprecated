#ifndef AI_ROLE_PIT_STOP_H
#define AI_ROLE_PIT_STOP_H

#include "ai/role.h"
#include <vector>
#include "ai/tactic/move.h"
#include "ai/tactic.h"
#include "geom/point.h"

//
// Gets the robots to go to their pit_stop positions.
//
class pit_stop : public role {
	public:
		//
		// A pointer to a pit_stop role.
		//
		typedef Glib::RefPtr<pit_stop> ptr;

		//
		// Constructs a new pit_stop role.
		//
		pit_stop(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

                void set_robots(const std::vector<player::ptr> &robots);

	protected:
                std::vector<move::ptr> the_tactics;	
};

#endif

