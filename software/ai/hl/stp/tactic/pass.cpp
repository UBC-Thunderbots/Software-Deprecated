#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class PasserReady : public Tactic {
		public:
			PasserReady(const World &world, Coordinate p, Coordinate t) : Tactic(world), dest(p), target(t) {
			}
		private:
			Coordinate dest, target;
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}
			void execute() {
				// orient towards target
				player->move(dest(), (target() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);				
			}
			std::string description() const {
				return "passer-ready";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
				ctx->set_source_rgb(1.0, 1.0, 1.0);
				ctx->move_to(player->position().x, player->position().y);
				ctx->line_to(target().x, target().y);
				ctx->set_line_width(0.01);
				ctx->stroke();
			}
	};

	class PasseeReady : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReady(const World &world, Coordinate p) : Tactic(world, true), dest(p) {
			}

		private:
			Coordinate dest;
			bool done() const {
				return (player->position() - dest()).len() < AI::HL::Util::POS_CLOSE;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
			}
			void execute() {
				player->move(dest(), (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
			std::string description() const {
				return "passee-ready";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
				ctx->set_source_rgb(0.0, 0.0, 0.0);
				ctx->move_to(player->position().x, player->position().y);
				ctx->line_to(dest().x, dest().y);
				ctx->set_line_width(0.01);
				ctx->stroke();
			}
	};

	class PasserShoot : public Tactic {
		public:
			PasserShoot(const World &world, Coordinate t) : Tactic(world), target(t), kicked(false) {
			}

		private:
			Coordinate target;
			bool kicked;
			bool done() const {
				return kicked;
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players);
			}
			void execute() {
				// orient towards target
				player->move(target(), (target() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
				player->kick(7.5);
				kicked = true;
			}
			std::string description() const {
				return "passer-shoot";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
				ctx->set_source_rgb(1.0, 1.0, 1.0);
				ctx->move_to(player->position().x, player->position().y);
				ctx->line_to(target().x, target().y);
				ctx->set_line_width(0.01);
				ctx->stroke();
			}
	};

	class PasseeReceive : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReceive(const World &world, Coordinate p) : Tactic(world, true), dest(p) {
			}

		private:
			Coordinate dest;
			bool done() const {
				return player->has_ball();
			}
			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
			}
			void execute() {
				player->move(dest(), (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
			std::string description() const {
				return "passee-receive";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
				ctx->set_source_rgb(0.0, 0.0, 0.0);
				ctx->move_to(player->position().x, player->position().y);
				ctx->line_to(dest().x, dest().y);
				ctx->set_line_width(0.01);
				ctx->stroke();
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_ready(const World &world, Coordinate pos, Coordinate target) {
	const Tactic::Ptr p(new PasserReady(world, pos, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_ready(const World &world, Coordinate pos) {
	const Tactic::Ptr p(new PasseeReady(world, pos));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passer_shoot(const World &world, Coordinate target) {
	const Tactic::Ptr p(new PasserShoot(world, target));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::passee_receive(const World &world, Coordinate pos) {
	const Tactic::Ptr p(new PasseeReceive(world, pos));
	return p;
}

