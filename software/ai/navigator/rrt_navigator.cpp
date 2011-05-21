#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "ai/navigator/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "util/param.h"
#include <iostream>

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace AI {
	namespace Nav {
		namespace RRT {
			// fraction of the maximum speed that the robot will try to dribble at
			const double DRIBBLE_SPEED = 1.0;
			const double THRESHOLD = 0.08;
			const double STEP_DISTANCE = 0.1;
			// probability that we will take a step towards the goal
			const double GOAL_PROB = 0.2;
			const double WAYPOINT_PROB = 0.5;
			const double RAND_PROB = 1.0 - GOAL_PROB - WAYPOINT_PROB;
			// number of iterations to go through for each robot until we give up and
			// just return the best partial path we've found
			const int ITERATION_LIMIT = 200;
			const int NUM_WAYPOINTS = 50;

			DoubleParam offset_angle("Pivot: offset angle (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);
			DoubleParam offset_distance("Pivot: offset distance", "Nav/RRT", 0.15, -10.0, 10.0);
			DoubleParam orientation_offset("Pivot: orientation offset (degrees)", "Nav/RRT", 30.0, -1000.0, 1000.0);


			class RRTNavigator : public Navigator {
				public:
					NavigatorFactory &factory() const;
					void pivot(Player::Ptr player);
					void grab_ball(Player::Ptr player);
					void tick();
					static Navigator::Ptr create(World &world);

				private:
					RRTNavigator(World &world);
					RRTPlanner planner;
			};

			class RRTNavigatorFactory : public NavigatorFactory {
				public:
					RRTNavigatorFactory();
					Navigator::Ptr create_navigator(World &world) const;
			};

			RRTNavigatorFactory factory_instance;

			NavigatorFactory &RRTNavigator::factory() const {
				return factory_instance;
			}

			void RRTNavigator::grab_ball(Player::Ptr player) {
				const double ux = world.ball().velocity().len(); // velocity of ball

#warning MAGIC NUMBER
				const double v = 1.5;

				const Point p1 = world.ball().position();
				const Point p2 = player->position();
				const Point u = world.ball().velocity().norm();

				const double x = (p2 - p1).dot(u);
				const double y = std::fabs((p2 - p1).cross(u));

				const Point p = p1 + u * x;

				double a = 1 + (y * y) / (x * x);
				double b = (2 * y * y * ux) / (x * x);
				double c = (y * y * ux * ux) / (x * x) - v;

				double vx1 = (-b + std::sqrt(b * b - (4 * a * c))) / (2 * a);
				double vx2 = (-b - std::sqrt(b * b - (4 * a * c))) / (2 * a);

				double t1 = x / (vx1 + ux);
				double t2 = x / (vx2 + ux);

				double t = std::min(t1, t2);
				if (t < 0) {
					t = std::max(t1, t2);
				}

				if (std::isnan(t) || std::isinf(t) || t < 0) {
					Player::Path path;
					double dest_ori = (world.ball().position() - player->position()).orientation();
					path.push_back(std::make_pair(std::make_pair(world.ball().position(), dest_ori), world.monotonic_time()));
					player->path(path);
					return;
				}

				Point dest_pos = p1 + world.ball().velocity() * t;

				Player::Path path;
				double dest_ori = (world.ball().position() - player->position()).orientation();
				path.push_back(std::make_pair(std::make_pair(dest_pos, dest_ori), world.monotonic_time()));
				player->path(path);
			}

			void RRTNavigator::pivot(Player::Ptr player) {
				Player::Path path;
				Point dest;
				double dest_orientation;

				// try to pivot around the ball to catch it
				Point current_position = player->position();
				double to_ball_orientation = (world.ball().position() - current_position).orientation();
				double orientation_temp = degrees2radians(orientation_offset);

				double angle = offset_angle;
				if (angle_mod(to_ball_orientation - player->destination().second) > 0) {
					angle = -angle;
					orientation_temp = -orientation_temp;
				}

				angle = degrees2radians(angle);

				Point diff = (world.ball().position() - current_position).rotate(angle);

				dest = world.ball().position() - offset_distance * (diff / diff.len());
				if (dest.len() > 0.5) {
					orientation_temp = 0;
				}
				dest_orientation = (world.ball().position() - current_position).orientation() + orientation_temp;

				path.push_back(std::make_pair(std::make_pair(dest, dest_orientation), world.monotonic_time()));
				player->path(path);
			}

			void RRTNavigator::tick() {
				timespec working_time;
				Player::Path path;
				std::vector<Point> path_points;

				for (std::size_t i = 0; i < world.friendly_team().size(); ++i) {
					path.clear();
					Player::Ptr player = world.friendly_team().get(i);
					Point dest;
					double dest_orientation;
					if (player->type() == AI::Flags::MoveType::CATCH) {
						grab_ball(player);
						continue;
					} else if (player->type() == AI::Flags::MoveType::PIVOT) {
						pivot(player);
						continue;
					} else if (valid_path(player->position(), player->destination().first, world, player)) {
						// if we're not trying to catch the ball and there are no obstacles in our way then go
						// to the exact location, skipping all of the tree creation
						path.push_back(std::make_pair(player->destination(), world.monotonic_time()));
						player->path(path);
						continue;
					} else {
						dest = player->destination().first;
					}

					// calculate a path
					path_points.clear();
					path_points = planner.plan(player, dest);

					double dist = 0.0;
					working_time = world.monotonic_time();

					dest_orientation = player->destination().second;
					for (std::size_t j = 0; j < path_points.size(); ++j) {
						// the last point will just use whatever the last orientation was
						if (j + 1 != path_points.size()) {
							dest_orientation = (path_points[j + 1] - path_points[j]).orientation();
						}

						// get distance between last two points
						if (j == 0) {
							dist = (player->position() - path_points[0]).len();
						} else {
							dist = (path_points[j] - path_points[j - 1]).len();
						}

						// dribble at a different speed
						if (player->type() == AI::Flags::MoveType::DRIBBLE) {
							timespec time_to_add = double_to_timespec(dist / player->MAX_LINEAR_VELOCITY / DRIBBLE_SPEED);
							timespec_add(working_time, time_to_add, working_time);
						}

						path.push_back(std::make_pair(std::make_pair(path_points[j], dest_orientation), working_time));
					}

					// just use the current player position as the destination if we are within the
					// threshold already
					if (path_points.size() == 0) {
						path.push_back(std::make_pair(std::make_pair(player->position(), dest_orientation), working_time));
					}

					player->path(path);
				}
			}

			Navigator::Ptr RRTNavigator::create(World &world) {
				const Navigator::Ptr p(new RRTNavigator(world));
				return p;
			}

			RRTNavigator::RRTNavigator(AI::Nav::W::World &world) : Navigator(world), planner(world) {
			}

			RRTNavigatorFactory::RRTNavigatorFactory() : NavigatorFactory("RRT Navigator") {
			}

			Navigator::Ptr RRTNavigatorFactory::create_navigator(World &world) const {
				return RRTNavigator::create(world);
			}
		}
	}
}

