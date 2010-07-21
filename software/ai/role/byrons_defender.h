#ifndef AI_ROLE_BYRONS_DEFENDER_H
#define AI_ROLE_BYRONS_DEFENDER_H

#include "ai/role/role.h"
#include "ai/tactic/tactic.h"
#include <vector>

class ByronsDefender : public Role {
	public:
		//
		// A pointer to a ByronsDefender role.
		//
		typedef RefPtr<ByronsDefender> Ptr;

		//
		// Constructs a new ByronsDefender role.
		//
		ByronsDefender(World::Ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the Robot membership.
		//
		void players_changed();

	protected:
	
		const World::Ptr the_world;

};

#endif

