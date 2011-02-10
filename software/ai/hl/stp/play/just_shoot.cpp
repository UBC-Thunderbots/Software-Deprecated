#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Evaluation::ConeDefense;

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
			ConeDefense cone_defense;

			void initialize();
			bool applicable() const;
			bool done();
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<JustShoot> factory_instance("Just Shoot");

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

	JustShoot::JustShoot(World &world) : Play(world), cone_defense(*this, world) {
	}

	JustShoot::~JustShoot() {
	}

	bool JustShoot::done() {
		return applicable();
	}

	void JustShoot::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(lone_goalie(world));

		// ROLE 1
		// shoot
		roles[0].push_back(shoot(world));

		// ROLE 2
		// block nearest enemy
		roles[1].push_back(block(world, Enemy::closest_friendly_goal(world, 0)));

		// ROLE 3
		// block 2nd nearest enemy
		roles[2].push_back(block(world, Enemy::closest_friendly_goal(world, 1)));

		// ROLE 4
		// block 3rd nearest enemy
		roles[3].push_back(block(world, Enemy::closest_friendly_goal(world, 2)));
	}
}

