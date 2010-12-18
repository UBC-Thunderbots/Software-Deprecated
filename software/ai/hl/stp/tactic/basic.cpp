#include "ai/hl/stp/tactic/basic.h"

#include "ai/hl/util.h"
#include "ai/hl/tactics.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

// move

namespace {
	class Move : public Tactic {
		public:
			Move(World &world, const Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;

			bool done() const {
				return (player->position() - dest).len() < AI::HL::Util::POS_CLOSE;
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - dest).len());
			}

			void execute() {
				// TODO: flags
				player->move(dest, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::move(World &world, const Point dest) {
	const Tactic::Ptr p(new Move(world, dest));
	return p;
}

// defend goal

namespace {
	class DefendGoal : public Tactic {
		public:
			DefendGoal(World &world) : Tactic(world) {
			}

		private:
			double score(Player::Ptr player) const {
				if (world.friendly_team().get(0) == player) {
					return 1;
				}
				return 0;
			}

			void execute() {
				AI::HL::Tactics::lone_goalie(world, player);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::defend_goal(World &world) {
	const Tactic::Ptr p(new DefendGoal(world));
	return p;
}

// repel

namespace {
	class Repel : public Tactic {
		public:
			Repel(World &world) : Tactic(world) {
			}

		private:
			bool done() const {
				// will never be done... unless ball is outside the field
				return false;
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - world.ball().position()).len());
			}

			void execute() {
				AI::HL::Tactics::repel(world, player, 0);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::repel(World &world) {
	const Tactic::Ptr p(new Repel(world));
	return p;
}

// block

namespace {
	class Block : public Tactic {
		public:
			Block(World &world, Robot::Ptr robot) : Tactic(world), robot(robot) {
			}

		private:
			Robot::Ptr robot;

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - robot->position()).len());
			}

			void execute() {
				// TODO: flags
				player->move(robot->position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::block(World &world, Robot::Ptr robot) {
	const Tactic::Ptr p(new Block(world, robot));
	return p;
}

// shoot

namespace {
	class Shoot : public Tactic {
		public:
			Shoot(World &world) : Tactic(world) {
			}

		private:
			bool done() const {
#warning find a way to check that the ball has left off in the right direction
				return true;
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - world.ball().position()).len());
			}

			void execute() {
				// TODO: flags
				AI::HL::Tactics::shoot(world, player, 0);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::shoot(World &world) {
	const Tactic::Ptr p(new Shoot(world));
	return p;
}

// chase

namespace {
	class Chase : public Tactic {
		public:
			Chase(World &world) : Tactic(world) {
			}

		private:
			bool done_;

			bool done() const {
				return done_;
			}

			void player_changed() {
				done_ = (player->has_ball());
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - world.ball().position()).len());
			}

			void execute() {
				if (player->has_ball()) {
					done_ = true;
				}
				// TODO: flags
				// player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
				// AI::HL::Tactics::chase(world, player, 0);
				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::chase(World &world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

// idle

namespace {
	class Idle : public Tactic {
		public:
			Idle(World &world) : Tactic(world) {
			}

		private:
			double score(Player::Ptr) const {
				return 1;
			}
			void execute() {
				// nothing lol
				Point dest = Point(0, world.field().width() / 2);
				player->move(dest, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::idle(World &world) {
	const Tactic::Ptr p(new Idle(world));
	return p;
}

