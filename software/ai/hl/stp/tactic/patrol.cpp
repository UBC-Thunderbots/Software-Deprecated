#include "ai/hl/stp/tactic/patrol.h"
#include "ai/hl/util.h"
#include <iostream>

using AI::HL::STP::Tactic::Patrol;
using namespace AI::HL::W;

Patrol::Patrol(AI::HL::W::World &world, Coordinate w1, Coordinate w2) : Tactic(world), p1(w1), p2(w2) {
}

double Patrol::score(AI::HL::W::Player::Ptr player) const {
	return -std::max((player->position() - p1()).len(), (player->position() - p2()).len());
}

void Patrol::execute() {
	if (!player.is()) {
		return;
	}

	if ((player->position() - p1()).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = false;
	} else if ((player->position() - p2()).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = true;
	}
	if (goto_target1) {
		player->move(p1(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	} else {
		player->move(p2(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}

