#include "jons_controller.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "world/player_impl.h"
#include <cmath>

namespace {

	class jons_controller_factory : public robot_controller_factory {
		public:
			jons_controller_factory() : robot_controller_factory("JONS RC") {
			}

			robot_controller::ptr create_controller(player_impl::ptr plr, bool, unsigned int) {
				robot_controller::ptr p(new jons_controller(plr));
				return p;
			}
	};

	jons_controller_factory factory;

}

jons_controller::jons_controller(player_impl::ptr plr) : plr(plr), max_acc(10), max_vel(8000), max_Aacc(1), time_step(1/15.0), close_param(2)
{
prev_orient = plr->orientation();
}

void jons_controller::move(const point &new_position, double new_orientation, point &linear_velocity, double &angular_velocity) {
	const point &current_position = plr->position();
	const double current_orientation = plr->orientation();
	const point &current_velocity = plr->est_velocity();
	const point diff = new_position - current_position;
	double current_angularvel;
	double vel_in_dir_travel;
	// relative new direction and angle
	double new_da = angle_mod(new_orientation - current_orientation);
	current_angularvel = angle_mod(current_orientation-prev_orient);
	if(current_angularvel > PI) current_angularvel -= 2*PI;
	current_angularvel *= time_step;
	
	if(pow(current_angularvel,2)/max_Aacc*close_param < abs(new_da))
		angular_velocity = new_da/abs(new_da)*max_vel;
	else
		angular_velocity=0;
	
	
	point new_dir = diff.rotate(-current_orientation);
	new_dir /= new_dir.len();
	if (new_da > PI) new_da -= 2 * PI;
	vel_in_dir_travel=current_velocity.dot(diff/diff.len());

	if(pow(vel_in_dir_travel,2)/max_acc*close_param < diff.len())
		linear_velocity = max_vel*new_dir;
	else
		linear_velocity = new_dir*0;
}

robot_controller_factory &jons_controller::get_factory() const {
	return factory;
}

