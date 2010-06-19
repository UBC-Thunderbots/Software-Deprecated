#include "ai/ball_filter/ball_filter.h"
#include "ai/world/team.h"
#include "geom/point.h"
#include "util/timestep.h"
#include <memory>
#include <utility>
#include <vector>

namespace {
	class circle {
		public:
			point centre;
			double certainty;

			circle(const point &centre, double certainty) : centre(centre), certainty(certainty) {
			}
	};

	bool operator==(const circle &c1, const circle &c2) {
		return (c1.centre - c2.centre).len() < 0.001;
	}

	bool sort_by_certainty(const circle &c1, const circle &c2) {
		if (c1.certainty != c2.certainty) {
			return c1.certainty < c2.certainty;
		} else {
			return c1.centre < c2.centre;
		}
	}

	class defensive_filter : public ball_filter {
		public:
			defensive_filter() : ball_filter("Defensive Filter") {
				circles.push_back(circle(point(), DELETE_THRESHOLD));
			}

			point filter(const std::vector<std::pair<double, point> > &obs, friendly_team &, enemy_team &) {
			
	
			  // Just use the maximum-confidence ball.
			  //				const std::vector<std::pair<double, point> >::const_iterator &best_obs(std::max_element(obs.begin(), obs.end()));

				//this code is for the purose of only looking at balls in the offense area
			  int best_pos = obs.size();
				double confidence = 0.0;

				for(unsigned int i =0; i<obs.size(); i++){
				 if(obs[i].second.x<0.0){
				   if(confidence < obs[i].first){
				     best_pos = i;
				   }
				  }
				 }
				const std::vector<std::pair<double, point> >::const_iterator &best_obs(obs.begin()+best_pos);

				if (best_obs != obs.end()) {
					// All the circles that contain the ball should be combined
					// into one single circle having the multiplicative
					// combination of the certainties of the original circles,
					// and having a centre point at the ball. If no circles
					// contain the ball, we will instead create a new circle
					// containing the ball with a default level of certainty.

					// Compute the new certainty and delete the existing
					// circles.
					double recip_certainty = 1.0;
					for (std::list<circle>::iterator i = circles.begin(); i != circles.end(); ) {
						if ((best_obs->second - i->centre).len() < RADIUS) {
							recip_certainty *= 1.0 - i->certainty;
							i = circles.erase(i);
						} else {
							++i;
						}
					}

					// Decay the other circles, deleting any that fall below the
					// threshold.
					decay(true);

					// Create the new circle.
					circles.push_back(circle(best_obs->second, 1.0 - recip_certainty * (1.0 - DECAY_RATE)));
				} else {
					// Decay the circles, but keep them around even if below the
					// threshold so we don't end up with no circles at all.
					// They'll all be deleted next time we get a detection.
					decay(false);
				}

				// Use the circle with the highest certainty.
				return std::max_element(circles.begin(), circles.end(), &sort_by_certainty)->centre;
			}

		private:
			static const double RADIUS = 10.0 / TIMESTEPS_PER_SECOND;
			static const double DECAY_RATE = 0.2063; // half-life = 3 frames
			static const double DELETE_THRESHOLD = 0.02; // stores < 50 circles
			std::list<circle> circles;
			point last_point;

			void decay(bool delete_if_below_threshold) {
				for (std::list<circle>::iterator i = circles.begin(); i != circles.end(); ) {
					i->certainty *= 1.0 - DECAY_RATE;
					if (delete_if_below_threshold && i->certainty < DELETE_THRESHOLD) {
						i = circles.erase(i);
					} else {
						++i;
					}
				}
			}
	};

	defensive_filter instance;
}

