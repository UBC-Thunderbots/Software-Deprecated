#include "ai/hl/stp/evaluation/ball_threat.h"
#include "geom/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"

#include <algorithm>

using namespace AI::HL::W;
using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	DoubleParam steal_threshold("Steal threshold: distance of ball from enemy (robot radius)", "STP/evaluation", 1.1, 0.8, 5.0);

	DoubleParam negligible_velocity("speed goalie should ignore direction of ball", "STP/evaluation", 0.05, 1e-4, 1.0);
}

BallThreat AI::HL::STP::Evaluation::evaluate_ball_threat(const World &world) {
	BallThreat ball_threat;

	ball_threat.enemies = AI::HL::Util::get_robots(world.enemy_team());
	ball_threat.activate_steal = false;

	if (ball_threat.enemies.size() > 0) {
		std::sort(ball_threat.enemies.begin(), ball_threat.enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));

		ball_threat.threat = ball_threat.enemies[0];

		ball_threat.threat_distance = (ball_threat.threat->position() - world.ball().position()).len();
		
		for (std::size_t i = 0; i < ball_threat.enemies.size(); ++i) {
		
			Point pos = world.ball().position(), vel = world.ball().velocity();
			Point a = ball_threat.enemies[i]->position();
			Point b = a + ball_threat.enemies[i]->velocity();
			Point c = pos;
			Point d = pos + vel;

			if (unique_line_intersect(a, b, c, d)) {
				Point inter = line_intersect(a, b, c, d);
				ball_threat.activate_steal = !(vel.len() > negligible_velocity && inter.y <= std::max(a.y, b.y) && inter.y >= std::min(a.y, b.y));
			}
		}

		// steal mechanism activation when ball is close enough
		if ((ball_threat.threat->position() - world.ball().position()).len() * Robot::MAX_RADIUS < steal_threshold) {
			ball_threat.activate_steal = true;
		} 
		
	}

	return ball_threat;
}

bool AI::HL::STP::Evaluation::ball_on_net(const AI::HL::W::World &world) {
	if (world.ball().velocity().lensq() < negligible_velocity || world.ball().velocity().x > 0) {
		return false;
	}

	Point a = world.field().friendly_goal_boundary().first;
	Point b = world.field().friendly_goal_boundary().second;
	Point c = world.ball().position();
	Point d = world.ball().position() + world.ball().velocity();

	if (unique_line_intersect(a, b, c, d)) {
		Point inter = line_intersect(a, b, c, d);
		return inter.y <= std::max(a.y, b.y) && inter.y >= std::min(a.y, b.y);
	}
	return false;
}

bool AI::HL::STP::Evaluation::ball_on_enemy_net(const AI::HL::W::World &world) {
	if (world.ball().velocity().lensq() < negligible_velocity || world.ball().velocity().x < 0) {
		return false;
	}

	Point a = world.field().enemy_goal_boundary().first;
	Point b = world.field().enemy_goal_boundary().second;
	Point c = world.ball().position();
	Point d = world.ball().position() + world.ball().velocity();

	if (unique_line_intersect(a, b, c, d)) {
		Point inter = line_intersect(a, b, c, d);
		return inter.y <= std::max(a.y, b.y) && inter.y >= std::min(a.y, b.y);
	}
	return false;
}

Point AI::HL::STP::Evaluation::goalie_shot_block(const AI::HL::W::World &world, const AI::HL::W::Player::Ptr player) {
	if (world.friendly_team().size() <= 0 || !ball_on_net(world)) {
		return Point(0, 0);
	}

	Point goalie_pos = player->position();

	Point a = world.field().friendly_goal_boundary().first;
	Point b = world.field().friendly_goal_boundary().second;

	Point c = world.ball().position();
	Point d = world.ball().position() + world.ball().velocity();
	Point inter = line_intersect(a, b, c, d);

	return closest_lineseg_point(goalie_pos, c, inter);
}

