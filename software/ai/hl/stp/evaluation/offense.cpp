#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::W;
// using AI::HL::STP::Evaluation::OffenseData;
// using AI::HL::STP::Evaluation::EvaluateOffense;

IntParam AI::HL::STP::Evaluation::grid_x("grid x size", "STP/offense", 25, 1, 100);
IntParam AI::HL::STP::Evaluation::grid_y("grid y size", "STP/offense", 25, 1, 100);

using AI::HL::STP::Evaluation::grid_x;
using AI::HL::STP::Evaluation::grid_y;

namespace {
	//const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/offense", 4.0, 1.0, 10.0);

	DoubleParam ball_dist_weight("ball distance weight", "STP/offense", 1.0, 0.0, 2.0);

	DoubleParam weight_goal("Scoring weight for angle to goal", "STP/offense", 1.0, 0.0, 99.0);

	DoubleParam weight_ball("Scoring weight for angle from ball to goal at robot", "STP/offense", 1.0, 0.0, 99.0);

	double scoring_function(const World &world, const Point &passee_pos, const std::vector<Point> &enemy_pos, const Point &dest, const std::vector<Point> &dont_block, bool pass = false) {
		// can't be too close to enemy
		double closest_enemy = world.field().width();
		for (std::size_t i = 0; i < enemy_pos.size(); ++i) {
			double dist = (enemy_pos[i] - dest).len();
			if (dist < near_thresh * Robot::MAX_RADIUS) {
				return -1e99;
			}
			closest_enemy = std::min(closest_enemy, dist);
		}

		double score_goal = -1e99;
		if (pass) {
			score_goal = AI::HL::Util::calc_best_shot_target(passee_pos, enemy_pos, dest).second;
		} else {
			// Hmm.. not sure if having negative number is a good idea.
			score_goal = AI::HL::Util::calc_best_shot(world.field(), enemy_pos, dest).second;
		}

		// TODO: check the line below here
		// scoring factors:
		// density of enemy, passing distance, distance to the goal, angle of shooting, angle of receiving
		// distance toward the closest enemy, travel distance, behind of in front of the enemy

		// TODO: fix this
		if (!AI::HL::Util::path_check(world.ball().position(), dest, enemy_pos, Robot::MAX_RADIUS + Ball::RADIUS * 3)) {
			return -1e99;
		}

		for (size_t i = 0; i < dont_block.size(); ++i) {
			const Point diff2 = (dest - dont_block[i]);
			if (diff2.len() < near_thresh * Robot::MAX_RADIUS) {
				return -1e99;
			}
		}

		// super expensive calculation
		// basically, ensures that this position does not block the list of positions
		// inside dont_block from view of goal.
		for (size_t i = 0; i < dont_block.size(); ++i) {
			std::pair<Point, double> shootershot = AI::HL::Util::calc_best_shot(world.field(), enemy_pos, dont_block[i]);
			const Point diff1 = (shootershot.first - dont_block[i]);
			const Point diff2 = (dest - dont_block[i]);
			const double anglediff = angle_diff(diff1.orientation(), diff2.orientation());
			if (anglediff * 2 < shootershot.second) {
				return -1e99;
			}
		}

		// want to be as near to enemy goal or ball as possible
		const double ball_dist = (dest - world.ball().position()).len() + ball_dist_weight;
		// const double goal_dist = (dest - bestshot.first).len();

		// TODO: take into account of the angle needed to rotate and shoot
		double d1 = (world.ball().position() - dest).orientation();
		double d2 = (world.field().enemy_goal() - dest).orientation();
		const double score_ball = angle_diff(d1, d2);

		return weight_goal * score_goal - weight_ball * score_ball;
	}

	bool calc_position_best(const World &world, const Point &passee_pos, const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos, bool pass = false) {
		// divide up into a hexagonal grid
		const double x1 = -world.field().length() / 2, x2 = -x1;
		const double y1 = -world.field().width() / 2, y2 = -y1;

		// for the spacing to be uniform, we need dy = sqrt(3/4)*dx
		const double dx = (x2 - x1) / (grid_x + 1) / 2;
		const double dy = (y2 - y1) / (grid_y + 1) / 2;
		double best_score = -1e50;

		best_pos = Point();

		for (int i = 1; i <= 2*grid_y+1; i += 2) {
			for (int j = i%2+1; j <= 2*grid_x+1; j += 2) {
				const double x = x1 + dx * j;
				const double y = y1 + dy * i;
				const Point pos = Point(x, y);

				// TEMPORARY HACK!!
				// ensures that we do not get too close to the enemy defense area.
				const double goal_dist = (pos - world.field().enemy_goal()).len();
				if (goal_dist < world.field().goal_width()) {
					continue;
				}

				double score = scoring_function(world, passee_pos, enemy_pos, pos, dont_block, pass);
				
				if (score > best_score) {
					best_score = score;
					best_pos = pos;
				}
			}
		}
		return best_score > 0;
	}
}

double AI::HL::STP::Evaluation::offense_score(const World &world, const Point dest) {
	const EnemyTeam &enemy = world.enemy_team();

	std::vector<Point> enemy_pos;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());

	return scoring_function(world, Point(), enemy_pos, dest, dont_block);
}

std::array<Point, 2> AI::HL::STP::Evaluation::offense_positions(const World &world) {
	// just for caching..
	const EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	// TODO: optimize using the matrix below
	// std::vector<std::vector<bool>> grid(GRID_X, std::vector<bool>(GRID_Y, true));

	// don't block ball, and the others
	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());
	/*
	   const FriendlyTeam &friendly = world.friendly_team();
	   for (size_t i = 0; i < friendly.size(); ++i) {
	   if (players.find(friendly.get(i)) == players.end()) {
	   dont_block.push_back(friendly.get(i)->position());
	   }
	   }
	 */

	std::array<Point, 2> best;

	calc_position_best(world, Point(), enemy_pos, dont_block, best[0]);

	dont_block.push_back(best[0]);
	calc_position_best(world, Point(), enemy_pos, dont_block, best[1]);

	return best;
}

Point AI::HL::STP::Evaluation::passee_position(const World &world) {

	const EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());

	Point best;
	calc_position_best(world, Point(), enemy_pos, dont_block, best);

	return best;
}

Point AI::HL::STP::Evaluation::passer_position(const World &world, Point passee_pos) {
	const EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}
	
	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());
	
	Point best;
	calc_position_best(world, passee_pos, enemy_pos, dont_block, best, true);

	return best;
}

