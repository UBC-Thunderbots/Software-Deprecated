#include "ai/hl/hl.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/ui.h"
#include "ai/hl/stp/tactic/shooting_challenge.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/move.h"
#include "geom/util.h"
#include "util/param.h"
#include "ai/common/playtype.h"


namespace Flags = AI::Flags;

using namespace AI::HL;
using namespace AI::HL::STP;
using namespace AI::HL::W;


namespace {
	class ShootingChallenge : public HighLevel {
		public:
			ShootingChallenge(World world) : world(world) {
			}

		private:
			World world;

			HighLevelFactory &factory() const;

			Gtk::Widget *ui_controls() {
				return 0;
			}

			void tick() {

				tick_eval(world);

				FriendlyTeam friendly = world.friendly_team();
				if (friendly.size() == 0) {
					return;
				}
				if (friendly.size() > 0) {
					if (world.playtype() == AI::Common::PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY || world.playtype() == AI::Common::PlayType::PLAY) {
						friendly.get(0).flags(0);
						AI::HL::STP::Action::shoot_goal(world, friendly.get(0), true);
					}
					else if (world.playtype() == AI::Common::PlayType::STOP) {
						Action::move(world, friendly.get(0), Point((-2*Robot::MAX_RADIUS),0));
					}

					else if (world.playtype() == AI::Common::PlayType::HALT) {
						return;
					}
				}

			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				draw_shoot(world, ctx);
			}
	};
}

HIGH_LEVEL_REGISTER(ShootingChallenge)

