#include "ai/ball_filter/ball_filter.h"
#include "geom/point.h"
#include "util/timestep.h"
#include "ai/param.h"
#include <memory>
#include <utility>
#include <vector>

using AI::BF::BallFilter;
using namespace AI::BF::W;

namespace {
	DoubleParam decay_rate("circle decay rate", "Ball/Chris", 0.2063, 0.0, 1.0);
	DoubleParam circle_radius("circle radius", "Ball/Chris", 0.67, 0.0, 10.0);

	class Circle {
		public:
			Point centre;
			double certainty;

			Circle(const Point &centre, double certainty) : centre(centre), certainty(certainty) {
			}
	};

	bool operator==(const Circle &c1, const Circle &c2) {
		return (c1.centre - c2.centre).len() < 0.001;
	}

	bool sort_by_certainty(const Circle &c1, const Circle &c2) {
		if (c1.certainty != c2.certainty) {
			return c1.certainty < c2.certainty;
		} else {
			return c1.centre < c2.centre;
		}
	}

	class ChrisFilter : public BallFilter {
		public:
			ChrisFilter() : BallFilter("Chris's Filter") {
				circles.push_back(Circle(Point(), DELETE_THRESHOLD));
			}

			Point filter(const std::vector<std::pair<double, Point> > &obs, World &) {
				// Just use the maximum-confidence ball.
				const std::vector<std::pair<double, Point> >::const_iterator &best_obs(std::max_element(obs.begin(), obs.end()));

				if (best_obs != obs.end()) {
					// All the circles that contain the ball should be combined into one single circle.
					// It should have the multiplicative combination of the certainties of the original circles, and a centre point at the ball.
					// If no circles contain the ball, we will instead create a new circle containing the ball with a default level of certainty.

					// Compute the new certainty and delete the existing circles.
					double recip_certainty = 1.0 - decay_rate;
					for (std::list<Circle>::iterator i = circles.begin(); i != circles.end(); ) {
						if ((best_obs->second - i->centre).len() < circle_radius) {
							recip_certainty *= 1.0 - i->certainty;
							i = circles.erase(i);
						} else {
							++i;
						}
					}

					// Decay the other circles, deleting any that fall below the threshold.
					decay(true);

					// Create the new circle.
					circles.push_back(Circle(best_obs->second, 1.0 - recip_certainty));
				} else {
					// Decay the circles, but keep them around even if below the threshold so we don't end up with no circles at all.
					// They'll all be deleted next time we get a detection.
					decay(false);
				}

				// Use the circle with the highest certainty.
				return std::max_element(circles.begin(), circles.end(), &sort_by_certainty)->centre;
			}

		private:
			static const double DELETE_THRESHOLD; // stores < 50 circles
			std::list<Circle> circles;
			Point last_point;

			void decay(bool delete_if_below_threshold) {
				for (std::list<Circle>::iterator i = circles.begin(); i != circles.end(); ) {
					i->certainty *= 1.0 - decay_rate;
					if (delete_if_below_threshold && i->certainty < DELETE_THRESHOLD) {
						i = circles.erase(i);
					} else {
						++i;
					}
				}
			}
	};

	const double ChrisFilter::DELETE_THRESHOLD = 0.02; // stores < 50 circles

	ChrisFilter instance;
}

