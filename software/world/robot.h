#ifndef WORLD_ROBOT_H
#define WORLD_ROBOT_H

#include "geom/angle.h"
#include "world/robot.h"

//
// A robot can be either friendly or enemy. Vectors in this class are in team
// coordinates.
//
class robot : public draggable {
	public:
		//
		// A pointer to a robot object.
		//
		typedef Glib::RefPtr<robot> ptr;

		//
		// The position of the robot at the last camera frame.
		//
		point position() const __attribute__((warn_unused_result)) {
			return impl->position() * (flip ? -1.0 : 1.0);
		}

		//
		// The estimated velocity of the robot at the last camera frame.
		//
		point est_velocity() const __attribute__((warn_unused_result)) {
			return impl->est_velocity() * (flip ? -1.0 : 1.0);
		}

		//
		// The orientation of the robot in radians at the last camera frame.
		//
		double orientation() const __attribute__((warn_unused_result)) {
			return angle_mod(impl->orientation() + (flip ? PI : 0.0));
		}

		//
		// Allows the UI to set the position of the robot.
		//
		void ext_drag(const point &p, const point &v) {
			impl->ext_drag(p * (flip ? -1.0 : 1.0), v * (flip ? -1.0 : 1.0));
		}

		//
		// Constructs a new robot object.
		//
		// Parameters:
		//  impl
		//   the implementation object that provides global coordinates
		//
		//  flip
		//   whether the X and Y coordinates are reversed for this object
		//
		robot(robot_impl::ptr impl, bool flip) : impl(impl), flip(flip) {
		}

		//
		// The maximum possible radius of the robot.
		//
		static const double MAX_RADIUS = 0.09;

	private:
		robot_impl::ptr impl;
		const bool flip;
};

#endif

