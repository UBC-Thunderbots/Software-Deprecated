#include "ai/ball_filter/ball_filter.h"
#include "geom/angle.h"
#include "util/timestep.h"
#include <cmath>
#include <list>
#include <utility>
#include <vector>

using AI::BF::BallFilter;
using namespace AI::BF::W;

namespace {
	struct Circle {
		Point center;
		double certainty;

		bool operator==(Circle other) {
			return (center - other.center).len() < 0.001;
		}
	};

	class CircleFilter : public BallFilter {
		private:
			static const double RADIUS = 10.0/TIMESTEPS_PER_SECOND;
			static const double DECAY_RATE = 0.2063; // half-life = 3 frames
			static const double DEFAULT_CERT = 0.04; // one half-life to delete
			static const double DELETE_THRESHOLD = 0.02; // stores < 50 circles
			std::list<Circle> circles;
			Point last_point;
			bool use_closest;
			unsigned int robot_index;
			unsigned int has_ball_timesteps;

		public:
			CircleFilter() : BallFilter("Circle Filter") {
				Circle c;
				c.center = Point(0, 0);
				c.certainty = DELETE_THRESHOLD;
				circles.push_back(c);
				has_ball_timesteps = 0;
			}

                        Point filter(const std::vector<std::pair<double, Point> > &obs, World &world) {
							FriendlyTeam &friendly = world.friendly_team();
							EnemyTeam &enemy = world.enemy_team();
				Point max_point;
				double max_cert = -0.1;
				Point has_ball_point;
				double has_ball_cert = -0.1;
			
				for (unsigned int i = 0; i < friendly.size(); ++i) {
					Player::Ptr player = friendly.get(i);
					if (player->has_ball()) {						
						has_ball_timesteps++;

						Point orient(1,0);
						has_ball_point = player->position() + (Ball::RADIUS + Robot::MAX_RADIUS) * orient.rotate(player->orientation());

						has_ball_cert = 1.0;
						break;
					}
				}				

				if (has_ball_cert <= 0) {
					has_ball_timesteps = 0;
				}

				// There's nothing we can do to add a new obs, so just decay
				if (obs.empty() && !use_closest && has_ball_cert <= 0) {
					for (std::list<Circle>::iterator it = circles.begin(); it != circles.end(); ++it) {
						it->certainty = (1.0 - DECAY_RATE)*it->certainty;
					}
				}
				else {
					// We don't have obs, but the ball was really close to an enemy robot before, so pretend the robot has the ball
					if (obs.empty() && use_closest) {
						Point orient(1,0);
						Robot::Ptr robot;

						for (unsigned int i = 0; i < enemy.size(); ++i) {
							robot = enemy.get(i);

							if (robot->pattern() == robot_index) {
								max_point = robot->position() + (Ball::RADIUS + Robot::MAX_RADIUS) * orient.rotate(robot->orientation());
								max_cert = DEFAULT_CERT;
								break;
							}
						}
					} else {
						for (unsigned int i = 0; i < obs.size(); i++) {
							if (max_cert < obs[i].first) {
								max_point = obs[i].second;
								max_cert = obs[i].first;
							}
						}
					}

					if (has_ball_cert > max_cert) {
						max_point = has_ball_point;
						max_cert = has_ball_cert;
					}

					std::vector<Circle> containing;
					for (std::list<Circle>::iterator it = circles.begin(); it != circles.end(); ++it) {
						if ((max_point - it->center).len() < RADIUS) {
							containing.push_back(*it);
							it->center = max_point;
						}
						else { it->certainty = (1.0 - DECAY_RATE)*it->certainty; }
					}

					if (containing.empty()) {
						if (max_cert < 0)
						      max_point = last_point;
						Circle c;
						c.center = max_point;
						c.certainty = DEFAULT_CERT;
						circles.push_back(c);
					}
					else {
						double anti_cert = 1.0;
						for (std::vector<Circle>::iterator it = containing.begin(); it != containing.end(); ++it) {
							anti_cert *= 1.0 - (*it).certainty;
							if (it != containing.begin()) {
								for (std::list<Circle>::iterator shit = circles.begin(); shit != circles.end(); ++shit) {
									if( (*shit) == (*it) )
										circles.erase(shit);
								}	

							}
						}
						anti_cert *= 1.0 - DECAY_RATE;
						containing[0].certainty = 1.0 - anti_cert;
					}

					for (std::list<Circle>::iterator it = circles.begin(); it != circles.end(); ) {
						if (it->certainty < DELETE_THRESHOLD) it = circles.erase(it);
						else ++it;
					}
				}

				max_cert = 0.0;
				std::list<Circle>::iterator max_point_it = circles.begin();
				for (std::list<Circle>::iterator it = circles.begin(); it != circles.end(); ++it) {
					if (max_cert < it->certainty) {
						max_cert = it->certainty;
						max_point_it = it;
					}
				}
				last_point = max_point_it->center;
				
				Robot::Ptr robot;
				double min_dist = -1;
				Point ball_ref;
				use_closest = false;

				for (unsigned int i = 0; i < friendly.size(); ++i) {
					robot = friendly.get(i);

					ball_ref = last_point - robot->position();
					if (min_dist == -1 || ball_ref.len() < min_dist) {
						min_dist = ball_ref.len();
					}
				}

				bool is_facing_ball;
				for (unsigned int i = 0; i < enemy.size(); ++i) {
					robot = enemy.get(i);

					ball_ref = last_point - robot->position();
					is_facing_ball = angle_diff(ball_ref.orientation(), robot->orientation()) < (M_PI / 4.0);
					if (is_facing_ball && (min_dist == -1 || ball_ref.len() < min_dist)) {
						use_closest = true;
						robot_index = robot->pattern();	
						min_dist = ball_ref.len();			
					}
				}

				use_closest = use_closest && min_dist != -1 && min_dist < Robot::MAX_RADIUS + 1.1 * Ball::RADIUS; // .1 for allowance
				
				return max_point_it->center;
			}
	};

	CircleFilter instance;
}

