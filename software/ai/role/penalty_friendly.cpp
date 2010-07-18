#include "ai/role/penalty_friendly.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/move.h"
#include "ai/flags.h"
#include "ai/util.h"

#include <iostream>

PenaltyFriendly::PenaltyFriendly(World::ptr world) : the_world(world) {
	const Field& the_field(world->field());

	// Let the first robot to be always the shooter
	ready_positions[0] = Point(0.5 * the_field.length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0);

	ready_positions[1] = Point(0, 0);

	// Let two robots be on the offensive, in case there is a rebound
	ready_positions[2] = Point(0.5 * the_field.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS);
	ready_positions[3] = Point(0.5 * the_field.length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS);

}

void PenaltyFriendly::tick() {
	if (robots.size() == 0) {
		std::cerr << "penalty_friendly: no robots " << std::endl;
		return;
	}
	// Set lowest numbered robot without chicker fault to be kicker
	if (robots[0]->chicker_ready_time() >= Player::CHICKER_FOREVER){
		for (size_t i = 1; i < robots.size(); ++i)
			if (robots[i]->chicker_ready_time() < Player::CHICKER_FOREVER){
				swap(robots[0],robots[i]);
				break;
			}
	}

	// flags... hopefully set correct
	const unsigned int flags = AIFlags::calc_flags(the_world->playtype());

	if (the_world->playtype() == PlayType::PREPARE_PENALTY_FRIENDLY) {
		for (size_t i = 0; i < robots.size(); ++i) {
			Move tactic(robots[i], the_world);
			tactic.set_position(ready_positions[i]);
			if (i) tactic.set_flags(flags);
			else tactic.set_flags((flags & ~AIFlags::AVOID_BALL_STOP) | AIFlags::AVOID_BALL_NEAR);
			tactic.tick();
		}
	} else if (the_world->playtype() == PlayType::EXECUTE_PENALTY_FRIENDLY) {

		// make shooter shoot
		const Player::ptr shooter = robots[0];
		Shoot tactic(shooter, the_world);
		if (the_world->playtype_time() > AIUtil::PLAYTYPE_WAIT_TIME) {
			tactic.force();
			tactic.set_pivot(false);
		}
		// don't set flags, otherwise robot will try to avoid ball
		tactic.tick();

		for (size_t i = 1; i < robots.size(); ++i) {
			Move tactic(robots[i], the_world);
			tactic.set_position(ready_positions[i]);
			tactic.set_flags(flags);
			tactic.tick();
		}
	} else {
		std::cerr << "penalty_friendly: unhandled playtype" << std::endl;
	}
}

void PenaltyFriendly::robots_changed() {

}

