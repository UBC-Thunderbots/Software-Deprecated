#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/param.h"
#include "geom/angle.h"
#include "geom/util.h"

using namespace AI::HL::STP;

namespace {

	DoubleParam friendly_pass_width("Friendly pass checking width (robot radius)", "STP/offense", 2, 0, 9);

	DoubleParam enemy_pass_width("Enemy pass checking width (robot radius)", "STP/enemy", 2, 0, 9);

	bool can_pass_check(const Point p1, const Point p2, const std::vector<Point>& obstacles, double tol) {

		// auto allowance = AI::HL::Util::calc_best_shot_target(passer->position(), obstacles, passee->position(), 1).second;
		// return allowance > degrees2radians(enemy_shoot_accuracy);

		// OLD method is TRIED and TESTED
		return AI::HL::Util::path_check(p1, p2, obstacles, Robot::MAX_RADIUS * tol);
	}

#warning TOOD: refactor
	bool ray_on_friendly_goal(const World& world, const Point c, const Point d) {
		Point a = world.field().friendly_goal_boundary().first;
		Point b = world.field().friendly_goal_boundary().second;

		if (unique_line_intersect(a, b, c, d)) {
			Point inter = line_intersect(a, b, c, d);
			return inter.y <= std::max(a.y, b.y) && inter.y >= std::min(a.y, b.y);
		}
		return false;
	}

	DoubleParam pass_ray_threat_mult("Ray pass threat multiplier", "STP/PassRay", 2, 1, 99);

}

DoubleParam Evaluation::max_pass_ray_angle("Max ray shoot rotation (degrees)", "STP/PassRay", 75, 0, 180);

IntParam Evaluation::ray_intervals("Ray # of intervals", "STP/PassRay", 30, 0, 80);

bool Evaluation::can_shoot_ray(const World& world, Player::CPtr player, double orientation) {
	const Point p1 = player->position();
	const Point p2 = p1 + 10 * Point::of_angle(orientation);

	double diff = angle_diff(player->orientation(), orientation);
	if (diff > degrees2radians(max_pass_ray_angle)) {
		return false;
	}

	// check if the ray heads towards our net
	if (ray_on_friendly_goal(world, p1, p2)) {
		return false;
	}

	const FriendlyTeam &friendly = world.friendly_team();
	const EnemyTeam &enemy = world.enemy_team();

	double closest_enemy = 1e99;
	double closest_friendly = 1e99;

	for (std::size_t i = 0; i < friendly.size(); ++i) {
		Player::CPtr fptr = friendly.get(i);
		if (fptr == player) continue;
		double dist = seg_pt_dist(p1, p2, fptr->position());
		closest_friendly = std::min(closest_friendly, dist);
	}

	for (std::size_t i = 0; i < enemy.size(); ++i) {
		Robot::Ptr robot = enemy.get(i);
		double dist = seg_pt_dist(p1, p2, robot->position());
		closest_enemy = std::min(closest_enemy, dist);
	}

	return closest_friendly * pass_ray_threat_mult <= closest_enemy;
}

std::pair<bool, double> Evaluation::best_shoot_ray(const World& world, const Player::CPtr player) {
	if (!Evaluation::possess_ball(world, player)) {
		return std::make_pair(false, 0);
	}

	double best_diff = 1e99;
	double best_angle;

	// draw rays for ray shooting

	const double angle_span = 2 * degrees2radians(max_pass_ray_angle);
	const double angle_step = angle_span / Evaluation::ray_intervals;
	const double angle_min = player->orientation() - angle_span / 2;

	for (int i = 0; i < Evaluation::ray_intervals; ++i) {
		const double angle = angle_min + angle_step * i;

		const Point p1 = player->position();
		const Point p2 = p1 + 3 * Point::of_angle(angle);

		double diff = angle_diff(player->orientation(), angle);

		if (diff > best_diff) {
			continue;
		}

		// ok
		if (!Evaluation::can_shoot_ray(world, player, angle)) {
			continue;
		}

		if (diff < best_diff) {
			best_diff = diff;
			best_angle = angle;
		}
	}

	// cant find good angle
	if (best_diff > 1e50) {
		return std::make_pair(false, 0);
	}

	return std::make_pair(true, best_angle);
}

bool Evaluation::enemy_can_pass(const World &world, const Robot::Ptr passer, const Robot::Ptr passee) {
	std::vector<Point> obstacles;
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		obstacles.push_back(friendly.get(i)->position());
	}

	return can_pass_check(passer->position(), passee->position(), obstacles, enemy_pass_width);
}

bool Evaluation::can_pass(const World& world, Player::CPtr passer, Player::CPtr passee) {

	std::vector<Point> obstacles;
	const EnemyTeam& enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}
	const FriendlyTeam& friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (friendly.get(i) == passer) continue;
		if (friendly.get(i) == passee) continue;
		obstacles.push_back(friendly.get(i)->position());
	}

	return can_pass_check(passer->position(), passee->position(), obstacles, friendly_pass_width);
}

bool Evaluation::can_pass(const World& world, const Point p1, const Point p2) {
	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}

	return can_pass_check(p1, p2, obstacles, friendly_pass_width);
}

bool Evaluation::passee_facing_ball(const World& world, Player::CPtr passee) {
	return player_within_angle_thresh(passee, world.ball().position(), passee_angle_threshold);
}

bool Evaluation::passee_facing_passer(Player::CPtr passer, Player::CPtr passee) {
	return player_within_angle_thresh(passee, passer->position(), passee_angle_threshold);
}

bool Evaluation::passee_suitable(const World& world, Player::CPtr passee) {

	// can't pass backwards
	if (passee->position().x < world.ball().position().x) {
		return false;
	}

	// must be at least some distance
	if ((passee->position() - world.ball().position()).len() < min_pass_dist) {
		return false;
	}

	// must be able to pass
	if (!Evaluation::can_pass(world, world.ball().position(), passee->position())) {
		return false;
	}

	/*
	   if (!Evaluation::passee_facing_ball(world, passee)) {
	   return false;
	   }
	 */

	return true;
}

namespace {
	// hysterysis for select_passee
	Player::CPtr previous_passee;
}

Player::CPtr Evaluation::select_passee(const World& world) {
	const FriendlyTeam& friendly = world.friendly_team();
	std::vector<Player::CPtr> candidates;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (possess_ball(world, friendly.get(i))) {
			continue;
		}
		if (!passee_suitable(world, friendly.get(i))) {
			continue;
		}

		if (friendly.get(i) == previous_passee) {
			return friendly.get(i);
		}

		candidates.push_back(friendly.get(i));
	}
	if (candidates.size() == 0) {
		return Player::CPtr();
	}
	random_shuffle(candidates.begin(), candidates.end());
	previous_passee = candidates.front();
	return candidates.front();
}
