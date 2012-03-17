#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/dribble.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class Intercept : public Tactic {
		public:
			Intercept(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			Glib::ustring description() const {
				return "intercept";
			}
			// void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
	};

	bool Intercept::done() const {
		return player->has_ball();
	}

	Player::Ptr Intercept::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players, player);
	}

	void Intercept::execute() {
		// if it has the ball, stay there
		if (player->has_ball()) {
			Action::dribble(world, player);
			return;
		}

		// orient towards the enemy goal?
		Action::intercept(player, world.field().enemy_goal());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::intercept(const World &world) {
	Tactic::Ptr p(new Intercept(world));
	return p;
}

