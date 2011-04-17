#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - ball not under any possesion
	 * - at least 2 players
	 *
	 * Objective:
	 * - grab the ball
	 */
	class GrabBall : public Play {
		public:
			GrabBall(const World &world);

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<GrabBall> factory_instance("Grab Ball");

	const PlayFactory &GrabBall::factory() const {
		return factory_instance;
	}

	GrabBall::GrabBall(const World &world) : Play(world) {
	}

	bool GrabBall::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3);
	}

	bool GrabBall::applicable() const {
		//return false;
		return Predicates::none_ball(world);
	}

	bool GrabBall::done() const {
		return Predicates::our_ball(world);
	}

	bool GrabBall::fail() const {
		return Predicates::their_ball(world);
	}

	void GrabBall::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// GOALIE
		// defend the goal
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// chase the ball!
		roles[0].push_back(chase(world));

		// ROLE 2
		// defend
		roles[1].push_back(defend_duo_defender(world));

		// ROLE 3 (optional)
		// offensive support
		roles[2].push_back(offend(world));

		// ROLE 4 (optional)
		// extra defense
		roles[3].push_back(defend_duo_extra(world));
	}
}

