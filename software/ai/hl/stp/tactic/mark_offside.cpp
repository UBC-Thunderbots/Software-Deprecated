#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/tactic/mark_offside.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class MarkOffside : public Tactic {
		public:
			MarkOffside(const World &world) : Tactic(world), target(target) {
			}

		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			Coordinate dest;
			Robot::Ptr player_to_mark(std::vector<AI::HL::W::Robot::Ptr> enemies) const;
			const Point target;
			Player::CPtr nearest_friendly (Point target) const;
			Glib::ustring description() const {
				return "MarkOffside";
			}
	};

	Player::Ptr MarkOffside::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest.position()));
	}

	Robot::Ptr MarkOffside::player_to_mark(std::vector<AI::HL::W::Robot::Ptr> enemies) const {
		// filter out enemies that:
		// 1. are far away from our goal
		// 2. can't shoot to goal
		for (std::size_t i = enemies.size() - 1; i > 0; i--) {
			if ((enemies[i]->position().x > 1.0) || (!Evaluation::enemy_can_shoot_goal(world, enemies[i]))) {
				enemies.erase(enemies.begin() + i);
			}
		}
		AI::HL::W::Robot::Ptr open_robot = Evaluation::calc_enemy_baller(world);
		// find the enemy robot with the biggest open space
		int most_open_index = -1;
		double dist = 0;
		for (std::size_t i = 0; i < enemies.size(); i++) {
			double d = (enemies[i]->position() - nearest_friendly(enemies[i]->position())->position()).len();
			if (most_open_index < 0 || d > dist) {
				most_open_index = static_cast<int>(i);
				dist = d;
			}
		}
		open_robot = enemies[most_open_index];
		return open_robot;
	}

	//return nearest friendly from the pool of non-marker players
	Player::CPtr MarkOffside::nearest_friendly (Point target) const{
		std::vector<Player::CPtr> team_pool;
		for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
			// filter out the player
			if (!world.friendly_team().get(i)->position().close(player->position(), 0.01)) {
				team_pool.push_back(world.friendly_team().get(i));
			}
		}
		int closest_i = -1;
		double dist = 0;
		for (std::size_t i = 0; i < team_pool.size(); i++) {
			double d = (target - team_pool[i]->position()).len();
			if (closest_i < 0 || d < dist) {
				closest_i = static_cast<int>(i);
				dist = d;
			}
		}
		return team_pool[closest_i];
	}

	void MarkOffside::execute() {
		std::vector<AI::HL::W::Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		Action::block_ball(world, player, player_to_mark(enemies));
	}
}


Tactic::Ptr AI::HL::STP::Tactic::mark_offside(const World &world) {
	Tactic::Ptr p(new MarkOffside(world));
	return p;
}
