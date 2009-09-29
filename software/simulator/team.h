#ifndef SIMULATOR_TEAM_H
#define SIMULATOR_TEAM_H

#include "ai/strategy.h"
#include "robot_controller/robot_controller.h"
#include "simulator/engine.h"
#include "world/player.h"
#include "world/player_impl.h"
#include "world/playtype.h"
#include "world/team.h"
#include <vector>
#include <cassert>
#include <glibmm/refptr.h>
#include <libxml++/libxml++.h>

//
// A view of a physical team from either the west or the east perspective.
//
class simulator_team_view : public virtual controlled_team {
	public:
		//
		// Constructs a new simulator_team_view.
		//
		simulator_team_view(const std::vector<player::ptr> &players, const unsigned int &score, const team::ptr &other, const bool &yellow) : players(players), the_score(score), the_other(other), the_yellow(yellow) {
		}

		//
		// Gets the size of the team.
		//
		virtual std::size_t size() const {
			return players.size();
		}

		//
		// Gets one player on the team.
		//
		virtual player::ptr get_player(std::size_t idx) {
			assert(idx < players.size());
			return players[idx];
		}

		//
		// Gets the team's score.
		//
		virtual unsigned int score() const {
			return the_score;
		}

		//
		// Gets the other team.
		//
		virtual team::ptr other() {
			return the_other;
		}

		//
		// Gets the colour of the team.
		//
		virtual bool yellow() const {
			return the_yellow;
		}

	private:
		//
		// The vector, stored in the corresponding simulator_team_data object,
		// that contains the player objects exposed by this view.
		//
		const std::vector<player::ptr> &players;

		//
		// The score variable, stored in the corresponding simulator_team_data
		// object.
		//
		const unsigned int &the_score;

		//
		// The pointer to the other team, stored in the corresponding
		// simulator_team_data object.
		//
		const team::ptr &the_other;

		//
		// The colour of the team, stored in the corresponding
		// simulator_team_data object.
		//
		const bool &the_yellow;
};

//
// All the data about a physical team implemented by the simulator.
//
class simulator_team_data : public virtual noncopyable {
	public:
		//
		// Constructs a new simulator_team_data.
		//
		simulator_team_data(xmlpp::Element *xml, bool yellow);

		//
		// Sets the views of the other team.
		//
		void set_other(const team::ptr &w, const team::ptr &e) {
			assert(w);
			assert(e);
			west_other = w;
			east_other = e;
		}

		//
		// Sets the colour of this team.
		//
		void set_yellow(bool y) {
			yellow = y;
		}

		//
		// Gets the colour of this team.
		//
		bool is_yellow() {
			return yellow;
		}

		//
		// Sets the current play type.
		//
		void set_playtype(playtype::playtype pt) {
			current_playtype = pt;
			if (team_strategy)
				team_strategy->set_playtype(current_playtype);
		}

		//
		// Gets the strategy governing this team.
		//
		strategy::ptr get_strategy() {
			return team_strategy;
		}

		//
		// Sets the strategy governing this team by name.
		//
		void set_strategy(const Glib::ustring &name, ball::ptr ball, field::ptr field);

		//
		// Configures the team to use a new engine.
		//
		void set_engine(simulator_engine::ptr e);

		//
		// Configures the team to use a class of controller.
		//
		void set_controller_type(robot_controller_factory *cf);

		//
		// Adds a new player with the given ID number.
		//
		void add_player(unsigned int id);

		//
		// Removes the player with the given ID number.
		//
		void remove_player(unsigned int id);

		//
		// Returns the list of ID numbers in this team.
		//
		const std::vector<unsigned int> &get_ids() const {
			return ids;
		}

	private:
		//
		// The current engine.
		//
		simulator_engine::ptr engine;

		//
		// The ID numbers of the players on this team.
		//
		std::vector<unsigned int> ids;

		//
		// The engine-provided implementations of the players.
		//
		std::vector<player_impl::ptr> impls;

		//
		// The west and east views of the players on this team.
		//
		std::vector<player::ptr> west_players, east_players;

		//
		// The score.
		//
		unsigned int score;

		//
		// The pointers to the west and east views of the other team.
		//
		team::ptr west_other, east_other;

		//
		// The colour of this team.
		//
		bool yellow;

		//
		// The current play type.
		//
		playtype::playtype current_playtype;

		//
		// The current strategy governing this team.
		//
		strategy::ptr team_strategy;

		//
		// The robot controller class to use for new players.
		//
		robot_controller_factory *controller_factory;

	public:
		//
		// The objects that provide west and east views of this team.
		//
		const controlled_team::ptr west_view, east_view;

	private:
		//
		// The configuration element for this team.
		//
		xmlpp::Element *xml;
};

#endif

