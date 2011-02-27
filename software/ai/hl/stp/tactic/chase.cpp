#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Chase : public Tactic {
		public:
			Chase(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool Chase::done() const {
		return player->has_ball();
	}

	Player::Ptr Chase::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players);
	}

	void Chase::execute() {
		// if it has the ball, stay there
		if (player->has_ball()) {
			player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
			return;
		}

		// TODO: flags
		AI::HL::STP::Action::chase(world, player, 0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::chase(const World &world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

