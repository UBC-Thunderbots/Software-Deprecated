#ifndef ROBOT_CONTROLLER_LINEAR_CONTROLLER_H
#define ROBOT_CONTROLLER_LINEAR_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "world/player_impl.h"

class linear_controller : public virtual robot_controller {
	public:

		void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);

		robot_controller_factory &get_factory() const;

		linear_controller(player_impl::ptr plr);

	protected:
	private:
		player_impl::ptr plr;
};

#endif

