#ifndef ROBOT_CONTROLLER_LINEAR_CONTROLLER_H
#define ROBOT_CONTROLLER_LINEAR_CONTROLLER_H

#include <map>
#include <glibmm.h>
#include "ai/world/player.h"
#include "robot_controller/robot_controller.h"
#include "geom/point.h"
#include "util/memory.h"
#include "util/noncopyable.h"

class LinearController : public RobotController {
	public:

		void move(const Point &new_position, double new_orientation, Point &linear_velocity, double &angular_velocity);

		void clear();

		RobotControllerFactory &get_factory() const;

		LinearController(RefPtr<Player> plr);

	protected:
	private:
		RefPtr<Player> plr;
};

#endif

