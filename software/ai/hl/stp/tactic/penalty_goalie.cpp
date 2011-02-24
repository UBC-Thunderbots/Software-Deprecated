#include "ai/hl/stp/tactic/penalty_goalie.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;

namespace {
	class PenaltyGoalie : public Tactic {
		public:
			PenaltyGoalie(const World &world);
		private:
			bool goto_target1;
			bool done() const;
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
	};

	PenaltyGoalie::PenaltyGoalie(const World& world) : Tactic(world, true) {
	}

	bool PenaltyGoalie::done() const {
		return world.playtype() != PlayType::PREPARE_PENALTY_ENEMY
			&& world.playtype() != PlayType::EXECUTE_PENALTY_ENEMY;
	}

	void PenaltyGoalie::execute() {
		const Point p1(-0.5 * world.field().length(), -0.8 * Robot::MAX_RADIUS);
		const Point p2(-0.5 * world.field().length(), 0.8 * Robot::MAX_RADIUS);

		if ((player->position() - p1).len() < AI::HL::Util::POS_CLOSE) {
			goto_target1 = false;
		} else if ((player->position() - p2).len() < AI::HL::Util::POS_CLOSE) {
			goto_target1 = true;
		}

		// just orient towards the "front"

		if (goto_target1) {
			player->move(p1, 0, 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
		} else {
			player->move(p2, 0, 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_goalie(const World &world) {
	const Tactic::Ptr p(new PenaltyGoalie(world));
	return p;
}

