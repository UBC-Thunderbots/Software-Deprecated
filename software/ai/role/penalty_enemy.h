#ifndef AI_ROLE_PREPARE_PENALTY_ENEMY_H
#define AI_ROLE_PREPARE_PENALTY_ENEMY_H

#include <vector>
#include "ai/role/role.h"
#include "ai/tactic/move.h"

//
// Gets the robots to go to their enemy penalty kick positions.
//
class PenaltyEnemy : public Role {
	public:
		//
		// A pointer to a PenaltyEnemy Role.
		//
		typedef Glib::RefPtr<PenaltyEnemy> ptr;

		//
		// Constructs a new PenaltyEnemy Role.
		//
		PenaltyEnemy(World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();

		void players_changed();

	protected:
		const World::ptr the_world;

		/**
		 * The distance between the baseline and the line behind which the robots must stand.
		 */
		const static double RESTRICTED_ZONE_LENGTH = 0.85;

		/**
		* Maximum number of positions that can be assigned for this Role.
		*/
		const static unsigned int NUM_POSITIONS = 4;

		/**
		* The designated standing positions for this Role.
		*/
		Point standing_positions[NUM_POSITIONS];
};

class PenaltyGoalie : public Role {
	public:
		typedef Glib::RefPtr<PenaltyGoalie> ptr;

		PenaltyGoalie(World::ptr world);

		void tick();

		void players_changed();

	protected:
		const World::ptr the_world;
};

#endif

