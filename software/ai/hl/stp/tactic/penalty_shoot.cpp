#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class PenaltyShoot : public Tactic {
		public:
			PenaltyShoot(const World &world) : Tactic(world, true), shoot_up(true), has_shot(false) {
			}

		private:
			bool shoot_up;
			bool has_shot;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "penalty-shoot";
			}
	};

	bool PenaltyShoot::done() const {
		return has_shot;
	}

	Player::Ptr PenaltyShoot::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.field().enemy_goal()));
	}

	void PenaltyShoot::execute() {
		/*
		   // shoot center of goal if there is no enemy
		   Point target = world.field().enemy_goal();

		   // otherwise, find a side to shoot
		   if (world.enemy_team().size() > 0) {
		    // since all other robots not participating in penalty shoot must be far away from the goal post
		    // hence the enemy goalie is the robot closest to enemy goal post
		    std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

		    Robot::Ptr enemy_goalie = *std::min_element(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().enemy_goal()));
		    const Field &f = world.field();
		    // a hysteresis
		    double goal_post_diff = (f.enemy_goal_boundary().first - f.enemy_goal_boundary().second).len();
		    const double target_y = goal_post_diff * 3 / 4;

		    if (shoot_up && enemy_goalie->position().y + Robot::MAX_RADIUS > target_y) {
		        shoot_up = false;
		    } else if (!shoot_up && enemy_goalie->position().y - Robot::MAX_RADIUS < -target_y) {
		        shoot_up = true;
		    }

		    if (shoot_up) {
		        target = Point(f.length() / 2, target_y);
		    } else {
		        target = Point(f.length() / 2, -target_y);
		    }
		   }

		   if (AI::HL::STP::Action::shoot_target(world, player, target)) {
		    has_shot = true;
		   }
		 */
		has_shot = AI::HL::STP::Action::shoot_goal(world, player, false);

		// unset any flags
		player->flags(0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_shoot(const World &world) {
	const Tactic::Ptr p(new PenaltyShoot(world));
	return p;
}

