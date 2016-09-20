#include "ai/common/playtype.h"
#include <cstdlib>
#include <stdexcept>

AI::Common::PlayType AI::Common::PlayTypeInfo::of_int(unsigned int pt) {
	if (pt <= static_cast<unsigned int>(AI::Common::PlayType::NONE)) {
		return static_cast<AI::Common::PlayType>(pt);
	} else {
		throw std::invalid_argument("Invalid play type index");
	}
}

Glib::ustring AI::Common::PlayTypeInfo::to_string(PlayType pt) {
	switch (pt) {
		case PlayType::HALT:
			return u8"Halt";

		case PlayType::STOP:
			return u8"Stop";

		case PlayType::PLAY:
			return u8"Play";

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
			return u8"Prep Kickoff Friendly";

		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			return u8"Kickoff Friendly";

		case PlayType::PREPARE_KICKOFF_ENEMY:
			return u8"Prep Kickoff Enemy";

		case PlayType::EXECUTE_KICKOFF_ENEMY:
			return u8"Kickoff Enemy";

		case PlayType::PREPARE_PENALTY_FRIENDLY:
			return u8"Prep Penalty Friendly";

		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			return u8"Penalty Friendly";

		case PlayType::PREPARE_PENALTY_ENEMY:
			return u8"Prep Penalty Enemy";

		case PlayType::EXECUTE_PENALTY_ENEMY:
			return u8"Penalty Enemy";

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			return u8"Direct Free Friendly";

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			return u8"Indirect Free Friendly";

		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			return u8"Direct Free Enemy";

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			return u8"Indirect Free Enemy";

		case PlayType::BALL_PLACEMENT_ENEMY:
			return u8"Ball Placement Enemy";
			
		case PlayType::BALL_PLACEMENT_FRIENDLY:
			return u8"Ball Placement Friendly";

		case PlayType::NONE:
			return u8"None";
	}

	std::abort();
}

AI::Common::PlayType AI::Common::PlayTypeInfo::invert(PlayType pt) {
	switch (pt) {
		case PlayType::HALT:
		case PlayType::STOP:
		case PlayType::PLAY:
		case PlayType::NONE:
			return pt;

		case PlayType::PREPARE_KICKOFF_FRIENDLY:
			return PlayType::PREPARE_KICKOFF_ENEMY;

		case PlayType::EXECUTE_KICKOFF_FRIENDLY:
			return PlayType::EXECUTE_KICKOFF_ENEMY;

		case PlayType::PREPARE_KICKOFF_ENEMY:
			return PlayType::PREPARE_KICKOFF_FRIENDLY;

		case PlayType::EXECUTE_KICKOFF_ENEMY:
			return PlayType::EXECUTE_KICKOFF_FRIENDLY;

		case PlayType::PREPARE_PENALTY_FRIENDLY:
			return PlayType::PREPARE_PENALTY_ENEMY;

		case PlayType::EXECUTE_PENALTY_FRIENDLY:
			return PlayType::EXECUTE_PENALTY_ENEMY;

		case PlayType::PREPARE_PENALTY_ENEMY:
			return PlayType::PREPARE_PENALTY_FRIENDLY;

		case PlayType::EXECUTE_PENALTY_ENEMY:
			return PlayType::EXECUTE_PENALTY_FRIENDLY;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY:
			return PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY;

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY:
			return PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY;

		case PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY:
			return PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY;

		case PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY:
			return PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY;

		case PlayType::BALL_PLACEMENT_ENEMY:
			return PlayType::BALL_PLACEMENT_FRIENDLY;

		case PlayType::BALL_PLACEMENT_FRIENDLY:
			return PlayType::BALL_PLACEMENT_ENEMY;
	}

	std::abort();
}
