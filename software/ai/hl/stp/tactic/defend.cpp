#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Evaluation;
using namespace AI::HL::W;

namespace {
	/**
	 * Goalie in a team of N robots.
	 */
	template<int N>
		class Goalie : public Tactic {
			public:
				Goalie(World &world) : Tactic(world) {
				}

			private:
				Player::Ptr select(const std::set<Player::Ptr> &players) const;
				void execute();
		};

	/**
	 * D-th Defender in a team of N robots.
	 * D starts from 1.
	 */
	template<int N, int D>
		class Defender : public Tactic {
			public:
				Defender(World &world) : Tactic(world) {
				}
			private:
				Player::Ptr select(const std::set<Player::Ptr> &players) const;
				void execute();
		};

	template<int N>
		Player::Ptr Goalie<N>::select(const std::set<Player::Ptr> &players) const {
			Player::CPtr cgoalie = world.friendly_team().get(0);
			Player::Ptr goalie;
			for (auto it = players.begin(); it != players.end(); ++it) {
				Player::CPtr tmp = *it;
				if (cgoalie == tmp) {
					goalie = *it;
				}
			}
			assert(goalie.is());
			return goalie;
		}

	template<int N>
		void Goalie<N>::execute() {
			auto waypoints = evaluate_defense<N>(world);
#warning use goalie SSM
			player->move(waypoints[0], (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
		}

	template<int N, int D>
		Player::Ptr Defender<N, D>::select(const std::set<Player::Ptr> &players) const {
			auto waypoints = evaluate_defense<N>(world);
			Point dest = waypoints[D];
			return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
		}

	template<int N, int D>
		void Defender<N, D>::execute() {
			auto waypoints = evaluate_defense<N>(world);
			Point dest = waypoints[D];
			// TODO: medium priority for D = 1, low for D = 2
			player->move(dest, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, param.move_priority);
		}
}

Tactic::Ptr AI::HL::STP::Tactic::defend_solo_goalie(AI::HL::W::World &world) {
	return lone_goalie(world);
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_goalie(AI::HL::W::World &world) {
	const Tactic::Ptr p(new Goalie<2>(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_defender(AI::HL::W::World &world) {
	const Tactic::Ptr p(new Defender<2, 1>(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra(AI::HL::W::World &world) {
	// TODO: do something more useful...
	const Tactic::Ptr p(new Defender<3, 2>(world));
	return p;
}

/*
Tactic::Ptr AI::HL::STP::Tactic::defend_trio_goalie(AI::HL::W::World &world) {
	const Tactic::Ptr p(new Goalie<3>(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_trio_defender_1(AI::HL::W::World &world) {
	const Tactic::Ptr p(new Defender<3, 1>(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_trio_defender_2(AI::HL::W::World &world) {
	const Tactic::Ptr p(new Defender<3, 2>(world));
	return p;
}
*/

