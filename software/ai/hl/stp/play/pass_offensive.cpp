#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace {

	const std::vector<const Predicate*> APPLICABLE = {
		playtype(PlayType::PLAY),
		our_team_size_at_least(3),
		our_ball(),
	};

	const std::vector<const Predicate*> DONE = {
		goal(),
	};

	const std::vector<const Predicate*> FAIL = {
		negate(our_ball()),
	};

	/**
	 * Condition:
	 * - ball under team possesion
	 * - have at least 3 players (one goalie, one passer, one passee)
	 *
	 * Objective:
	 * - shoot the ball to enemy goal while passing the ball between the passer and passee
	 */
	class PassOffensive : public Play {
		public:
			PassOffensive(AI::HL::W::World &world);
			~PassOffensive();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<PassOffensive> factory_instance("Pass Offensive");

	const PlayFactory& PassOffensive::factory() const {
		return factory_instance;
	}

	PassOffensive::PassOffensive(World &world) : Play(world) {
	}

	PassOffensive::~PassOffensive() {
	}

	bool PassOffensive::applicable() const {
		return evaluate(world, APPLICABLE);
	}

	bool PassOffensive::done() const {
		return evaluate(world, DONE);
	}

	bool PassOffensive::fail() const {
		return evaluate(world, FAIL);
	}

	void PassOffensive::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>* roles) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		FriendlyTeam &friendly = world.friendly_team();

		// GOALIE
		goalie_role.push_back(lone_goalie(world));

		// TODO: better passer and passee positioning and targeting
		
		// ROLE 1
		// passer 
		roles[0].push_back(passer_ready(world, friendly.get(1)->position(), friendly.get(2)->position()));

		// ROLE 2
		// passee
		roles[1].push_back(passee_ready(world, world.ball().position()));
		roles[1].push_back(shoot(world));

		// ROLE 3
		// block nearest enemy
		roles[2].push_back(block(world, Enemy::closest_friendly_goal(world, 0)));

		// ROLE 4
		// block 2nd nearest enemy
		roles[3].push_back(block(world, Enemy::closest_friendly_goal(world, 1)));
	}
}

