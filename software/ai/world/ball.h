#ifndef AI_WORLD_BALL_H
#define AI_WORLD_BALL_H

#include "ai/world/predictable.h"
#include "geom/point.h"
#include "proto/messages_robocup_ssl_detection.pb.h"
#include "uicomponents/visualizer.h"
#include "util/memory.h"
#include <cstdlib>
#include <glibmm.h>

class World;

/**
 * The ball.
 */
class Ball : public Visualizable::Ball, public Predictable {
	public:
		/**
		 * The approximate radius of the ball.
		 */
		static const double RADIUS;

		/**
		 * \return the position of the robot.
		 */
		Point position() const {
			return Predictable::position();
		}

	private:
		double sign;

		/**
		 * Constructs a new Ball object.
		 *
		 * \return the new object.
		 */
		static RefPtr<Ball> create();

		/**
		 * Constructs a new Ball object.
		 */
		Ball();

		/**
		 * Updates the position of the ball using new data.
		 *
		 * \param[in] pos the new position of the ball, in unswapped field
		 * coordinates.
		 */
		void update(const Point &pos);

		bool visualizer_can_drag() const {
			return false;
		}

		void visualizer_drag(const Point &) {
			std::abort();
		}

		Point velocity() const {
			return est_velocity();
		}

		friend class World;
};

#endif

