#ifndef AI_HL_PENALTY_ENEMY_H
#define AI_HL_PENALTY_ENEMY_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {
		/**
		 * Gets the robots to go to their penalty_enemy positions.
		 */
		class PenaltyEnemy {
			public:

				/**
				 * Constructs a new penalty_enemy role.
				 *
				 * \param[in] w the world.
				 */
				PenaltyEnemy(W::World &w);

				/**
				 * Sets all the players
				 *
				 * \param[in] p players.
				 *
				 * \param[in] g goalie, must always exist.
				 */
				void set_players(std::vector<W::Player::Ptr> p, W::Player::Ptr g);


				/**
				 * Runs the AI for one time tick.
				 */
				void tick();

			protected:
				W::World &world;

				std::vector<W::Player::Ptr> players;

				W::Player::Ptr goalie;

				/**
				 * The distance between the penalty mark and the mid point of the two goal posts as described in the rules.
				 */
				const static double PENALTY_MARK_LENGTH;

				/**
				 * The distance between the baseline and the line behind which other robots may stand.
				 */
				const static double RESTRICTED_ZONE_LENGTH;

				/**
				 * Maximum number of robots that can be assigned to this role.
				 */
				const static unsigned int NUMBER_OF_READY_POSITIONS = 4;

				/**
				 * The positions that the robots should move to for this role.
				 */
				Point ready_positions[NUMBER_OF_READY_POSITIONS];
		};
	}
}

#endif

