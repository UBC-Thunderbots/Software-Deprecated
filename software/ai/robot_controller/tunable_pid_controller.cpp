#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include <cmath>

using AI::RC::RobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

#define LINEAR_XY

namespace {
	class TunablePIDController : public AI::RC::OldRobotController, public AI::RC::TunableController {
		public:
			void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity);

			void clear();

			explicit TunablePIDController(AI::RC::W::World world, AI::RC::W::Player plr);

			void set_params(const std::vector<double> &params) {
				this->param = params;
			}

			const std::vector<std::string> get_params_name() const;

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
			static const std::vector<double> param_min;
			static const std::vector<double> param_max;
			static const std::vector<double> param_default;

			bool initialized;

			std::vector<double> param;

			// errors in x, y, d
			std::vector<Point> error_pos;
			std::vector<Angle> error_ori;

			Point prev_new_pos;
			Angle prev_new_ori;
	};

	// std::ofstream dout("pid.csv");
	// int dnt = 1;

	// full configurations
	// params = 9
	// param := [prop x, diff x, int x, prop y, diff y, int y, prop r, diff r, int r]

	// reduced parameters
	// assume: x and y are linearly related
	// no integral needed
	// params = 5
	// param := [prop x, diff x, y/x ratio, prop r, diff r]

	const double DAMP = 0.5;

	// basic
	const double DEF_X_PROP = 1.15;
	const double MIN_X_PROP = 1;
	const double MAX_X_PROP = 1.2;

	const double DEF_X_DIFF = 0;
	const double MIN_X_DIFF = 0;
	const double MAX_X_DIFF = 1;

	const double DEF_X_INTG = 0;
	const double MIN_X_INTG = 0;
	const double MAX_X_INTG = 0.05;

#ifndef LINEAR_XY
	// if y independent of x
	const double DEF_Y_PROP = DEF_X_PROP; // for now
	const double MIN_Y_PROP = 0;
	const double MAX_Y_PROP = 30;

	const double DEF_Y_DIFF = DEF_X_DIFF; // for now
	const double MIN_Y_DIFF = 0;
	const double MAX_Y_DIFF = 60;

	const double DEF_Y_INTG = 0;
	const double MIN_Y_INTG = 0;
	const double MAX_Y_INTG = 0.7;
#else
	// if x and y are tied linearly

	const double DEF_XY_RATIO = 1.3;
	const double MIN_XY_RATIO = 1.0;
	const double MAX_XY_RATIO = 1.4;
#endif

	const double DEF_A_PROP = 4;
	const double MIN_A_PROP = 1;
	const double MAX_A_PROP = 5;

	const double DEF_A_DIFF = 0;
	const double MIN_A_DIFF = 0;
	const double MAX_A_DIFF = 0;

	enum {
		PARAM_X_PROP = 0, PARAM_X_DIFF, PARAM_X_INTG,
#ifndef LINEAR_XY
		PARAM_Y_PROP, PARAM_Y_DIFF, PARAM_Y_INTG,
#else
		PARAM_XY_RATIO,
#endif
		PARAM_A_PROP, PARAM_A_DIFF,
	};

	const double arr_min[] = {
		MIN_X_PROP, MIN_X_DIFF, MIN_X_INTG,
#ifndef LINEAR_XY
		MIN_Y_PROP, MIN_Y_DIFF,
#else
		MIN_XY_RATIO,
#endif
		MIN_A_PROP, MIN_A_DIFF,
	};

	const double arr_def[] = {
		DEF_X_PROP, DEF_X_DIFF, DEF_X_INTG,
#ifndef LINEAR_XY
		DEF_Y_PROP, DEF_Y_DIFF,
#else
		DEF_XY_RATIO,
#endif
		DEF_A_PROP, DEF_A_DIFF,
	};

	const double arr_max[] = {
		MAX_X_PROP, MAX_X_DIFF, MAX_X_INTG,
#ifndef LINEAR_XY
		MAX_Y_PROP, MAX_Y_DIFF,
#else
		MAX_XY_RATIO,
#endif
		MAX_A_PROP, MAX_A_DIFF,
	};

	const int P = sizeof(arr_max) / sizeof(arr_max[0]);
}

const std::vector<double> TunablePIDController::param_min(arr_min, arr_min + P);
const std::vector<double> TunablePIDController::param_max(arr_max, arr_max + P);
const std::vector<double> TunablePIDController::param_default(arr_def, arr_def + P);

TunablePIDController::TunablePIDController(World world, AI::RC::W::Player plr) : OldRobotController(world, plr), initialized(false), param(param_default), error_pos(10), error_ori(10) {
}

const std::vector<std::string> TunablePIDController::get_params_name() const {
	std::vector<std::string> ret;
	ret.push_back("Proportional X");
	ret.push_back("Differential X");
	ret.push_back("Integral X");
#ifdef LINEAR_XY
	ret.push_back("Y/X Parameter Ratio");
#else
	ret.push_back("Proportional Y");
	ret.push_back("Differential Y");
	ret.push_back("Integral Y");
#endif
	ret.push_back("Proportional Angle");
	ret.push_back("Differential Angle");
	return ret;
}

void TunablePIDController::move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) {
	const Point &current_position = player.position();
	const Angle current_orientation = player.orientation();

	// relative new direction and angle
	Angle new_da = (new_orientation - current_orientation).angle_mod();
	const Point &new_dir = (new_position - current_position).rotate(-current_orientation);

	if (new_da > Angle::HALF) {
		new_da -= Angle::FULL;
	}

	if (!initialized) {
		initialized = true;
		// make error 0
		for (std::size_t t = 9; t > 0; --t) {
			error_pos[t] = new_dir;
			error_ori[t] = new_da;
		}
		prev_new_pos = new_position;
		prev_new_ori = new_orientation;
	}

	// update the previous
	for (std::size_t t = 9; t > 0; --t) {
		error_pos[t] = error_pos[t - 1];
		error_ori[t] = error_ori[t - 1];
	}
	error_pos[0] = new_dir;
	error_ori[0] = new_da;

	Point accum_pos(0, 0);
	Angle accum_ori = Angle::ZERO;
	for (int t = 9; t >= 0; --t) {
		accum_pos *= DAMP;
		accum_ori *= DAMP;
		accum_pos += error_pos[static_cast<std::size_t>(t)];
		accum_ori += error_ori[static_cast<std::size_t>(t)];
	}

	const double px = error_pos[0].x;
	const double py = error_pos[0].y;
	const Angle pa = error_ori[0];
	Point vel = (player.velocity()).rotate(-current_orientation);
	double vx = -vel.x;
	double vy = -vel.y;
	Angle va = -player.avelocity();

	const double cx = accum_pos.x;
	const double cy = accum_pos.y;
	// const double ca = accum_ori;

	// double ovx = error_pos[0].x - error_pos[1].x;
	// double ovy = error_pos[0].y - error_pos[1].y;
	// double ova = error_ori[0] - error_ori[1];
	// dout << ovx << " " << vx << " " << ovy << " " << vy << " " << ova << " " << va << std::endl;
	// dout << dnt << " " << va << std::endl;
	// dnt++;

	// check if command has changed
	if (prev_new_pos.x != new_position.x || prev_new_pos.y != new_position.y || prev_new_ori != new_orientation) {
		prev_new_pos = new_position;
		prev_new_ori = new_orientation;
	}

	linear_velocity.x = px * param[PARAM_X_PROP] + vx * param[PARAM_X_DIFF] + cx * param[PARAM_X_INTG];

#ifndef LINEAR_XY
	linear_velocity.y = py * param[PARAM_Y_PROP] + vy * param[PARAM_Y_DIFF] + cy * param[PARAM_Y_INTG];
#else
	linear_velocity.y = (py * param[PARAM_X_PROP] + vy * param[PARAM_X_DIFF] + cy * param[PARAM_X_INTG]) * param[PARAM_XY_RATIO];
#endif

	angular_velocity = pa * param[PARAM_A_PROP] + va * param[PARAM_A_DIFF];
}

void TunablePIDController::clear() {
#warning WRITE CODE HERE
}

ROBOT_CONTROLLER_REGISTER(TunablePIDController)

