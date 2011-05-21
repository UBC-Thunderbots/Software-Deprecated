#ifndef AI_HL_STP_EVALUATION_CM_EVALUATION_H
#define AI_HL_STP_EVALUATION_CM_EVALUATION_H

#include "ai/hl/stp/cm_coordinate.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/region.h"
#include "ai/hl/stp/world.h"
#include "util/timestep.h"
#include <functional>
#include <vector>

#define MAX_TEAM_ROBOTS 5

// this needs to be remeasured
#define LATENCY_DELAY 0.100

// ==== Obstacle Flags ================================================//

// Standard Obstacles
#define OBS_BALL (1U << 0)
#define OBS_WALLS (1U << 1)
#define OBS_THEIR_DZONE (1U << 2)
#define OBS_OUR_DZONE (1U << 3)
#define OBS_TEAMMATE(id) (1U << (4 + (id)))
#define OBS_OPPONENT(id) (1U << (4 + MAX_TEAM_ROBOTS + (id)))
#define OBS_TEAMMATES (((1U << MAX_TEAM_ROBOTS) - 1) << 4)
#define OBS_OPPONENTS (((1U << MAX_TEAM_ROBOTS) - 1) << 4 + MAX_TEAM_ROBOTS)

#define OBS_EVERYTHING (~0U)
#define OBS_EVERYTHING_BUT_ME(id) (OBS_EVERYTHING & (~(OBS_TEAMMATE(id))))
#define OBS_EVERYTHING_BUT_US (OBS_EVERYTHING & (~(OBS_TEAMMATES)))
#define OBS_EVERYTHING_BUT_BALL (OBS_EVERYTHING & (~(OBS_BALL)))

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * HL World evaluation ported from CMDragon world.cc
				 */

				/**
				 * Checks if ball is on the sides.
				 */
				int side_ball(const World &world);

				/**
				 * Checks if the opponents are generally positioned on the sides.
				 */
				int side_strong(const World &world);

				/**
				 * Checks if the ball or the opponents are generally positioned on the sides.
				 */
				int side_ball_or_strong(const World &world);

				/**
				 * Finds the nearest teammate to a point on the field.
				 */
				int nearest_teammate(const World &world, Point p, double time);

				/**
				 * Finds the nearest opponent to a point on the field.
				 */
				int nearest_opponent(const World &world, Point p, double time);

				/**
				 * Obs methods return an obs_flag set to why a position or other
				 * shape is not open. Or zero if the position or shape is open
				 */
				unsigned int obs_position(const World &world, Point p, unsigned int obs_flags, double pradius, double time = -1);

				unsigned int obs_line(const World &world, Point p1, Point p2, unsigned int obs_flags, double pradius, double time);

				unsigned int obs_line_first(const World &world, Point p1, Point p2, unsigned int obs_flags, Point &first, double pradius, double time = -1);

				/**
				 * returns number of obstacles on the line
				 */
				unsigned int obs_line_num(const World &world, Point p1, Point p2, unsigned int obs_flags, double pradius, double time = -1);

				/**
				 * returns true if point p will block a shot at time
				 */
				bool obs_blocks_shot(const World &world, Point p, double time);

				/**
				 * Evaluation functions ported from CMDragon evaluation.cc
				 */
				namespace CMEvaluation {
					/**
					 * aim()
					 *
					 * These functions take a target point and two relative vectors
					 * denoting the range to aim along. They take an obs_flags of
					 * obstacles to avoid in aiming and then return the the point along
					 * the vectors with the largest clear angle.
					 *
					 * The pref_target_point provides a preferred direction along with a
					 * bias. If no other direction is clear by more than the bias it
					 * will simply return the point along the largest open angle near
					 * the preference. Used for hysteresis.
					 *
					 * aim() should be guaranteed not to return false if obs_flags is 0.
					 *
					 */
					bool aim(const World &world, double time, Point target, Point r2, Point r1, unsigned int obs_flags, Point pref_target_point, double pref_amount, Point &target_point, double &target_tolerance);

					/**
					 * aim() but with pref_target_point set to center of the two aiming vectors and obs_flags set to 0
					 */
					bool aim(const World &world, double time, Point target, Point r2, Point r1, unsigned int obs_flags, Point &target_point, double &target_tolerance);

					/**
					 * defend_line()
					 * defend_point()
					 * defend_on_line()
					 *
					 * This returns the position and velocity to use to defend a
					 * particular line (or point) on the field. It combines the
					 * positions of the best static defense with the interception point
					 * using the variance on the interception point from the Kalman
					 * filter. It also computes the velocity to hit that point with.
					 *
					 * Setting obs_flags and optionally pref_point and pref_amount can
					 * be used to take account for other robots also defending. When
					 * computing a static position it will use aim() with the provided
					 * paramters to find the largest remaining open angle and statically
					 * defend this range.
					 *
					 * The defend_*_static() and defend_*_trajectory() methods are
					 * helper functions that defend_*() uses.
					 *
					 * defend_on_line() positions itself along a line segment nearest to
					 * the ball. The intercept flag here still works as biasing the
					 * position towards where the ball will cross the segment.
					 *
					 * The intercept field specifies whether a moving ball should be
					 * intercepted. If true after the call it means the robot is actively
					 * trying to intercept the ball.
					 *
					 */
					bool defend_line(const World &world, double time, Point g1, Point g2, double distmin, double distmax, double dist_off_ball, bool &intercept, unsigned int obs_flags, Point pref_point, double pref_amount, Point &target, Point &velocity);

					bool defend_line(const World &world, double time, Point g1, Point g2, double distmin, double distmax, double dist_off_ball, bool &intercept, Point &target, Point &velocity);

					bool defend_point(const World &world, double time, Point point, double distmin, double distmax, double dist_off_ball, bool &intercept, Point &target, Point &velocity);

					bool defend_on_line(const World &world, double time, Point p1, Point p2, bool &intercept, Point &target, Point &velocity);

					/**
					 * finds the furthest point of a robot in a direction
					 */
					Point farthest(const World &world, double time, unsigned int obs_flags, Point bbox_min, Point bbox_max, Point dir);

					/**
					 * finds an open position
					 */
					Point find_open_position(const World &world, Point p, Point toward, unsigned int obs_flags, double pradius = Robot::MAX_RADIUS);

					/**
					 * finds an open position and yield
					 */
					Point find_open_position_and_yield(const World &world, Point p, Point toward, unsigned int obs_flags);
				};

				class CMEvaluationPosition {
					public:
						typedef std::function<double (const World &, Point, unsigned int, double &)> EvalFn;

						TRegion region;

						CMEvaluationPosition();

						CMEvaluationPosition(const TRegion &region, const EvalFn &eval, double pref_amount = 0, unsigned int n_points = 10);

						void set(const TRegion &region_, const EvalFn &eval_, double pref_amount_ = 0, unsigned int n_points_ = 10);

						void update(const World &world, unsigned int obs_flags_);

						void addPoint(Point p);

						Point point() const;

						double angle() const;

					private:
						// Function
						EvalFn eval;

						unsigned int obs_flags;

						double last_updated;

						// Evaluated Points
						unsigned int n_points;
						std::vector<Point> points;
						std::vector<double> angles;
						std::vector<double> weights;

						std::vector<Point> new_points;

						int best;
						double pref_amount;

						Point pointFromDistribution(const World &w);
				};
			}
		}
	}
}

inline bool AI::HL::STP::Evaluation::CMEvaluation::aim(const World &world, double time, Point target, Point r2, Point r1, unsigned int obs_flags, Point &target_point, double &target_tolerance) {
	return aim(world, time, target, r2, r1, obs_flags, ((r2 + r1) / 2.0), 0.0, target_point, target_tolerance);
}

inline bool AI::HL::STP::Evaluation::CMEvaluation::defend_line(const World &world, double time, Point g1, Point g2, double distmin, double distmax, double dist_off_ball, bool &intercept, Point &target, Point &velocity) {
	return defend_line(world, time, g1, g2, distmin, distmax, dist_off_ball, intercept, 0, Point(), 0.0, target, velocity);
}

inline AI::HL::STP::Evaluation::CMEvaluationPosition::CMEvaluationPosition() : obs_flags(0), last_updated(0), n_points(0), best(-1), pref_amount(0) {
}

inline AI::HL::STP::Evaluation::CMEvaluationPosition::CMEvaluationPosition(const TRegion &region, const EvalFn &eval, double pref_amount, unsigned int n_points) : region(region), eval(eval), obs_flags(0), last_updated(0), n_points(n_points), best(-1), pref_amount(pref_amount) {
}

inline void AI::HL::STP::Evaluation::CMEvaluationPosition::set(const TRegion &region_, const EvalFn &eval_, double pref_amount_, unsigned int n_points_) {
	region = region_;
	eval = eval_;
	n_points = n_points_;
	pref_amount = pref_amount_;

	obs_flags = 0;

	points.clear();
	weights.clear();
	best = -1;
	last_updated = 0;
}

inline void AI::HL::STP::Evaluation::CMEvaluationPosition::addPoint(Point p) {
	for (unsigned int i = 0; i < new_points.size(); ++i) {
		if (new_points[i].x == p.x && new_points[i].y == p.y) {
			return;
		}
	}
	new_points.push_back(p);
}

inline Point AI::HL::STP::Evaluation::CMEvaluationPosition::point() const {
	return points[best];
}

inline double AI::HL::STP::Evaluation::CMEvaluationPosition::angle() const {
	return angles[best];
}

inline Point AI::HL::STP::Evaluation::CMEvaluationPosition::pointFromDistribution(const World &w) {
	return region.sample(w);
}

#endif

