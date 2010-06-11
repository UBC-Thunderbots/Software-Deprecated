#ifndef AI_WORLD_TEAM_H
#define AI_WORLD_TEAM_H

#include "ai/world/player.h"
#include "ai/world/robot.h"
#include "util/byref.h"
#include <cassert>
#include <cstddef>
#include <vector>
#include <sigc++/sigc++.h>

class world;

/**
 * A team is a collection of robots.
 */
class team : public byref {
	public:
		/**
		 * A pointer to a team.
		 */
		typedef Glib::RefPtr<team> ptr;

		/**
		 * Fired when a robot is added to the team.
		 */
		mutable sigc::signal<void, unsigned int, robot::ptr> signal_robot_added;

		/**
		 * Fired when a robot is removed from the team.
		 */
		mutable sigc::signal<void, unsigned int, robot::ptr> signal_robot_removed;

		/**
		 * \return The number of robots on the team.
		 */
		virtual std::size_t size() const = 0;

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		virtual robot::ptr get_robot(unsigned int index) const = 0;

		/**
		 * \return A vector of robots.
		 */
		virtual std::vector<robot::ptr> get_robots() const = 0;

	protected:
		/**
		 * Constructs a new team.
		 */
		team();

		/**
		 * Removes a robot from the team.
		 * \param index the index of the robot to remove
		 */
		virtual void remove(unsigned int index) = 0;

	private:
		friend class world;
};

/**
 * An enemy_team is a collection of robots that cannot be driven.
 */
class enemy_team : public team {
	public:
		/**
		 * A pointer to a team.
		 */
		typedef Glib::RefPtr<enemy_team> ptr;

		/**
		 * \return The number of robots on the team
		 */
		std::size_t size() const {
			return members.size();
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		robot::ptr get_robot(unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \return A vector of robots.
		 */
		std::vector<robot::ptr> get_robots() const {
			return members;
		}

	private:
		std::vector<robot::ptr> members;

		/**
		 * Creates a new team.
		 */
		static ptr create();

		/**
		 * Adds a robot to the team.
		 * \param bot the robot to add
		 */
		void add(robot::ptr bot);

		/**
		 * Removes a robot from the team.
		 * \param index the index of the robot to remove
		 */
		void remove(unsigned int index);

		friend class world;
};

/**
 * A friendly_team is a collection of players that can be driven.
 */
class friendly_team : public team {
	public:
		/**
		 * A pointer to a team.
		 */
		typedef Glib::RefPtr<friendly_team> ptr;

		/**
		 * Fired when a player is added to the team.
		 */
		mutable sigc::signal<void, unsigned int, player::ptr> signal_player_added;

		/**
		 * Fired when a player is removed from the team.
		 */
		mutable sigc::signal<void, unsigned int, player::ptr> signal_player_removed;

		/**
		 * \return The number of robots on the team
		 */
		std::size_t size() const {
			return members.size();
		}

		/**
		 * \param index the index of the robot to fetch
		 * \return The robot
		 */
		robot::ptr get_robot(unsigned int index) const {
			return get_player(index);
		}

		/**
		 * \param index the index of the player to fetch
		 * \return The player
		 */
		player::ptr get_player(unsigned int index) const {
			assert(index < size());
			return members[index];
		}

		/**
		 * \return A vector of players.
		 */
		const std::vector<player::ptr>& get_players() const {
			return members;
		}

		/**
		 * \return A vector of robots.
		 */
		std::vector<robot::ptr> get_robots() const {
			return std::vector<robot::ptr>(members.begin(), members.end());
		}

	private:
		std::vector<player::ptr> members;

		/**
		 * Creates a new team.
		 */
		static ptr create();

		/**
		 * Adds a player to the team.
		 * \param bot the player to add
		 */
		void add(player::ptr bot);

		/**
		 * Removes a player from the team.
		 * \param index the index of the robot to remove
		 */
		void remove(unsigned int index);

		friend class world;
};

#endif

