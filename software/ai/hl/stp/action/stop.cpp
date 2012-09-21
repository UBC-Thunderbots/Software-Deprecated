#include "ai/hl/stp/action/stop.h"
#include "ai/flags.h"
#include "util/param.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::stop(World, Player::Ptr player) {
	player->move(player->position(), player->orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::LOW);
}

