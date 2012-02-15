#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include <cmath>

using namespace AI::HL::W;

namespace {
	BoolParam possess_ball_is_has_ball("possess ball is has ball", "STP/util", true);
	DoubleParam ball_close_factor("distance for ball possession (x ball radius)", "STP/util", 2.0, 1.0, 3.0);
}

#warning hardware depending parameters should move somewhere else
DegreeParam AI::HL::Util::shoot_accuracy("Shooting Accuracy General (degrees)", "STP/util", 10.0, 0.0, 80.0);

DoubleParam AI::HL::Util::dribble_timeout("if dribble > this time, force shoot (sec)", "STP/util", 2.0, 0.0, 20.0);

DoubleParam AI::HL::Util::get_ready_time("time we can prepare during special plays (sec)", "STP/util", 3.0, -1e99, 10.0);

const double AI::HL::Util::POS_CLOSE = AI::HL::W::Robot::MAX_RADIUS / 4.0;

const double AI::HL::Util::POS_EPS = 1e-12;

const double AI::HL::Util::VEL_CLOSE = 1e-2;

const double AI::HL::Util::VEL_EPS = 1e-12;

const Angle AI::HL::Util::ORI_PASS_CLOSE = Angle::of_degrees(45.0);

const double AI::HL::Util::HAS_BALL_ALLOWANCE = 3.0;

const double AI::HL::Util::HAS_BALL_TIME = 2.0 / 15.0;

bool AI::HL::Util::point_in_friendly_defense(const Field &field, const Point p) {
	const double defense_stretch = field.defense_area_stretch();
	const double defense_radius = field.defense_area_radius();
	const Point friendly_goal = field.friendly_goal();
	const Point pole1 = Point(friendly_goal.x, defense_stretch / 2);
	const Point pole2 = Point(friendly_goal.x, -defense_stretch / 2);
	double dist1 = (p - pole1).len();
	double dist2 = (p - pole2).len();
	if (p.x > friendly_goal.x && p.x < friendly_goal.x + defense_radius && p.y > -defense_stretch / 2 && p.y < defense_stretch / 2) {
		return true;
	}
	if (dist1 < defense_radius || dist2 < defense_radius) {
		return true;
	}
	return false;
}

Point AI::HL::Util::crop_point_to_field(const Field &field, const Point p) {
	double x = p.x;
	double y = p.y;
	if (p.x > field.length() / 2) {
		x = field.length() / 2;
	}
	if (p.x < -(field.length() / 2)) {
		x = -(field.length() / 2);
	}
	if (p.y > field.width() / 2) {
		y = field.width() / 2;
	}
	if (p.y < -(field.width() / 2)) {
		y = -(field.width() / 2);
	}

	return Point(x, y);
}

bool AI::HL::Util::path_check(const Point &begin, const Point &end, const std::vector<Point> &obstacles, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (std::size_t i = 0; i < obstacles.size(); ++i) {
		const Point ray = obstacles[i] - begin;
		const double proj = ray.dot(direction);
		const double perp = fabs(ray.cross(direction));
		if (proj <= 0) {
			continue;
		}
		if (proj < dist && perp < thresh) {
			return false;
		}
	}
	return true;
}

#warning TODO: add more features to this function
bool AI::HL::Util::path_check(const Point &begin, const Point &end, const std::vector<Robot::Ptr> &robots, const double thresh) {
	const Point direction = (end - begin).norm();
	const double dist = (end - begin).len();
	for (std::size_t i = 0; i < robots.size(); ++i) {
		const Point ray = robots[i]->position() - begin;
		const double proj = ray.dot(direction);
		const double perp = fabs(ray.cross(direction));
		if (proj <= 0) {
			continue;
		}
		if (proj < dist && perp < thresh) {
			return false;
		}
	}
	return true;
}

#warning TODO: maybe the source to a point instead of defaulting to ball.
bool AI::HL::Util::can_receive(World &world, const Player::Ptr passee) {
	const Ball &ball = world.ball();
	if ((ball.position() - passee->position()).lensq() < POS_CLOSE) {
		LOG_ERROR("can_pass: passee too close to ball");
		return true;
	}
	// if the passee is not facing the ball, forget it
	const Point ray = ball.position() - passee->position();
	if (ray.orientation().angle_diff(passee->orientation()) > ORI_PASS_CLOSE) {
		return false;
	}

	const Point direction = ray.norm();
	const double distance = (ball.position() - passee->position()).len();
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		const Robot::Ptr rob = enemy.get(i);
		const Point rp = rob->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) {
			continue;
		}
		if (proj < distance && perp < shoot_accuracy.get().to_degrees() + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::Ptr plr = friendly.get(i);
		if (possess_ball(world, plr) || plr == passee) {
			continue;
		}
		const Point rp = plr->position() - passee->position();
		const double proj = rp.dot(direction);
		const double perp = sqrt(rp.dot(rp) - proj * proj);
		if (proj <= 0) {
			continue;
		}
		if (proj < distance && perp < shoot_accuracy.get().to_degrees() + Robot::MAX_RADIUS + Ball::RADIUS) {
			return false;
		}
	}
	return true;
}

std::pair<Point, Angle> AI::HL::Util::calc_best_shot(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
	const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);
	return angle_sweep_circles(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
}

std::vector<std::pair<Point, Angle> > AI::HL::Util::calc_best_shot_all(const Field &f, const std::vector<Point> &obstacles, const Point &p, const double radius) {
	const Point p1 = Point(f.length() / 2.0, -f.goal_width() / 2.0);
	const Point p2 = Point(f.length() / 2.0, f.goal_width() / 2.0);
	return angle_sweep_circles_all(p, p1, p2, obstacles, radius * Robot::MAX_RADIUS);
}

std::pair<Point, Angle> AI::HL::Util::calc_best_shot(const World &world, const Player::CPtr player, const double radius) {
	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	const FriendlyTeam &friendly = world.friendly_team();
	obstacles.reserve(enemy.size() + friendly.size());
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr fpl = friendly.get(i);
		if (fpl == player) {
			continue;
		}
		obstacles.push_back(fpl->position());
	}
	std::pair<Point, Angle> best_shot = calc_best_shot(world.field(), obstacles, player->position(), radius);
	// if there is no good shot at least make the
	// target within the goal area
	if (best_shot.second <= Angle::ZERO) {
		Point temp = Point(world.field().length() / 2.0, 0.0);
		best_shot.first = temp;
	}
	return best_shot;
}

std::vector<std::pair<Point, Angle> > AI::HL::Util::calc_best_shot_all(const World &world, const Player::CPtr player, const double radius) {
	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	const FriendlyTeam &friendly = world.friendly_team();
	obstacles.reserve(enemy.size() + friendly.size());
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr fpl = friendly.get(i);
		if (fpl == player) {
			continue;
		}
		obstacles.push_back(fpl->position());
	}
	return calc_best_shot_all(world.field(), obstacles, player->position(), radius);
}


/*
   bool AI::HL::Util::can_shoot_circle(const Point begin, const Point center, const double radius, const std::vector<Point>& obstacles) {
    const Point dirToBall = (p - target_pos).norm();
    const Point p1 = target_pos + (Robot::MAX_RADIUS * radius * dir_to_ball).rotate(M_PI / 2);
    const Point p2 = target_pos - (Robot::MAX_RADIUS * radius * dir_to_ball).rotate(M_PI / 2);
    if (triangle_circle_intersect(begin, p1, p2, obstacles[i], radius)) {
        return false;
    }

    return true;
   }
 */

bool AI::HL::Util::ball_close(const World &world, Robot::Ptr robot) {
	const Point dist = world.ball().position() - robot->position();
	return dist.len() < (Robot::MAX_RADIUS + Ball::RADIUS * ball_close_factor);
}

bool AI::HL::Util::possess_ball(const World &world, Player::Ptr player) {
	if (player->has_ball()) {
		return true;
	}
	if (possess_ball_is_has_ball) {
		return false;
	}
	return ball_close(world, player);
}

bool AI::HL::Util::possess_ball(const World &world, Robot::Ptr robot) {
	return ball_close(world, robot);
}

Player::Ptr AI::HL::Util::calc_baller(World &world, const std::vector<Player::Ptr> &players) {
	for (std::size_t i = 0; i < players.size(); ++i) {
		if (possess_ball(world, players[i])) {
			return players[i];
		}
	}
	return Player::Ptr();
}

std::vector<Player::CPtr> AI::HL::Util::get_players(const FriendlyTeam &friendly) {
	std::vector<Player::CPtr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		players.push_back(friendly.get(i));
	}
	return players;
}

std::vector<Player::Ptr> AI::HL::Util::get_players(FriendlyTeam &friendly) {
	std::vector<Player::Ptr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		players.push_back(friendly.get(i));
	}
	return players;
}

std::vector<Player::Ptr> AI::HL::Util::get_players_exclude(FriendlyTeam &friendly, std::vector<Player::Ptr> &others) {
	std::vector<Player::Ptr> players;
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (!exists(others.begin(), others.end(), friendly.get(i))) {
			players.push_back(friendly.get(i));
		}
	}
	return players;
}

std::vector<Robot::Ptr> AI::HL::Util::get_robots(const EnemyTeam &enemy) {
	std::vector<Robot::Ptr> robots;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		robots.push_back(enemy.get(i));
	}
	return robots;
}

