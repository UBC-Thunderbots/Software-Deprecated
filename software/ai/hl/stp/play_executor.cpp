#include "ai/hl/stp/play_executor.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include "ai/hl/stp/ui.h"
#include <cassert>
#include <registerable.h>

using AI::HL::STP::PlayExecutor;
using namespace AI::HL::STP;

namespace AI {
	namespace HL {
		namespace STP {
		 	namespace HACK {
				Player::Ptr active_player;
				Player::Ptr last_kicked;
			}
		}
	}
}

namespace {
	// The maximum amount of time a play can be running.
	const double PLAY_TIMEOUT = 30.0;

	BoolParam goalie_lowest("Goalie is lowest index", "STP/STP", true);
	IntParam goalie_pattern_index("Goalie pattern index", "STP/STP", 0, 0, 4);

	void on_robot_removing(std::size_t i, World &w) {
		Player::Ptr plr = w.friendly_team().get(i);
		if (plr == HACK::active_player) {
			HACK::active_player.reset(); 
		}
		if (plr == HACK::last_kicked) {
			HACK::last_kicked.reset();
		}
	}

	void connect_player_remove_handler(World &w) {
		static bool connected = false;
		if (!connected) {
			w.friendly_team().signal_robot_removing().connect(sigc::bind(&on_robot_removing, sigc::ref(w)));
			connected = true;
		}
	}

}

PlayExecutor::PlayExecutor(World &w) : world(w) {
	connect_player_remove_handler(w);
	// initialize all plays
	const Play::PlayFactory::Map &m = Play::PlayFactory::all();
	assert(m.size() != 0);
	for (Play::PlayFactory::Map::const_iterator i = m.begin(), iend = m.end(); i != iend; ++i) {
		plays.push_back(i->second->create(world));
	}
}

void PlayExecutor::calc_play() {
	// find a valid play
	std::random_shuffle(plays.begin(), plays.end());
	for (std::size_t i = 0; i < plays.size(); ++i) {
		if (plays[i]->invariant() && plays[i]->applicable()) {
			curr_play = plays[i];
			LOG_INFO(curr_play->factory().name());
			assert(!curr_play->done());
			curr_role_step = 0;
			for (std::size_t j = 0; j < 5; ++j) {
				curr_roles[j].clear();
				// default to idle tactic
				curr_tactic[j] = Tactic::idle(world);
			}
			// assign the players
			{
				std::vector<Tactic::Tactic::Ptr> goalie_role;
				std::vector<Tactic::Tactic::Ptr> normal_roles[4];
				curr_play->assign(goalie_role, normal_roles);
				swap(goalie_role, curr_roles[0]);
				for (std::size_t j = 1; j < 5; ++j) {
					swap(normal_roles[j - 1], curr_roles[j]);
				}
			}
			return;
		}
	}
}

void PlayExecutor::role_assignment() {
	// this must be reset every tick
	curr_active.reset();

	for (std::size_t i = 0; i < 5; ++i) {
		if (curr_role_step < curr_roles[i].size()) {
			curr_tactic[i] = curr_roles[i][curr_role_step];
		} else {
			// if there are no more tactics, use the previous one
			// BUT active tactic cannot be reused!
			assert(!curr_tactic[i]->active());
		}

		if (curr_tactic[i]->active()) {
			// we cannot have more than 1 active tactic.
			assert(!curr_active.is());
			curr_active = curr_tactic[i];
		}
	}

	// we cannot have less than 1 active tactic.
	assert(curr_active.is());

	std::fill(curr_assignment, curr_assignment + 5, Player::Ptr());

	Player::Ptr goalie;
	if (goalie_lowest) {
		goalie = world.friendly_team().get(0);
	} else {
		for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
			Player::Ptr p = world.friendly_team().get(i);
			if (p->pattern() == goalie_pattern_index) {
				goalie = p;
			}
		}
	}

	if (!goalie.is()) {
		LOG_ERROR("No goalie");
		curr_play.reset();
		return;
	}

	assert(curr_tactic[0].is());
	curr_tactic[0]->set_player(goalie);
	curr_assignment[0] = goalie;

	// pool of available people
	std::set<Player::Ptr> players;
	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		Player::Ptr p = world.friendly_team().get(i);
		if (p == goalie) continue;
		players.insert(p);
	}

	bool active_assigned = (curr_tactic[0]->active());
	for (std::size_t i = 1; i < 5 && players.size() > 0; ++i) {
		curr_assignment[i] = curr_tactic[i]->select(players);
		// assignment cannot be empty
		assert(curr_assignment[i].is());
		assert(players.find(curr_assignment[i]) != players.end());
		players.erase(curr_assignment[i]);
		curr_tactic[i]->set_player(curr_assignment[i]);
		if (curr_tactic[i]->active()) {
			active_assigned = true;
			HACK::active_player = curr_assignment[i];
		}
	}

	// can't assign active tactic to anyone
	if (!active_assigned) {
		LOG_ERROR("Active tactic not assigned");
		curr_play.reset();
		return;
	}
}

void PlayExecutor::execute_tactics() {
	std::size_t max_role_step = 0;
	for (std::size_t i = 0; i < 5; ++i) {
		max_role_step = std::max(max_role_step, curr_roles[i].size());
	}

	while (true) {
		role_assignment();

		// if role assignment failed
		if (!curr_play.is()) {
			return;
		}

		// it is possible to skip steps
		if (curr_active->done()) {
			++curr_role_step;

			// when the play runs out of tactics, they are done!
			if (curr_role_step >= max_role_step) {
				LOG_INFO("Play done");
				curr_play.reset();
				return;
			}

			continue;
		}

		break;
	}

	// execute!
	for (std::size_t i = 0; i < 5; ++i) {
		if (!curr_assignment[i].is()) {
			continue;
		}
		curr_tactic[i]->execute();
	}
}

void PlayExecutor::tick() {
	// override halt completely
	if (world.friendly_team().size() == 0 || world.playtype() == AI::Common::PlayType::HALT) {
		curr_play.reset();
		return;
	}

	// check if curr play wants to continue
	if (curr_play.is() && (!curr_play->invariant() || curr_play->done() || curr_play->fail())) {
		curr_play.reset();
	}

	// check if curr is valid
	if (!curr_play.is()) {
		calc_play();
		if (!curr_play.is()) {
			LOG_WARN("calc play failed");
			return;
		}
	}

	execute_tactics();
}

void PlayExecutor::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
	draw_shoot(world, ctx);
	draw_offense(world, ctx);
	draw_defense(world, ctx);
	// draw_velocity(ctx); // uncommand to display velocity
	if (world.playtype() == AI::Common::PlayType::STOP) {
		ctx->set_source_rgb(1.0, 0.5, 0.5);
		ctx->arc(world.ball().position().x, world.ball().position().y, 0.5, 0.0, 2 * M_PI);
		ctx->stroke();
	}
	if (!curr_play.is()) {
		return;
	}
	curr_play->draw_overlay(ctx);
	for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
		const auto &role = curr_roles[i];
		for (std::size_t t = 0; t < role.size(); ++t) {
			role[t]->draw_overlay(ctx);
		}
	}
}

