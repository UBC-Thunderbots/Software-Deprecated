#include "ai/hl/world.h"

using namespace AI::HL::W;

void AI::HL::W::Player::move(Point dest, Angle ori, unsigned int flags, AI::Flags::MoveType type, AI::Flags::MovePrio prio) {
	move(dest, ori, Point());
	this->flags(flags);
	this->type(type);
	this->prio(prio);
}

AI::HL::W::World::World() = default;

