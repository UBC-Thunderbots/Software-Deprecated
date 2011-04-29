#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world) : Tactic(world), kicked(false) {
			}

		private:

			bool kicked;
			bool done() const {
				return kicked;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}
			void execute() {
			
				const FriendlyTeam &friendly = world.friendly_team();
		
				std::set<Player::CPtr> players;
				for (std::size_t i = 0; i < friendly.size(); ++i) {
					players.insert(friendly.get(i));
				}
		
				std::pair <Point, Point> pp = Evaluation::calc_pass_positions(world, players);
			
				// orient towards target
				player->move(pp.first, (pp.second - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MoveType::DRIBBLE, AI::Flags::MovePrio::HIGH);
				player->kick(7.5);
				kicked = true;
			}
			std::string description() const {
				return "passer-shoot";
			}
			
	};

	class PasseeReceive : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReceive(const World &world) : Tactic(world, true) {
			}

		private:

			bool done() const {
				return player->has_ball();
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				const FriendlyTeam &friendly = world.friendly_team();
		
				std::set<Player::CPtr> p;
				for (std::size_t i = 0; i < friendly.size(); ++i) {
					p.insert(friendly.get(i));
				}
		
				std::pair <Point, Point> pp = Evaluation::calc_pass_positions(world, p);
				Point dest = pp.second;
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
			}
			void execute() {
				const FriendlyTeam &friendly = world.friendly_team();
		
				std::set<Player::CPtr> players;
				for (std::size_t i = 0; i < friendly.size(); ++i) {
					players.insert(friendly.get(i));
				}
		
				std::pair <Point, Point> pp = Evaluation::calc_pass_positions(world, players);
				Point dest = pp.second;
				player->move(dest, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
			}
			std::string description() const {
				return "passee-receive";
			}
			
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot(const World &world) {
	const Tactic::Ptr p(new PasserShoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_receive(const World &world) {
	const Tactic::Ptr p(new PasseeReceive(world));
	return p;
}

