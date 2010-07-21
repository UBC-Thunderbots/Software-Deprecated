#ifndef AI_DEFENSIVE3_H
#define AI_DEFENSIVE3_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include <vector>
#include <set>

/**
 * Combined goalie and defender
 * The goalie is always automatically assigned,
 * and never changes unless removed from this role.
 *
 * NEW DEFENSIVE for STATEFUL AI
 * This supercedes all previous defensive role implementations
 *
 * Notes
 * - perhaps add_player/remove_player make more sense than set_players
 *   since we are moving to stateful AI
 * - must not have more defenders than 1 + number of enemy players
 */
class Defensive3 : public NonCopyable {
	public:

		Defensive3(const World::ptr world);

		void tick();

		/**
		 * Adds a player to this role.
		 * The first player added will become a goalie.
		 * To override, please use set_goalie.
		 */
		void add_player(Player::ptr player);

		/**
		 * WARNING! Used only after a player goes missing.
		 * Please use pop_player() if possible.
		 */
		void remove_player(Player::ptr player);

		bool has_player(Player::ptr player) const {
			return players.find(player) != players.end();
		}

		size_t size() const {
			return players.size();
		}

		/**
		 * DEPRECATED function to hack old strategies.
		 * Reset all players.
		 */
		void deprecated_set_players(std::vector<Player::ptr>& ps) __attribute__ ((deprecated));

		/**
		 * WARNING! NEVER call this unless playtype is penalty enemy/victory/pitstop,
		 * or the team has only one player.
		 */
		void clear_players();

		/**
		 * By default, role chooses a player to be the goalie.
		 * This function overrides the behaviour.
		 * Calling this function implies adding the player to this role,
		 * if not already so.
		 */
		void set_goalie(Player::ptr player);

		/**
		 * Chooses and removes one player from this role.
		 * Ideally the least important player.
		 *
		 * \return a suitable player, or null if this role has nobody.
		 */
		Player::ptr pop_player();

		/**
		 * Makes a player go after the ball.
		 * This function is useful in case multiple roles want the ball.
		 * The behaviour of this function is to be determined in the near future.
		 * Q: What if you don't want anybody from this role to go after the ball?
		 */
		void set_chaser(Player::ptr player);

	protected:

		/**
		 * Updates points which should be used to defend from enemy robots.
		 * Should be adjusted to take into account of the rules
		 * and number of robots in this Role.
		 */
		void calc_block_positions();

		const World::ptr world;

		std::set<Player::ptr> players;

		Point goalie_position;

		/// Ordered by importance.
		std::vector<Point> defender_positions;

		Player::ptr goalie;

		/// Should goalie defend the top part of the net?
		bool goalie_guard_top;
};

#endif

