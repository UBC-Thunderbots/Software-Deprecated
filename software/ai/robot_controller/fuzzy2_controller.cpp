#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include <cmath>
#include <glibmm.h>
#include <map>
#include "uicomponents/param.h"

using AI::RC::RobotController;
using AI::RC::TunableController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	const int P = 5;
	
	DoubleParam FUZZY2_MAX_ACC("Fuzzy2: max acc", 3, 0.0, 20.0);

	const double arr_min[P] = { 6.0, 0.0, 0.0, 6.0, 6.0 };
	const double arr_max[P] = { 13.0, 2.0, 2.0, 13.0, 13.0 };

	// default parameters:
	const double arr_def[P] = {9.18749, 0.575552, 0.695691, 9.50912, 7.91213};
 
	const std::vector<double> param_min(arr_min, arr_min + P);
	const std::vector<double> param_max(arr_max, arr_max + P);
	const std::vector<double> param_default(arr_def, arr_def + P);

	class Fuzzy2Controller : public RobotController, public TunableController {
		public:
			Fuzzy2Controller(World &world, Player::Ptr player) : RobotController(world, player), param(param_default) {
			}

			void tick() {
				const Player::Path &path = player->path();
				if (path.empty()) {
					return;
				}

				Point new_position = path[0].first.first;
				double new_orientation = path[0].first.second;

				const Point &current_position = player->position();
				const double current_orientation = player->orientation();
				double angular_velocity = param[4] * angle_mod(new_orientation - current_orientation);

				double distance_factor = (new_position - current_position).len() / param[1];
				if (distance_factor > 1) {
					distance_factor = 1;
				}

				Point linear_velocity = (new_position - current_position).rotate(-current_orientation);

				if (linear_velocity.len() != 0) {
					linear_velocity = linear_velocity / linear_velocity.len() * distance_factor * param[0];
				}

				Point stopping_velocity = (-player->velocity()).rotate(-current_orientation);
				if (stopping_velocity.len() != 0) {
					stopping_velocity = stopping_velocity / stopping_velocity.len() * param[0];
				}

				double velocity_factor = ((player->velocity()).len() / param[0]) * param[2];
				if (velocity_factor > 1) {
					velocity_factor = 1;
				}

				distance_factor = (new_position - current_position).len() / param[3];
				if (distance_factor > 1) {
					distance_factor = 1;
				}

				linear_velocity = distance_factor * linear_velocity + (1 - distance_factor) * (velocity_factor * stopping_velocity + (1 - velocity_factor) * linear_velocity);


				struct timespec currentTime, finalTime;
				currentTime = world.monotonic_time();
				timespec_sub(path[0].second, currentTime, finalTime);
				double desired_velocity = (path[0].first.first - player->position()).len() / (static_cast<double>(timespec_to_millis(finalTime)) / 1000);
				if (linear_velocity.len() > desired_velocity && desired_velocity > 0) {
					LOG_INFO("Warning: Fuzzy controller is being told to travel slow.");
					linear_velocity = desired_velocity * (linear_velocity / linear_velocity.len());
				}


				// threshold the linear acceleration
				Point accel = linear_velocity - prev_linear_velocity;
				if (accel.len() > FUZZY2_MAX_ACC) {
					accel *= FUZZY2_MAX_ACC / accel.len();
					linear_velocity = prev_linear_velocity + accel;
				}
		
				int wheel_speeds[4] = { 0, 0, 0, 0 };

				convert_to_wheels(linear_velocity, angular_velocity, wheel_speeds);

				player->drive(wheel_speeds);

				prev_linear_velocity = linear_velocity;
			}

			void set_params(const std::vector<double> &params) {
				this->param = params;
			}

			const std::vector<double> get_params() const {
				return param;
			}

			const std::vector<double> get_params_default() const {
				return param_default;
			}

			const std::vector<double> get_params_min() const {
				return param_min;
			}

			const std::vector<double> get_params_max() const {
				return param_max;
			}

		protected:
			std::vector<double> param;
			Point prev_linear_velocity;
	};

	class Fuzzy2ControllerFactory : public RobotControllerFactory {
		public:
			Fuzzy2ControllerFactory() : RobotControllerFactory("Fuzzy Version 2") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr player) const {
				RobotController::Ptr p(new Fuzzy2Controller(world, player));
				return p;
			}
	};

	Fuzzy2ControllerFactory factory;
}

