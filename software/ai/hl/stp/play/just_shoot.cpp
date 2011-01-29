#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/basic.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {

	/**
	 * Condition:
	 * - ball under team possesion
	 *
	 * Objective:
	 * - shoot the ball to enemy goal
	 */
	class JustShoot : public Play {
		public:
			JustShoot(AI::HL::W::World &world);
			~JustShoot();

		private:
			void initialize();
			bool applicable() const;
			bool done();
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	class JustShootFactory : public PlayFactory {
		public:
			JustShootFactory() : PlayFactory("Just Shoot") {
			}
			Play::Ptr create(World &world) const {
				const Play::Ptr p(new JustShoot(world));
				return p;
			}
	} factory_instance;

	const PlayFactory& JustShoot::factory() const {
		return factory_instance;
	}

	void JustShoot::initialize() {
	}

	bool JustShoot::applicable() const {
		// check if we do not have ball
		FriendlyTeam &friendly = world.friendly_team();
		if (friendly.size() < 2) {
			return false;
		}
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (friendly.get(i)->has_ball()) {
				return true;
			}
		}
		return false;
	}

	JustShoot::JustShoot(World &world) : Play(world) {
	}

	JustShoot::~JustShoot() {
	}

	bool JustShoot::done() {
		return applicable();
	}

	void JustShoot::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		std::vector<Player::Ptr> players = AI::HL::Util::get_players(world.friendly_team());

		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

		// GOALIE
		if (players[0]->has_ball()) {
			goalie_role.push_back(repel(world));
		} else {
			goalie_role.push_back(defend_goal(world));
		}

		// ROLE 1
		// shoot
		if (players[0]->has_ball()) {
			if (enemies.size() > 0) {
				roles[0].push_back(block(world, enemies[0]));
			} else {
				roles[0].push_back(idle(world));
			}
		} else {
			roles[0].push_back(shoot(world));
		}

		// ROLE 2
		// block nearest enemy
		if (enemies.size() > 0) {
			roles[1].push_back(block(world, enemies[0]));
		} else {
			roles[1].push_back(idle(world));
		}

		// ROLE 3
		// block 2nd nearest enemy
		if (enemies.size() > 1) {
			roles[2].push_back(block(world, enemies[1]));
		} else {
			roles[2].push_back(idle(world));
		}

		// ROLE 4
		// block 3rd nearest enemy
		if (enemies.size() > 2) {
			roles[3].push_back(block(world, enemies[2]));
		} else {
			roles[3].push_back(idle(world));
		}
	}
}

