#include "ai/strategy/strategy.h"
#include <stdexcept>

void Strategy::halt() {
}

void Strategy::stop() {
	resign();
}

void Strategy::play() {
}

void Strategy::prepare_kickoff_friendly() {
}

void Strategy::execute_kickoff_friendly() {
}

void Strategy::prepare_kickoff_enemy() {
}

void Strategy::execute_kickoff_enemy() {
}

void Strategy::prepare_penalty_friendly() {
}

void Strategy::execute_penalty_friendly() {
}

void Strategy::prepare_penalty_enemy() {
}

void Strategy::execute_penalty_enemy() {
}

void Strategy::execute_direct_free_kick_friendly() {
}

void Strategy::execute_indirect_free_kick_friendly() {
}

void Strategy::execute_direct_free_kick_enemy() {
}

void Strategy::execute_indirect_free_kick_enemy() {
}

bool Strategy::has_resigned() const {
	return has_resigned_;
}

void Strategy::tick() {
	switch (world.playtype()) {
		case PlayType::HALT:
			halt();
			return;

		case PlayType::STOP:
			stop();
			return;

		case PlayType::PLAY:
			play();
			return;

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
			prepare_kickoff_friendly();
			return;

		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			execute_kickoff_friendly();
			return;

		case PlayType::PREPARE_KICKOFF_ENEMY:
			prepare_kickoff_enemy();
			return;

		case PlayType::EXECUTE_KICKOFF_ENEMY:
			execute_kickoff_enemy();
			return;

		case PlayType::PREPARE_PENALTY_FRIENDLY:
			prepare_penalty_friendly();
			return;

		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			execute_penalty_friendly();
			return;

		case PlayType::PREPARE_PENALTY_ENEMY:
			prepare_penalty_enemy();
			return;

		case PlayType::EXECUTE_PENALTY_ENEMY:
			execute_penalty_enemy();
			return;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			execute_direct_free_kick_friendly();
			return;

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			execute_indirect_free_kick_friendly();
			return;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			execute_direct_free_kick_enemy();
			return;

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			execute_indirect_free_kick_enemy();
			return;

		case PlayType::COUNT:
			break;
	}
	throw std::out_of_range("Play type number is out of range!");
}

Strategy::Strategy(World &world) : world(world), has_resigned_(false) {
}

Strategy::~Strategy() {
}

void Strategy::resign() {
	has_resigned_ = true;
}

StrategyFactory::StrategyFactory(const Glib::ustring &name, const PlayType::PlayType *handled_play_types, std::size_t handled_play_types_size) : Registerable<StrategyFactory>(name), handled_play_types(handled_play_types), handled_play_types_size(handled_play_types_size) {
}

StrategyFactory::~StrategyFactory() {
}

