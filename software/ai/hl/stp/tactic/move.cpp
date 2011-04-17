#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class Move : public Tactic {
		public:
			Move(const World &world, const Coordinate dest) : Tactic(world), dest(dest) {
			}

		private:
			const Coordinate dest;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "move";
			}
	};

	Player::Ptr Move::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
	}

	void Move::execute() {
		player->move(dest(), (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::MEDIUM);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move(const World &world, const Coordinate dest) {
	const Tactic::Ptr p(new Move(world, dest));
	return p;
}

