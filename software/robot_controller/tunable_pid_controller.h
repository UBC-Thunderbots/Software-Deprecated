#ifndef ROBOT_CONTROLLER_PID_CONTROLLER_H
#define ROBOT_CONTROLLER_PID_CONTROLLER_H

#include "robot_controller/robot_controller.h"
#include "robot_controller/tunable_controller.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "world/player_impl.h"

#include <vector>
#include <glibmm.h>

class tunable_pid_controller : public robot_controller, public tunable_controller {
	public:

		void move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity);

		robot_controller_factory &get_factory() const;

		tunable_pid_controller(player_impl::ptr plr);

		const std::vector<double>& get_params_min() const {
			return param_min;
		}

		const std::vector<double>& get_params_max() const {
			return param_max;
		}

		void set_params(const std::vector<double>& params);

		const std::vector<double>& get_params() const;

	private:
		player_impl::ptr plr;

	protected:

		static const std::vector<double> param_min;
		static const std::vector<double> param_max;
		static const std::vector<double> param_default;

		bool initialized;

		std::vector<double> param;

		// errors in x, y, d
		std::vector<point> error_pos;
		std::vector<double> error_ori;
};

#endif
