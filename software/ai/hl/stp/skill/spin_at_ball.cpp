#include "ai/hl/stp/skill/spin_at_ball.h"
#include "ai/flags.h"

using namespace AI::HL::W;
using namespace AI::HL::STP::Skill;
using AI::HL::STP::Skill::Skill;

namespace {
	class SpinAtBall : public Skill {
		private:
			void execute(World& world, Player::Ptr player, Param& param, Context& context) const {
				// TODO
			}
	};

	SpinAtBall spin_at_ball_instance;
}

const Skill* spin_at_ball() {
	return &spin_at_ball_instance;
}

