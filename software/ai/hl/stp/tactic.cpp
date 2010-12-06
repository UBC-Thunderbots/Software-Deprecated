#include "ai/hl/stp/tactic.h"

#include "ai/hl/tactics.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;

Tactic::Tactic(World& world) : world(world) {
}

Tactic::~Tactic() {
}

// move

namespace {
	class Move : public Tactic {
		public:
			Move(World& world, const Point dest) : Tactic(world), dest(dest) {
			}
		private:
			const Point dest;

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - dest).len());
			}

			void tick(Player::Ptr player) {
				// TODO: flags
				player->move(dest, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::move(World& world, const Point dest) {
	const Tactic::Ptr p(new Move(world, dest));
	return p;
}

// defend goal

namespace {
	class DefendGoal : public Tactic {
		public:
			DefendGoal(World& world) : Tactic(world) {
			}
		private:
			double score(Player::Ptr player) const {
				if (world.friendly_team().get(0) == player) {
					return 1;
				}
				return 0;
			}

			void tick(Player::Ptr player) {
				// TODO: flags
				AI::HL::Tactics::lone_goalie(world, player);
			}
	};
}

Tactic::Ptr AI::HL::STP::defend_goal(World& world) {
	const Tactic::Ptr p(new DefendGoal(world));
	return p;
}

// chase

namespace {
	class Chase : public Tactic {
		public:
			Chase(World& world) : Tactic(world) {
			}
		private:
			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - world.ball().position()).len());
			}

			void tick(Player::Ptr player) {
				// TODO: flags
				// player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
				AI::HL::Tactics::chase(world, player, 0);
			}
	};
}

Tactic::Ptr AI::HL::STP::chase(World& world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

// block

namespace {
	class Block : public Tactic {
		public:
			Block(World& world, Robot::Ptr robot) : Tactic(world), robot(robot) {
			}
		private:
			Robot::Ptr robot;

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - robot->position()).len());
			}

			void tick(Player::Ptr player) {
				// TODO: flags
				player->move(robot->position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::block(World& world, Robot::Ptr robot) {
	const Tactic::Ptr p(new Block(world, robot));
	return p;
}

// nothing to do

namespace {
	class Idle : public Tactic {
		public:
			Idle(World& world) : Tactic(world) {
			}
		private:
			double score(Player::Ptr player) const {
				return 1;
			}
			void tick(Player::Ptr player) {
				// nothing lol
			}
	};
}

Tactic::Ptr AI::HL::STP::idle(World& world) {
	const Tactic::Ptr p(new Idle(world));
	return p;
}

