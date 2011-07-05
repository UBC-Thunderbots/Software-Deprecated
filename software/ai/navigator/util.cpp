#include "ai/navigator/util.h"
#include "ai/flags.h"
#include "util/time.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "util/param.h"

using namespace AI::Flags;
using namespace AI::Nav::W;

namespace {
	const double EPS = 1e-9;

	BoolParam OWN_HALF_OVERRIDE( "enforce that robots stay on own half ", "Nav/Util", false);

	// small value to ensure non-equivilance with floating point math
	// but too small to make a difference in the actual game
	const double SMALL_BUFFER = 0.0001;

	DoubleParam GOAL_POST_BUFFER( "The amount robots should stay away from goal post" , "Nav/Util", 0.0, -0.2, 0.2);

	// zero lets them brush
	// positive enforces amount meters away
	// negative lets them bump
	// const double ENEMY_BUFFER = 0.1;
	DoubleParam ENEMY_BUFFER("The amount of distance to maintain from enemy robots", "Nav/Util", 0.1, -1.0, 1.0);
	// zero lets them brush
	// positive enforces amount meters away
	// negative lets them bump
	DoubleParam FRIENDLY_BUFFER("Buffer for equal priority friendly robot (meters)", "Nav/Util", 0.1, -1.0, 1.0);
	DoubleParam FRIENDLY_BUFFER_HIGH("Buffer for higher priority friendly robot (meters)", "Nav/Util", 0.2, -1.0, 1.0);
	DoubleParam FRIENDLY_BUFFER_LOW("Buffer for lower priority friendly robot (meters)", "Nav/Util", 0.1, -1.0, 1.0);

	// This buffer is in addition to the robot radius
	const double BALL_STOP_BUFFER = 0.5;

	// This buffer is in addition to the robot radius
	DoubleParam BALL_TINY_BUFFER("Buffer avoid ball tiny (meters)", "Nav/Util", 0.05, -1.0, 1.0);

	// This buffer is in addition to the robot radius
	DoubleParam DEFENSE_AREA_BUFFER("Buffer avoid defense area (meters)", "Nav/Util", 0.0, -1.0, 1.0);

	// this is by how much we should stay away from the playing boundry
	DoubleParam PLAY_AREA_BUFFER("Buffer for staying away from play area boundary ", "Nav/Util", 0.0, 0.0, 1.0);
	DoubleParam OWN_HALF_BUFFER("Buffer for staying on own half ", "Nav/Util", 0.0, 0.0, 1.0);
	DoubleParam TOTAL_BOUNDS_BUFFER("Buffer for staying away from referee area boundary ", "Nav/Util", 0.0, -1.0, 1.0);

	DoubleParam PENALTY_KICK_BUFFER("Amount behind ball during Penalty kick (rule=0.4) ", "Nav/Util", 0.4, 0.0, 1.0);

	//	const double PENALTY_KICK_BUFFER = 0.4;
	DoubleParam FRIENDLY_KICK_BUFFER("Additionaly offense area buffer for friendly kick (rule=0.2) ", "Nav/Util", 0., 0.0, 1.0);

	//	const double FRIENDLY_KICK_BUFFER = 0.2;

	const double RAM_BALL_ALLOWANCE = 0.05;

	const double BALL_STOP = 0.05;

	// this structure determines how far away to stay from a prohibited point or line-segment
	struct distance_keepout {
		static double play_area(AI::Nav::W::Player::Ptr player) {
			return (player->MAX_RADIUS) + PLAY_AREA_BUFFER;
		}
		static double total_bounds_area(AI::Nav::W::Player::Ptr player) {
			return (player->MAX_RADIUS) + PLAY_AREA_BUFFER;
		}
		static double enemy(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			if (world.enemy_team().size() <= 0) {
				return 0.0;
			}
			return player->MAX_RADIUS + world.enemy_team().get(0)->MAX_RADIUS + ENEMY_BUFFER;
		}

		static double goal_post(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player){
			return Ball::RADIUS + player->MAX_RADIUS + GOAL_POST_BUFFER;
		}

		static double friendly(AI::Nav::W::Player::Ptr player, MovePrio obs_prio = MovePrio::MEDIUM) {
			MovePrio player_prio = player->prio();
			double buffer = FRIENDLY_BUFFER;
			if (obs_prio < player_prio) {
				buffer = FRIENDLY_BUFFER_LOW;
			} else if (player_prio < obs_prio) {
				buffer = FRIENDLY_BUFFER_HIGH;
			}
			return 2.0 * (player->MAX_RADIUS) + buffer;
		}
		static double ball_stop(AI::Nav::W::Player::Ptr player) {
			return Ball::RADIUS + player->MAX_RADIUS + BALL_STOP_BUFFER;
		}
		static double ball_tiny(AI::Nav::W::Player::Ptr player) {
			return Ball::RADIUS + player->MAX_RADIUS + BALL_TINY_BUFFER;
		}
		static double friendly_defense(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			return world.field().defense_area_radius() + player->MAX_RADIUS + DEFENSE_AREA_BUFFER;
		}
		static double enemy_defense(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			return world.field().defense_area_radius() + player->MAX_RADIUS + DEFENSE_AREA_BUFFER;
		}

		static double own_half(AI::Nav::W::Player::Ptr player) {
			return player->MAX_RADIUS + OWN_HALF_BUFFER;
		}
		static double penalty_kick_friendly(AI::Nav::W::Player::Ptr player) {
			return player->MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
		}
		static double penalty_kick_enemy(AI::Nav::W::Player::Ptr player) {
			return player->MAX_RADIUS + PENALTY_KICK_BUFFER + Ball::RADIUS;
		}
		static double friendly_kick(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			return enemy_defense(world, player) + FRIENDLY_KICK_BUFFER;
		}
	};

	double get_goal_post_tresspass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player){

		double violate = 0.0;
		double circle_radius = distance_keepout::goal_post(world, player);
		Point A(world.field().length()/2.0, world.field().goal_width()/2.0);
		Point B(world.field().length()/2.0, -world.field().goal_width()/2.0);
		Point C(-world.field().length()/2.0, world.field().goal_width()/2.0);
		Point D(-world.field().length()/2.0, -world.field().goal_width()/2.0);
		Point pts[4] = {A,B,C,D};
		for(unsigned int i=0; i<4; i++){
			double dist = lineseg_point_dist(pts[i], cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	double get_enemy_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double violate = 0.0;
		double circle_radius = distance_keepout::enemy(world, player);
		// avoid enemy robots
		for (std::size_t i = 0; i < world.enemy_team().size(); i++) {
			AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
			double dist = lineseg_point_dist(rob->position(), cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	double get_play_area_boundary_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		Point sw_corner(f.length() / 2, f.width() / 2);
		Rect bounds(sw_corner, f.length(), f.width());
		bounds.expand(-distance_keepout::play_area(player));
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_total_bounds_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		Point sw_corner(f.total_length() / 2, f.total_width() / 2);
		Rect bounds(sw_corner, f.total_length(), f.total_width());
		bounds.expand(-distance_keepout::total_bounds_area(player));
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_friendly_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		// double player_rad = player->MAX_RADIUS;
		double violate = 0.0;
		// avoid enemy robots
		for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
			AI::Nav::W::Player::Ptr rob = world.friendly_team().get(i);
			if (rob == player) {
				continue;
			}
			// double friendly_rad = rob->MAX_RADIUS;
			// double circle_radius = player_rad + friendly_rad + FRIENDLY_BUFFER;

			double circle_radius = distance_keepout::friendly(player, rob->prio());
			double dist = lineseg_point_dist(rob->position(), cur, dst);
			violate = std::max(violate, circle_radius - dist);
		}
		return violate;
	}

	double get_ball_stop_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		double violate = 0.0;
		const Ball &ball = world.ball();
		double circle_radius = distance_keepout::ball_stop(player);
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		violate = std::max(violate, circle_radius - dist);
		return violate;
	}

	double get_defense_area_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		double defense_dist = distance_keepout::friendly_defense(world, player);
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	double get_friendly_kick_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		double defense_dist = distance_keepout::friendly_kick(world, player);
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	double get_own_half_trespass(Point, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		Point p(-f.total_length() / 2, -f.total_width() / 2);
		Rect bounds(p, f.total_length() / 2, f.total_width());
		bounds.expand(-distance_keepout::own_half(player));
		double violation = 0.0;
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_offense_area_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Field &f = world.field();
		double defense_dist = distance_keepout::friendly_defense(world, player);
		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		return std::max(0.0, defense_dist - seg_seg_distance(cur, dst, defense_point1, defense_point2));
	}

	double get_ball_tiny_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Ball &ball = world.ball();
		double circle_radius = distance_keepout::ball_tiny(player);
		double dist = lineseg_point_dist(ball.position(), cur, dst);
		return std::max(0.0, circle_radius - dist);
	}

	double get_penalty_friendly_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Ball &ball = world.ball();
		const Field &f = world.field();
		Point a(ball.position().x - distance_keepout::penalty_kick_friendly(player), -f.total_width() / 2);
		Point b(f.total_length() / 2, f.total_width() / 2);
		Rect bounds(a, b);
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	double get_penalty_enemy_trespass(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
		const Ball &ball = world.ball();
		const Field &f = world.field();
		Point a(ball.position().x + distance_keepout::penalty_kick_enemy(player), -f.total_width() / 2);
		Point b(f.total_length() / 2, f.total_width() / 2);
		Rect bounds(a, b);
		double violation = 0.0;
		if (!bounds.point_inside(cur)) {
			violation = std::max(violation, bounds.dist_to_boundary(cur));
		}
		if (!bounds.point_inside(dst)) {
			violation = std::max(violation, bounds.dist_to_boundary(dst));
		}
		return violation;
	}

	struct violation {
		double enemy, friendly, play_area, ball_stop, ball_tiny, friendly_defense, enemy_defense, own_half, penalty_kick_friendly, penalty_kick_enemy, friendly_kick, goal_post, total_bounds;

		unsigned int extra_flags;

		void set_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			friendly = get_friendly_trespass(cur, dst, world, player);
			enemy = get_enemy_trespass(cur, dst, world, player);
			goal_post = get_goal_post_tresspass(cur, dst, world, player);
			total_bounds = get_total_bounds_trespass(cur, dst, world, player);
			unsigned int flags = player->flags() | extra_flags;
			if (flags & FLAG_CLIP_PLAY_AREA) {
				play_area = get_play_area_boundary_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_BALL_STOP) {
				ball_stop = get_ball_stop_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_BALL_TINY) {
				ball_tiny = get_ball_tiny_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
				friendly_defense = get_defense_area_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
				friendly_defense = get_offense_area_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_STAY_OWN_HALF) {
				own_half = get_own_half_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_PENALTY_KICK_FRIENDLY) {
				penalty_kick_friendly = get_penalty_friendly_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_PENALTY_KICK_ENEMY) {
				penalty_kick_enemy = get_penalty_enemy_trespass(cur, dst, world, player);
			}
			if (flags & FLAG_FRIENDLY_KICK) {
				friendly_kick = get_friendly_kick_trespass(cur, dst, world, player);
			}
		}

		violation() : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), friendly_kick(0.0), goal_post(0.0), total_bounds(0.0), extra_flags(0) {
		}

		violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), friendly_kick(0.0), goal_post(0.0), total_bounds(0.0), extra_flags(0) {
			if(OWN_HALF_OVERRIDE){
				extra_flags = extra_flags | FLAG_STAY_OWN_HALF;
			}
			set_violation_amount(cur, dst, world, player);
		}

		violation(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int added_flags) : enemy(0.0), friendly(0.0), play_area(0.0), ball_stop(0.0), ball_tiny(0.0), friendly_defense(0.0), enemy_defense(0.0), own_half(0.0), penalty_kick_friendly(0.0), penalty_kick_enemy(0.0), friendly_kick(0.0), goal_post(0.0), total_bounds(0.0), extra_flags(added_flags) {
			if(OWN_HALF_OVERRIDE){
				extra_flags = extra_flags | FLAG_STAY_OWN_HALF;
			}
			set_violation_amount(cur, dst, world, player);
		}

		static violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
			violation v(cur, dst, world, player);
			return v;
		}

		static violation get_violation_amount(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int extra_flags) {
			violation v(cur, dst, world, player, extra_flags);
			return v;
		}

		bool no_more_violating_than(violation b) {
			return enemy < b.enemy + EPS && friendly < b.friendly + EPS &&
			play_area < b.play_area + EPS && ball_stop < b.ball_stop + EPS &&
			ball_tiny < b.ball_tiny + EPS && friendly_defense < b.friendly_defense + EPS &&
			enemy_defense < b.enemy_defense + EPS && own_half < b.own_half + EPS &&
			penalty_kick_enemy < b.penalty_kick_enemy + EPS &&
			penalty_kick_friendly < b.penalty_kick_friendly + EPS &&
			goal_post < b.goal_post + EPS &&
			total_bounds < b.total_bounds + EPS;
		}

		bool violation_free() {
			return enemy < EPS && friendly < EPS &&
			       play_area < EPS && ball_stop < EPS &&
			       ball_tiny < EPS && friendly_defense < EPS &&
			       enemy_defense < EPS && own_half < EPS &&
			       penalty_kick_enemy < EPS && penalty_kick_friendly < EPS &&
						 goal_post < EPS && total_bounds < EPS;
		}
	};


	void process_obstacle(std::vector<Point> &ans, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, Point segA, Point segB, double dist, int num_points) {
		// we want a regular polygon where the largest inscribed circle
		// has the keepout distance as it's radius
		// circle radius then becomes the radius of the smallest circle that will contain the polygon
		// plus a small buffer
		double radius = dist / std::cos(M_PI / static_cast<double>(num_points)) + SMALL_BUFFER;
		double TS = 2 * num_points * dist * std::tan(M_PI / num_points);
		double TS2 = TS + 2 * (segA - segB).len();
		int n_tot = num_points * static_cast<int>(std::ceil(TS2 / TS));
		std::vector<Point> temp = seg_buffer_boundaries(segA, segB, radius, n_tot);

		for (std::vector<Point>::const_iterator it = temp.begin(); it != temp.end(); it++) {
			if (AI::Nav::Util::valid_dst(*it, world, player)) {
				ans.push_back(*it);
			}
		}
	}
};

std::vector<Point> AI::Nav::Util::get_destination_alternatives(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	const int POINTS_PER_OBSTACLE = 6;
	std::vector<Point> ans;
	unsigned int flags = player->flags();

	if (flags & FLAG_AVOID_BALL_STOP) {
		process_obstacle(ans, world, player, dst, dst, distance_keepout::friendly(player), 3 * POINTS_PER_OBSTACLE);
	}

	return ans;
}

bool AI::Nav::Util::valid_dst(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return violation::get_violation_amount(dst, dst, world, player).violation_free();
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return violation::get_violation_amount(cur, dst, world, player).no_more_violating_than(violation::get_violation_amount(cur, cur, world, player));
}

bool AI::Nav::Util::valid_path(Point cur, Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int extra_flags) {
	return violation::get_violation_amount(cur, dst, world, player, extra_flags).no_more_violating_than(violation::get_violation_amount(cur, cur, world, player, extra_flags));
}


std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	return get_obstacle_boundaries(world, player, 0);
}
#warning some magic numbers here need to clean up a bit
std::vector<Point> AI::Nav::Util::get_obstacle_boundaries(AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player, unsigned int added_flags) {
	// this number must be >=3
	const int POINTS_PER_OBSTACLE = 6;
	std::vector<Point> ans;
	unsigned int flags = player->flags() | added_flags;
	const Field &f = world.field();

	if (flags & FLAG_AVOID_BALL_STOP) {
		process_obstacle(ans, world, player, world.ball().position(), world.ball().position(), distance_keepout::ball_stop(player), 3 * POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_STAY_OWN_HALF) {
		Point half_point1(0.0, -f.width() / 2);
		Point half_point2(0.0, f.width() / 2);
		process_obstacle(ans, world, player, half_point1, half_point2, distance_keepout::own_half(player), 7 * POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_AVOID_FRIENDLY_DEFENSE) {
		Point defense_point1(-f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(-f.length() / 2, f.defense_area_stretch() / 2);
		process_obstacle(ans, world, player, defense_point1, defense_point2, distance_keepout::friendly_defense(world, player), POINTS_PER_OBSTACLE);
	}

	if (flags & FLAG_AVOID_ENEMY_DEFENSE) {
		Point defense_point1(f.length() / 2, -f.defense_area_stretch() / 2);
		Point defense_point2(f.length() / 2, f.defense_area_stretch() / 2);
		process_obstacle(ans, world, player, defense_point1, defense_point2, distance_keepout::enemy_defense(world, player), POINTS_PER_OBSTACLE);
	}

	if ((flags & FLAG_AVOID_BALL_TINY) && !(flags & FLAG_AVOID_BALL_STOP)) {
		process_obstacle(ans, world, player, world.ball().position(), world.ball().position(), distance_keepout::ball_tiny(player), POINTS_PER_OBSTACLE);
	}

	for (std::size_t i = 0; i < world.friendly_team().size(); i++) {
		AI::Nav::W::Player::Ptr rob = world.friendly_team().get(i);
		if (rob == player) {
			// points around self may help with trying to escape when stuck
			// that is why there are double the number of points here
			process_obstacle(ans, world, player, rob->position(), rob->position(), distance_keepout::friendly(player), 2 * POINTS_PER_OBSTACLE);
			continue;
		}
		process_obstacle(ans, world, player, rob->position(), rob->position(), distance_keepout::friendly(player), POINTS_PER_OBSTACLE);
	}

	for (std::size_t i = 0; i < world.enemy_team().size(); i++) {
		AI::Nav::W::Robot::Ptr rob = world.enemy_team().get(i);
		process_obstacle(ans, world, player, rob->position(), rob->position(), distance_keepout::enemy(world, player), POINTS_PER_OBSTACLE);
	}

	return ans;
}

// bool has_ramball_location(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player){
// Point ball_dir = world.ball.velocity();
// return unique_line_intersect(player->position(), dst, world.ball.position(), world.ball.position() + ball_dir);
// }


std::pair<Point, timespec> AI::Nav::Util::get_ramball_location(Point dst, AI::Nav::W::World &world, AI::Nav::W::Player::Ptr player) {
	Point ball_dir = world.ball().velocity();

	if (ball_dir.lensq() < EPS) {
		if (lineseg_point_dist(world.ball().position(), player->position(), dst) < RAM_BALL_ALLOWANCE) {
			return std::make_pair(world.ball().position(), world.monotonic_time());
		}
	}

	if (unique_line_intersect(player->position(), dst, world.ball().position(), world.ball().position() + ball_dir)) {
		Point location = line_intersect(player->position(), dst, world.ball().position(), world.ball().position() + ball_dir);
		timespec intersect = world.monotonic_time();
		timespec_add(intersect, double_to_timespec((location - world.ball().position()).len() / world.ball().velocity().len()), intersect);

		Point vec1 = location - player->position();
		Point vec2 = dst - player->position();

		Point ball_vec = location - world.ball().position();

		if (vec1.dot(vec2) > 0 && ball_dir.dot(ball_vec)) {
			return std::make_pair(location, intersect);
		}
	}

	// if everything fails then just stay put
	return std::make_pair(player->position(), world.monotonic_time());
}

timespec AI::Nav::Util::get_next_ts(timespec now, Point &p1, Point &p2, Point target_velocity) {
	double velocity, distance;
	velocity = target_velocity.len();
	distance = (p1 - p2).len();
	return timespec_add(now, double_to_timespec(velocity * distance));
}

