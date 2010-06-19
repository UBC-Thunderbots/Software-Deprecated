#include "ai/role/defensive2.h"
#include "ai/role/offensive.h"
#include "ai/tactic/move.h"
#include "ai/tactic/shoot.h"
#include "ai/tactic/pass.h"
#include "ai/util.h"
#include "util/algorithm.h"
#include "geom/util.h"
#include "util/dprint.h"

#include <iostream>

namespace {
	// minimum distance from the goal post
	const double MIN_GOALPOST_DIST = 0.05;
}

defensive2::defensive2(world::ptr world) : the_world(world) {
}

void defensive2::assign(const player::ptr& p, tactic::ptr t) {
	for (size_t i = 0; i < the_robots.size(); ++i) {
		if (the_robots[i] == p) {
			tactics[i] = t;
			return;
		}
	}
	// ERROR MESSAGE HERE
}

std::pair<point, std::vector<point> > defensive2::calc_block_positions(const bool top) const {
	const enemy_team& enemy(the_world->enemy);

	if (enemy.size() == 0) {
		std::cerr << "defensive2: no enemy!?" << std::endl;
	}

	const field& f = the_world->field();

	// Sort enemies by distance from own goal.
	std::vector<robot::ptr> enemies = enemy.get_robots();
	std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(the_world->ball()->position()));

	std::vector<point> waypoints;

	// there is cone ball to goal sides, bounded by 1 rays.
	const point& ballpos = the_world->ball()->position();
	const point goalside = top ? point(-f.length()/2, f.goal_width()/2) : point(-f.length(), -f.goal_width()/2);
	const point goalopp = top ? point(-f.length()/2, -f.goal_width()/2) : point(-f.length(), f.goal_width()/2);

	// goalie and first defender integrated defence
	// maximum x-distance the goalie can go from own goal.
	/*
	point G;
	if (point_in_defense(the_world, ballpos)) {
		// panic and shoot out the ball
		G = ballpos;
	} else {
	 */
	// normally
	const double maxdist = f.defense_area_radius() - robot::MAX_RADIUS;
	const point L = line_intersect(goalside, ballpos, f.friendly_goal() + point(maxdist, -1), f.friendly_goal() + point(maxdist, 1));
	const point G = (top) ?  L + calc_block_cone(goalside - ballpos, point(0, -1), robot::MAX_RADIUS)
		: L + calc_block_cone(point(0, 1), goalside - ballpos, robot::MAX_RADIUS);

	// first defender will block the remaining cone from the ball
	const point D1 = calc_block_cone_defender(goalside, goalopp, ballpos, G, robot::MAX_RADIUS);
	waypoints.push_back(D1);
	//}

	// 2nd defender block nearest enemy sight to goal if needed
	// what happen if posses ball? skip or what?
	if (!ai_util::posses_ball(the_world, enemies[0])) {
		// block this enemy
		const point D2 = calc_block_cone(goalside, goalopp, enemies[0]->position(), robot::MAX_RADIUS);
		waypoints.push_back(D2);
	}

	if (enemy.size() == 1) return std::make_pair(G, waypoints);

	// 3rd defender block 2nd nearest enemy sight to goal
	const point D3 = calc_block_cone(goalside, goalopp, enemies[1]->position(), robot::MAX_RADIUS);
	waypoints.push_back(D3);

	// 4th defender go chase?
	waypoints.push_back(the_world->ball()->position());

	return std::make_pair(G, waypoints);
}

void defensive2::tick() {

	if (the_robots.size() == 0) {
		LOG_WARN("no robots");
		return;
	}

	const friendly_team& friendly(the_world->friendly);
	const enemy_team& enemy(the_world->enemy);
	const point& ballpos = the_world->ball()->position();

	if (enemy.size() == 0) {
		LOG_WARN("no enemy");

		unsigned int flags = ai_flags::calc_flags(the_world->playtype());
		shoot tactic(the_robots.back(), the_world);
		tactic.set_flags(flags);
		tactic.tick();

		for (size_t i = 0; i + 1 < the_robots.size(); ++i) {
			move tactic(the_robots[i], the_world);
			tactic.set_flags(flags);
			tactic.tick();
		}
		return;
	}

	// the robot nearest
	double nearestdist = 1e99;
	int nearest = -1;
	for (size_t i = 0; i < the_robots.size(); ++i) {
		const double dist = (the_robots[i]->position() - ballpos).len();
		if (nearest == -1 || dist < nearestdist) {
			nearestdist = dist;
			nearest = static_cast<int>(i);
		}
	}

	// robot 0 is goalie, the others are non-goalie
	if (!goalie) {
		goalie = the_robots[0];
	} else {
		for (size_t i = 0; i < the_robots.size(); ++i) {
			if (the_robots[i] != goalie) continue;
			swap(the_robots[i], the_robots[0]);
			break;
		}
	}

	std::vector<player::ptr> defenders;
	for (size_t i = 1; i < the_robots.size(); ++i)
		defenders.push_back(the_robots[i]);

	// Sort by distance to ball. DO NOT SORT IT AGAIN!!
	std::sort(defenders.begin(), defenders.end(), ai_util::cmp_dist<player::ptr>(the_world->ball()->position()));
	std::vector<player::ptr> friends = ai_util::get_friends(friendly, defenders);

	const int baller = ai_util::calc_baller(the_world, defenders);
	// const bool teamball = ai_util::friendly_posses_ball(the_world);

	std::pair<point, std::vector<point> > positions = calc_block_positions();
	std::vector<point>& waypoints = positions.second;

	// do matching for distances

	std::vector<point> locations;
	for (size_t i = 0; i < defenders.size(); ++i) {
		locations.push_back(defenders[i]->position());
	}

	// ensure we are only blocking as we need
	while (waypoints.size() > defenders.size()) waypoints.pop_back();

	std::vector<size_t> order = dist_matching(locations, waypoints);

	// do the actual assigmment

	// check if nearest robot
	if (nearest == 0 || ai_util::point_in_defense(the_world, ballpos)) {
		LOG_INFO("goalie to shoot");
		shoot::ptr tactic(new shoot(the_robots[0], the_world));
		tactic->force();
		tactics[0] = tactic;
	} else {
		move::ptr tactic(new move(the_robots[0], the_world));
		tactic->set_position(positions.first);
		tactics[0] = tactic;
	}

	size_t w = 0; // so we can skip robots as needed
	for (size_t i = 0; i < defenders.size(); ++i) {
		// if (static_cast<int>(i) == skipme) continue;
		if (w >= waypoints.size()) {
			LOG_WARN(Glib::ustring::compose("%1 nothing to do", defenders[i]->name));
			move::ptr tactic(new move(defenders[i], the_world));
			tactic->set_position(defenders[i]->position());
			assign(defenders[i], tactic);
			continue;
		} 

		const point& target = waypoints[order[w]];
		if ((target - ballpos).len() < ai_util::POS_EPS || nearest == static_cast<int>(i)) {
			// should be exact
			shoot::ptr tactic(new shoot(defenders[i], the_world));
			tactic->force();
			assign(defenders[i], tactic);
		} else {
			move::ptr tactic(new move(defenders[i], the_world));
			tactic->set_position(waypoints[order[w]]);
			assign(defenders[i], tactic);
		}
		++w;
	}

	unsigned int flags = ai_flags::calc_flags(the_world->playtype());

	for (size_t i = 0; i < tactics.size(); ++i) {
		if (static_cast<int>(i) == baller) {
			tactics[i]->set_flags(flags | ai_flags::clip_play_area);
		} else {
			tactics[i]->set_flags(flags);
		}
		tactics[i]->tick();
	}
}

void defensive2::robots_changed() {
	tactics.clear();
	tactics.resize(the_robots.size());
}

