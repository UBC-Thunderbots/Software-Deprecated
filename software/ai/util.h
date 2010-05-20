#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"
#include "ai/world/team.h"

#include <vector>

namespace ai_util {

	/**
	 * A comparator that sorts by values in a vector
	 */
	template<typename T> class cmp_table {
		public:
			cmp_table(const std::vector<T>& tbl) : tbl(tbl) {
			}
			bool operator()(unsigned int x, unsigned int y) {
				return tbl[x] > tbl[y];
			}
		private:
			const std::vector<T>& tbl;
	};

	/**
	 * A comparator that sorts by a particular distance.
	 * To be used together with std::sort.
	 * E.g.
	 * std::vector<robot::ptr> enemies = ai_util::get_robots(enemy);
	 * std::sort(enemies.begin(), enemies.end(), ai_util::cmp_dist<robot::ptr>(goal));
	 */
	template<typename T> class cmp_dist {
		public:
			cmp_dist(const point& dest) : dest(dest) {
			}
			bool operator()(T x, T y) const {
				return (x->position() - dest).lensq() < (y->position() - dest).lensq();
			}
		private:
			const point& dest;
	};

	/**
	 * Orientation epsilon.
	 * Generally higher than position epsilon.
	 */
	extern const double ORI_CLOSE;

	/**
	 * Position epsilon.
	 * Should be set to the accuracy of the image recognizition data.
	 */
	extern const double POS_CLOSE;

	/**
	 * Number of points to consider when shooting at the goal.
	 */
	extern const unsigned int SHOOTING_SAMPLE_POINTS;

	/**
	 * Gets the orientation of a point.
	 */
	double orientation(const point& p);

	/**
	 * Gets the absolute angle difference.
	 * Guaranteed to be between 0 and PI.
	 */
	double angle_diff(const double& a, const double& b);

	/**
	 * Checks if the path from begin to end is blocked by one team, with some threshold.
	 * Returns true if path is okay.
	 */
	bool path_check(const point& begin, const point& end, const team& theteam, const double& thresh);

	/**
	 * Checks if the path from begin to end is blocked by one team, with some threshold.
	 * Also skips one particular robot.
	 * Returns true if path is okay.
	 */
	bool path_check(const point& begin, const point& end, const team& theteam, const double& thresh, const robot::ptr skip);

	/**
	 * Checks if the passee can get the ball now.
	 * Returns false if some robots is blocking line of sight of ball from passee
	 * Returns false if passee is not facing the ball.
	 * Returns false if some condition is invalid.
	 */
	bool can_pass(const world::ptr w, const player::ptr passee);

	/**
	 * Calculates the candidates to aim for when shooting at the goal.
	 */
	const std::vector<point> calc_candidates(const world::ptr w);

	/**
	 * Returns an integer i, where candidates[i] is the best point to aim for when shooting.
	 * Here candidates is the vector returned by calc_candidates.
	 * If all shots are bad, candidates.size() is returned.
	 */
	size_t calc_best_shot(const player::ptr player, const world::ptr w);

	/**
	 * Clips a point to a rectangle boundary.
	 */
	point clip_point(const point& p, const point& bound1, const point& bound2);

	/**
	 * Convert team into vector of robots. 
	 * For enemy team.
	 */
	std::vector<robot::ptr> get_robots(const team& theteam);

	/**
	 * Convert friendly into vector of players. 
	 * For friendly team.
	 */
	std::vector<player::ptr> get_players(const friendly_team& friendly);

	/**
	 * Matches points of two different vectors.
	 * Returns ordering of the matching such that the total distance is minimized.
	 * If order is the returned vector.
	 * Then the i element of v1 is matched with order[i] element of v2.
	 * Currently uses a slow brute-force algorithm.
	 */
	std::vector<size_t> dist_matching(const std::vector<point>& v1, const std::vector<point>& v2);

}

#endif

