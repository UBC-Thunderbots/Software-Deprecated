#ifndef AI_ROLE_PENALTY_FRIENDLY_H
#define AI_ROLE_PENALTY_FRIENDLY_H

#include "ai/role/role.h"

/*
 * Does a penalty friendly
 */
class PenaltyFriendly : public Role {
	public:
		//
		// Constructs a new PenaltyFriendly Role.
		//
		PenaltyFriendly(RefPtr<World> world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		//
		// Handles changes to the robot membership.
		//
		void robots_changed();

	protected:
		const RefPtr<World> the_world;

		/**
		 * The distance between the penalty mark and the mid point of the two goal posts as described in the rules.
		 */
		const static double PENALTY_MARK_LENGTH = 0.45;

		/**
		 * The distance between the baseline and the line behind which other robots may stand.
		 */
		const static double RESTRICTED_ZONE_LENGTH = 0.85;

		//std::vector<RefPtr<Tactic> > the_tactics;

		/**
		 * Maximum number of robots that can be assigned to this Role.
		 */
		const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

		/**
		 * The positions that the robots should move to for this Role.
		 */
		Point ready_positions[NUMBER_OF_READY_POSITIONS];
};

#endif

