#include "ai/hl/stp/action/goalie.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/actions.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam lone_goalie_dist("Lone Goalie: distance to goal post (m)", "STP/Action/Goalie" , 0.30, 0.05, 1.0);
	DoubleParam ball_dangerous_speed("Goalie Action: threatening ball speed", "STP/Action/Goalie", 0.1, 0.1, 10.0); 
}

void AI::HL::STP::Action::lone_goalie(const World &world, Player::Ptr player) {

	// Patrol
	const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;
	target = target * (lone_goalie_dist / target.len());
	target += centre_of_goal;
	player->move(target, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::MEDIUM);

	goalie_move(world, player, target);
}

void AI::HL::STP::Action::goalie_move(const World &world, Player::Ptr player, Point dest) {
	// if ball is inside the defense area, must repel!
	if (AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position())) {
		repel(world, player, 0);
		return;
	}

	// Check if ball is threatening to our goal
	Point ballvel = world.ball().velocity();
	Point ballpos = world.ball().position();
	Point goalpos;
	if (ballvel.len() > ball_dangerous_speed && ballvel.x < -1e-6) {
		Point rushpos = line_intersect(ballpos, ballpos + ballvel, 
				Point(-world.field().length()/2.0 + 1.5*Robot::MAX_RADIUS, 1.0),
				Point(-world.field().length()/2.0 + 1.5*Robot::MAX_RADIUS, -1.0));

		goalpos = line_intersect(ballpos, ballpos + ballvel, 
				Point(world.field().length()/2.0, 1.0),
				Point(world.field().length()/2.0, -1.0));
		// LOG_INFO(Glib::ustring::compose("ball heading towards our side of the field: rushpos.y = %1, goalpos.y = %2", rushpos.y, goalpos.y));

		if (std::min(std::fabs(goalpos.y),std::fabs(rushpos.y)) < world.field().goal_width()/2.0) {
			rushpos.y = std::min(rushpos.y, world.field().goal_width()/2.0);
			rushpos.y = std::max(rushpos.y, world.field().goal_width()/2.0);

			player->move(rushpos, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MoveType::RAM_BALL, AI::Flags::MovePrio::HIGH);
			return;
		}
	}

	player->move(dest, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::MEDIUM);
}

void AI::HL::STP::Action::penalty_goalie(const World &world, Player::Ptr player) {

	bool goto_target1 = true;	
	const Point p1(-0.5 * world.field().length(), -0.8 * Robot::MAX_RADIUS);
	const Point p2(-0.5 * world.field().length(), 0.8 * Robot::MAX_RADIUS);
	if ((player->position() - p1).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = false;
	} else if ((player->position() - p2).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = true;
	}
		
	Point target;
	if (goto_target1) {
		target = p1;
	} else {
		target = p2;
	}

	// just orient towards the "front"
	player->move(target, 0, 0, AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::HIGH);
	
}

