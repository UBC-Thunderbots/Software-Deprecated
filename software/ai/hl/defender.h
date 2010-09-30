#ifndef AI_HL_ROLE_DEFENDER_H
#define AI_HL_ROLE_DEFENDER_H

#include "ai/hl/world.h"
#include <vector>

namespace AI {
	namespace HL {

		/**
		 * Combined goalie and defender
		 *
		 * Will not chase the ball, unless set_chase is set to true.
		 * Unless the ball is already in the defence area.
		 */
		class Defender {
			public:
				Defender(W::World& w);

				/**
				 * Resets all the players.
				 * This is the easiest way to handle player changes.
				 * Perhaps have something more advanced in the future.
				 * \param[in] p defenders
				 * \param[in] g goalie, must always exist
				 */
				void set_players(std::vector<W::Player::Ptr> p, W::Player::Ptr g);

				/**
				 * Finds a player that you can extract and make use.
				 * \return NULL if there is no player you can remove.
				 */
				W::Player::Ptr remove_player();

				/**
				 * This function can only be called ONCE per tick.
				 */
				void tick();

				/**
				 * Allows a player to chase the ball.
				 */
				void set_chase(const bool b) {
					chase = b;
				}

			protected:

				/**
				 * Calculate points which should be used to defend
				 * from enemy robots.
				 * Should be adjusted to take into account of the rules
				 * and number of robots in this role.
				 * Returns a pair
				 * - goalie position
				 * - other robots position
				 * Note:
				 * - if any of the position == ball position,
				 *   please use chase/shoot etc
				 */
				std::pair<Point, std::vector<Point> > calc_block_positions() const;

				AI::HL::W::World &world;

				std::vector<W::Player::Ptr> players;

				W::Player::Ptr goalie;
				// true if the goalie is currently defending the top
				bool goalie_top;

				// true if one of the players should chase after the ball.
				bool chase;
		};

	}

}

#endif

