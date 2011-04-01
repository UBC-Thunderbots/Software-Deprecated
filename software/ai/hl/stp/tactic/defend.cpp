#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Evaluation = AI::HL::STP::Evaluation;
namespace Action = AI::HL::STP::Action;

namespace {
	/**
	 * Goalie in a team of N robots.
	 */
	class Goalie : public Tactic {
		public:
			Goalie(const World &world) : Tactic(world) {
			}

		private:
			void execute();
			Player::Ptr select(const std::set<Player::Ptr> &) const {
				assert(0);
			}
			std::string description() const {
				return "goalie (helped by defender)";
			}
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
	};

	/**
	 * Primary defender.
	 */
	class Primary : public Tactic {
		public:
			Primary(const World &world) : Tactic(world) {
			}
		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "defender (helps goalie)";
			}
	};

	/**
	 * Secondary defender.
	 */
	class Secondary : public Tactic {
		public:
			Secondary(const World &world) : Tactic(world) {
			}
		private:
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "extra defender";
			}
	};

	void Goalie::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		//player->move(waypoints[0], (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
		Action::goalie_move(world, player,  waypoints[0]);
	}

	void Goalie::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		const Field& field = world.field();

		const Point goal1 = Point(-field.length() / 2, field.goal_width() / 2);
		const Point goal2 = Point(-field.length() / 2, -field.goal_width() / 2);

		ctx->set_line_width(0.01);
		ctx->set_source_rgba(0.5, 1.0, 0.5, 0.5);

		ctx->move_to(world.ball().position().x, world.ball().position().y);
		ctx->line_to(goal1.x, goal1.y);
		ctx->stroke();

		ctx->move_to(world.ball().position().x, world.ball().position().y);
		ctx->line_to(goal2.x, goal2.y);
		ctx->stroke();
	}

	Player::Ptr Primary::select(const std::set<Player::Ptr> &players) const {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[1];
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Primary::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[1];
		// TODO: medium priority for D = 1, low for D = 2
		player->move(dest, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}

	Player::Ptr Secondary::select(const std::set<Player::Ptr> &players) const {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[2];
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest));
	}

	void Secondary::execute() {
		auto waypoints = Evaluation::evaluate_defense(world);
		Point dest = waypoints[2];
		player->move(dest, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_goalie(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Goalie(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_defender(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Primary(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::defend_duo_extra(const AI::HL::W::World &world) {
	const Tactic::Ptr p(new Secondary(world));
	return p;
}

